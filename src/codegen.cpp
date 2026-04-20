#include "codegen.hpp"
#include "logging.hpp"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"

Value *IRCodegen::visit(const ExprAST &expr) { return expr.accept(*this); }

Value *IRCodegen::visit(const NumberExprAST &expr) {
  return ConstantFP::get(*Context, APFloat(expr.getName()));
}

Value *IRCodegen::visit(const VariableExprAST &expr) {
  auto Name = expr.getName();
  Value *V = NamedValues[Name];
  if (!V) {
    std::string msg = "Unkown varialbe name " + Name;
    return LogErrorV(msg);
  }
  return V;
}

Value *IRCodegen::visit(const BinaryExprAST &expr) {
  auto Op = expr.getOp();
  auto LHS = expr.getLHS();
  auto RHS = expr.getRHS();

  Value *L = visit(*LHS);
  Value *R = visit(*RHS);

  if (!L | !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*Context));
  default: {
    auto msg = "Invalid binary operator : " + std::string{Op};
    return LogErrorV(msg);
  }
  }
}

Value *IRCodegen::visit(const CallExprAST &expr) {
  auto Callee = expr.getCallee();
  const auto &Args = expr.getArgs();
  // Find the function
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF) {
    auto msg = "Unknown function reference : " + Callee;
    return LogErrorV(msg);
  }

  // Check for arg list mismatch
  if (CalleeF->arg_size() != Args.size()) {
    std::string msg = "Incorrect # of arguments provided";
    return LogErrorV(msg);
  }

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    auto Arg = Args[i].get();
    ArgsV.push_back(visit(*Arg));
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *IRCodegen::visit(const PrototypeAST &expr) {
  auto Args = expr.getArgs();
  auto Name = expr.getName();

  // A Prototype is part of the interface in this language, not an Expr.
  // So we are not returning a Value here.
  std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*Context));

  // Creates a FunctionType for our Prototype (where everythng is of type
  // double)
  FunctionType *FT =
      FunctionType::get(Type::getDoubleTy(*Context), Doubles, false);

  // This is the IR for a function and registers it in the modules symbol table.
  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Then we set a name for each function argument.
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(Args[Idx++]);
  }

  return F;
}

Function *IRCodegen::visit(const FunctionAST &expr) {
  auto Proto = expr.getProto();
  auto Body = expr.getBody();
  // First we want to look through the symbol table and try to find an existing
  // version.
  Function *TheFunction = TheModule->getFunction(Proto->getName());

  // If it doesn't exist we create one from the prototype
  if (!TheFunction)
    TheFunction = visit(*Proto);

  if (!TheFunction)
    return nullptr;

  // Want to make sure that it is empty.
  if (!TheFunction->empty()) {
    std::string msg = "Function can not be redefined";
    return (Function *)LogErrorV(msg);
  }

  // Create a new basic block and set the insert point to the end of it.
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Add the argument names to the NamedValues so VarialbeExprAST can see them.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;

  if (Value *RetVal = visit(*Body)) {
    Builder->CreateRet(RetVal);
    verifyFunction(*TheFunction);

    TheFPM.run(*TheFunction, TheFAM);

    return TheFunction;
  }

  TheFunction->eraseFromParent();
  return nullptr;
}

void IRCodegen::reset() {
  Context = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("jitty", *Context);
  TheModule->setDataLayout(DL);
  Builder = std::make_unique<IRBuilder<>>(*Context);
  TheSI = std::make_unique<StandardInstrumentations>(*Context, true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());
}
