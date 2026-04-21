#ifndef CODEGEN_H
#define CODEGEN_H
#include "AST.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include <map>
#include <memory>
using namespace llvm;

class IRCodegen : Visitor {
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<IRBuilder<>> Builder;
  std::unique_ptr<Module> TheModule;
  std::unique_ptr<StandardInstrumentations> TheSI;
  FunctionPassManager &TheFPM;
  FunctionAnalysisManager &TheFAM;
  std::unique_ptr<PassInstrumentationCallbacks> &ThePIC;
  std::unique_ptr<ModuleAnalysisManager> &TheMAM;
  DataLayout DL;
  std::map<std::string, Value *> NamedValues;
  std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

public:
  IRCodegen(std::unique_ptr<LLVMContext> Context,
            std::unique_ptr<Module> TheModule,
            std::unique_ptr<IRBuilder<>> Builder,
            std::unique_ptr<StandardInstrumentations> TheSI,
            FunctionPassManager &TheFPM, FunctionAnalysisManager &TheFAM,
            std::unique_ptr<PassInstrumentationCallbacks> &ThePIC,
            std::unique_ptr<ModuleAnalysisManager> &TheMAM,
            const DataLayout &DL, std::map<std::string, Value *> NamedValues)
      : Context(std::move(Context)), Builder(std::move(Builder)),
        TheModule(std::move(TheModule)), TheSI(std::move(TheSI)),
        TheFPM(TheFPM), TheFAM(TheFAM), ThePIC(ThePIC), TheMAM(TheMAM), DL(DL),
        NamedValues(NamedValues), FunctionProtos() {};

  Value *visit(const ExprAST &expr);
  Value *visit(const NumberExprAST &expr);
  Value *visit(const VariableExprAST &expr);
  Value *visit(const BinaryExprAST &expr);
  Value *visit(const CallExprAST &expr);
  Value *visit(const IfExprAST &expr);
  Value *visit(const ForExprAST &expr);
  Function *visit(const PrototypeAST &expr);
  Function *visit(const FunctionAST &expr);

  Module *getModule() const { return TheModule.get(); };
  LLVMContext *getContext() const { return Context.get(); };
  std::unique_ptr<Module> takeModule() { return std::move(TheModule); };
  std::unique_ptr<LLVMContext> takeContext() { return std::move(Context); };

  void reset();

  void addFunctionProto(std::string key, std::unique_ptr<PrototypeAST> ast) {
    FunctionProtos[key] = std::move(ast);
  };

  Function* getFunction(std::string Name);

};

#endif
