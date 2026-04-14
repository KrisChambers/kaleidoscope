#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cctype>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum TokenType {
  EOF_TOKEN = -1,
  DEF = -2,
  EXTERN = -3,
  IDENTIFIER = -4,
  NUMBER = -5,
};

static std::string IdentifierStr;
static double NumVal;

static int gettok() {
  static int LastChar = ' ';

  // Skip space
  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) { // identifier
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "def")
      return DEF;

    if (IdentifierStr == "extern")
      return EXTERN;
    return IDENTIFIER;
  }

  // Number
  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), 0);
    return NUMBER;
  }

  // Comments
  if (LastChar == '#') {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // EOF
  if (LastChar == EOF)
    return EOF_TOKEN;

  // Everything else
  int ThisChar = LastChar;
  LastChar = getchar();

  return ThisChar;
}

//// AST
///
///

class ExprAST {
public:
  virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
};

// A function signature (type essentially)
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
      : Name(Name), Args(std::move(Args)) {}
};

// A function is the signature + it's body
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

/// PARSER
///

std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

static int CurTok;
static int nextToken() { return CurTok = gettok(); };

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  nextToken();
  return std::move(Result);
}

// static std::unique_ptr<ExprAST> ParseExpression() {
//     if (CurTok == '(')
//         return ParseParenExpr()
//
// }

static std::unique_ptr<ExprAST> ParseParenExpr() {
  nextToken(); // skip (

  auto Expr = ParseExpression();
  if (!Expr)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  nextToken();
  return Expr;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
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

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");

      nextToken();
    }
  }

  nextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token : expected an expression");
  case IDENTIFIER:
    return ParseIdentifierExpr();
  case NUMBER:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

static std::map<char, int> BinopPrecedense;

static int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  int TokPrec = BinopPrecedense[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {

  while (true) {
    int TokPrec = GetTokPrecedence();
    if (TokPrec < ExprPrec)
      return LHS;

    int BinOp = CurTok;
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

static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (CurTok != IDENTIFIER)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  nextToken();

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> Params;
  while (nextToken() == IDENTIFIER)
    Params.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  nextToken();

  return std::make_unique<PrototypeAST>(FnName, std::move(Params));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
  nextToken(); // move ahead of 'def'
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto Expr = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));

  return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
  nextToken(); // move ahead of 'extern'
  return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto Expr = ParseExpression()) {
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));
  }

  return nullptr;
}

/// TOP LEVEL PARSING
///

static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    nextToken();
  }
}

static void HandleTopLevelExpression() {
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr.\n");
  } else {
    nextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed a top-level expr.\n");
  } else {
    nextToken();
  }
}

static void Parse() {
  while (true) {
    fprintf(stderr, "ready> ");
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

///
///

int main() {
  BinopPrecedense['<'] = 10;
  BinopPrecedense['+'] = 20;
  BinopPrecedense['-'] = 20;
  BinopPrecedense['*'] = 40;

  fprintf(stderr, "ready> ");
  int token_type = gettok();

  Parse();

  return 0;
}
