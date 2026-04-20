#ifndef AST_H
#define AST_H

#include "llvm/IR/Value.h"
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

class ExprAST;
class NumberExprAST;
class VariableExprAST;
class BinaryExprAST;
class CallExprAST;
class PrototypeAST;
class FunctionAST;

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual Value *visit(const ExprAST &expr) = 0;
  virtual Value *visit(const NumberExprAST &expr) = 0;
  virtual Value *visit(const VariableExprAST &expr) = 0;
  virtual Value *visit(const BinaryExprAST &expr) = 0;
  virtual Value *visit(const CallExprAST &expr) = 0;
  virtual Function *visit(const PrototypeAST &expr) = 0;
  virtual Function *visit(const FunctionAST &expr) = 0;
};

/// Base class for all expression nodes
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual Value *accept(Visitor &V) const = 0;
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  const double &getName() const { return Val; };
  NumberExprAST(double Val) : Val(Val) {}
  Value *accept(Visitor &V) const override { return V.visit(*this); }
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  const std::string &getName() const { return Name; };
  VariableExprAST(const std::string &Name) : Name(Name) {}
  Value *accept(Visitor &V) const override { return V.visit(*this); }
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  const char &getOp() const { return Op; };
  const ExprAST *getLHS() const { return LHS.get(); };
  const ExprAST *getRHS() const { return RHS.get(); };
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

  Value *accept(Visitor &V) const override { return V.visit(*this); }
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  const std::string &getCallee() const { return Callee; };
  const std::vector<std::unique_ptr<ExprAST>> &getArgs() const { return Args; };
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
  Value *accept(Visitor &V) const override { return V.visit(*this); }
};

// A function signature (type essentially)
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  const std::string &getName() const { return Name; };
  const std::vector<std::string> &getArgs() const { return Args; };
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
      : Name(Name), Args(std::move(Args)) {};
  Function *accept(Visitor &V) { return V.visit(*this); }
};

// A function is the signature + it's body
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  const PrototypeAST *getProto() const { return Proto.get(); };
  const ExprAST *getBody() const { return Body.get(); };

  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *accept(Visitor &V) { return V.visit(*this); }
};

#endif
