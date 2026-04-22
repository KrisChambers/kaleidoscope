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
class IfExprAST;
class ForExprAST;

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual Value *visit(const ExprAST &expr) = 0;
  virtual Value *visit(const NumberExprAST &expr) = 0;
  virtual Value *visit(const VariableExprAST &expr) = 0;
  virtual Value *visit(const BinaryExprAST &expr) = 0;
  virtual Value *visit(const CallExprAST &expr) = 0;
  virtual Value *visit(const IfExprAST &expr) = 0;
  virtual Value *visit(const ForExprAST &expr) = 0;
  virtual Function *visit(const PrototypeAST &expr) = 0;
  virtual Function *visit(const FunctionAST &expr) = 0;
};

/// Base class for all expression nodes
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual Value *accept(Visitor &V) const = 0;
  virtual std::string getTypeName() const = 0;
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  const double &getName() const { return Val; };
  NumberExprAST(double Val) : Val(Val) {}
  Value *accept(Visitor &V) const override { return V.visit(*this); }
  std::string getTypeName() const override { return "NumberExprAST";};
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  const std::string &getName() const { return Name; };
  VariableExprAST(const std::string &Name) : Name(Name) {}
  Value *accept(Visitor &V) const override { return V.visit(*this); }
  std::string getTypeName() const override { return "VariableExprAST";};
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
  std::string getTypeName() const override { return "BinaryExprAST";};
};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
      : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}

  const ExprAST* getStart() const { return Start.get(); };
  const ExprAST* getEnd() const { return End.get(); };
  const ExprAST* getStep() const { return Step.get(); };
  const ExprAST* getBody() const { return Body.get(); };
  const std::string &getVarName() const { return VarName; };

  Value *accept(Visitor &V) const override { return V.visit(*this); };
  std::string getTypeName() const override { return "ForExprAST";};
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
  std::string getTypeName() const override { return "CallExprAST";};
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  const ExprAST *getCond() const { return Cond.get(); };
  const ExprAST *getThen() const { return Then.get(); };
  const ExprAST *getElse() const { return Else.get(); };
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  Value *accept(Visitor &V) const override { return V.visit(*this); }
  std::string getTypeName() const override { return "IfExprAST";};
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
  std::string getTypeName() const { return "PrototypeAST";};
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
  std::string getTypeName() const { return "FunctionAST";};
};

#endif
