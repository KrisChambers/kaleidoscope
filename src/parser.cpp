#include "parser.hpp"
#include "codegen.hpp"
#include "kaleidoscopejit.hpp"
#include "lexer.hpp"
#include "logging.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <llvm-18/llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;
using namespace orc;

static std::unique_ptr<IRCodegen> TheCodegen;
static std::unique_ptr<KaleidoscopeJIT> TheJIT;
static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::map<std::string, Value *> NamedValues;
static std::unique_ptr<FunctionPassManager> TheFPM;
static std::unique_ptr<LoopAnalysisManager> TheLAM;
static std::unique_ptr<FunctionAnalysisManager> TheFAM;
static std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
static std::unique_ptr<ModuleAnalysisManager> TheMAM;
static std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
static std::unique_ptr<StandardInstrumentations> TheSI;
static std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
static ExitOnError ExitOnErr;

static void TheInit() {
}

void InitializeModuleAndPassManagers(void) {
  auto Context = std::make_unique<LLVMContext>();
  auto TheModule = std::make_unique<Module>("jitty", *Context);
  std::map<std::string, Value *> NamedValues = {};
  TheModule->setDataLayout(TheJIT->getDataLayout());

  auto Builder = std::make_unique<IRBuilder<>>(*Context);

  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);

  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  TheFPM->addPass(InstCombinePass());
  TheFPM->addPass(ReassociatePass());
  TheFPM->addPass(GVNPass());
  TheFPM->addPass(SimplifyCFGPass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);

  TheCodegen =
      std::make_unique<IRCodegen>(std::move(Context), std::move(Builder),
                                  std::move(TheModule), NamedValues);
}

std::string FormatToken(int token) {
  switch (token) {
  case TokenType::EOF_TOKEN:
    return "EOF_TOKEN";
  case TokenType::DEF:
    return "DEF";
  case TokenType::EXTERN:
    return "EXTERN";
  default:
    return std::to_string(static_cast<char>(token)).data();
  }
}

int nextToken() { return CurTok = gettok(); };

std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  nextToken();
  return std::move(Result);
}

std::unique_ptr<ExprAST> ParseParenExpr() {
  nextToken(); // skip (

  auto Expr = ParseExpression();
  if (!Expr)
    return nullptr;

  if (CurTok != ')') {
    std::string msg = "expected ')' but found " + FormatToken(CurTok);
    return LogError(msg);
  }
  nextToken();
  return Expr;
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  nextToken();

  if (CurTok != '(')
    return std::make_unique<VariableExprAST>(IdName);

  nextToken();
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',') {
        std::string msg = "Expected ')' or ',' in argument list";
        return LogError(msg);
      }

      nextToken();
    }
  }

  nextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default: {
    std::string msg =
        "unknown token " + FormatToken(CurTok) + " : expected an expression";
    return LogError(msg);
  }
  case IDENTIFIER:
    return ParseIdentifierExpr();
  case NUMBER:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  int TokPrec = BinopPrecedense[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {

  while (true) {
    int TokPrec = GetTokPrecedence();
    if (TokPrec < ExprPrec)
      return LHS;

    int BinOp = CurTok;
    nextToken();

    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (CurTok != IDENTIFIER) {
    std::string msg = "Expected function name in prototype";
    return LogErrorP(msg);
  }

  std::string FnName = IdentifierStr;
  nextToken();

  if (CurTok != '(') {
    std::string msg = "Expected '(' in prototype";
    return LogErrorP(msg);
  }

  std::vector<std::string> Params;
  while (nextToken() == IDENTIFIER)
    Params.push_back(IdentifierStr);
  if (CurTok != ')') {
    std::string msg = "Expected ')' in prototype";
    return LogErrorP(msg);
  }

  nextToken();

  return std::make_unique<PrototypeAST>(FnName, std::move(Params));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  nextToken(); // move ahead of 'def'
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto Expr = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));

  return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
  nextToken(); // move ahead of 'extern'
  return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto Expr = ParseExpression()) {
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));
  }

  return nullptr;
}

/// TOP LEVEL PARSING
///

void HandleDefinition() {
  fprintf(stderr, "boop\n");
  if (auto Ast = ParseDefinition()) {
    if (auto *IR = TheCodegen->visit(*Ast.get())) {
      fprintf(stderr, "Parsed a function definition.\n");
      IR->print(errs());
      fprintf(stderr, "\n");

      ExitOnErr(TheJIT->addModule(
          ThreadSafeModule(TheCodegen->getModule(), TheCodegen->getContext())));
      IR->eraseFromParent();
    }
  } else {
    nextToken();
  }
}

void HandleTopLevelExpression() {
  if (auto Ast = ParseTopLevelExpr()) {
    if (auto *IR = TheCodegen->visit(*Ast.get())) {
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();
      auto TSM =
          ThreadSafeModule(TheCodegen->getModule(), TheCodegen->getContext());
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      InitializeModuleAndPassManagers();

      auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
      auto Addr = ExprSymbol.getAddress();
      double (*FP)() = reinterpret_cast<double (*)()>(Addr.getValue());
      fprintf(stderr, "Evaluated to %f\n", FP());
      ExitOnErr(RT->remove());
      fprintf(stderr, "Parsed a top-level expr.\n");
      IR->print(errs());
      fprintf(stderr, "\n");
      IR->eraseFromParent();
    }
  } else {
    nextToken();
  }
}

void HandleExtern() {
  if (auto Ast = ParseExtern()) {
    if (auto *IR = TheCodegen->visit(*Ast.get())) {
      fprintf(stderr, "Parsed a top-level expr.\n");
      IR->print(errs());
      fprintf(stderr, "\n");
      IR->eraseFromParent();
    }
  } else {
    nextToken();
  }
}
std::map<char, int> BinopPrecedense = {};

void Parse() {
  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

  while (true) {
    switch (CurTok) {
    case EOF_TOKEN:
      return;
    case ';':
      nextToken();
      break;
    case DEF:
      HandleDefinition();
      break;
    case EXTERN:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}
