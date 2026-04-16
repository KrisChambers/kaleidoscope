#ifndef CODEGEN_H
#define CODEGEN_H
#include "AST.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include <map>
#include <memory>
using namespace llvm;

class IRCodegen : Visitor {
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<IRBuilder<>> Builder;
  std::unique_ptr<Module> TheModule;
  std::map<std::string, Value *> NamedValues;

public:
  IRCodegen(std::unique_ptr<LLVMContext> Context,
            std::unique_ptr<IRBuilder<>> Builder,
            std::unique_ptr<Module> TheModule,
            std::map<std::string, Value *> NamedValues)
      : Context(std::move(Context)), Builder(std::move(Builder)),
        TheModule(std::move(TheModule)), NamedValues(NamedValues) {};
  Value *visit(const ExprAST &expr);
  Value *visit(const NumberExprAST &expr);
  Value *visit(const VariableExprAST &expr);
  Value *visit(const BinaryExprAST &expr);
  Value *visit(const CallExprAST &expr);
  Function *visit(const PrototypeAST &expr);
  Function *visit(const FunctionAST &expr);
};

#endif
