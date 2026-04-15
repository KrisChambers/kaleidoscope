#include "AST.hpp"
#include <memory>
#include <map>
#include <vector>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "logging.hpp"

using namespace llvm;

static std::unique_ptr<LLVMContext> Context;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value*> NamedValues;


void InitializeModule() {
    Context = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("jitty", *Context);

    Builder = std::make_unique<IRBuilder<>>(*Context);
}

Value *NumberExprAST::codegen() {
    return ConstantFP::get(*Context, APFloat(Val));
}

Value *VariableExprAST::codegen() {
    Value *V = NamedValues[Name];
    if (!V) {
        std::string msg = "Unkown varialbe name " + Name;
        return LogErrorV(msg);

    }
    return V;
}

Value *BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();

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
            auto msg = "Invalid binary operator : " + std::string {Op};
            return LogErrorV(msg);
        }
    }
}

Value *CallExprAST::codegen() {
    // Find the function
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF){
        auto msg = "Unknown function reference : " + Callee;
        return LogErrorV(msg);
    }

    // Check for arg list mismatch
    if (CalleeF->arg_size() != Args.size()) {
        std::string msg = "Incorrect # of arguments provided";
        return LogErrorV(msg);
    }

    std::vector<Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}


Function *PrototypeAST::codegen() {
    // A Prototype is part of the interface in this language, not an Expr.
    // So we are not returning a Value here.
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*Context));

    // Creates a FunctionType for our Prototype (where everythng is of type double)
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*Context), Doubles, false);

    // This is the IR for a function and registers it in the modules symbol table.
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Then we set a name for each function argument.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        Arg.setName(Args[Idx++]);
    }

    return F;
}

Function *FunctionAST::codegen() {
    // First we want to look through the symbol table and try to find an existing version.
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    // If it doesn't exist we create one from the prototype
    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    // Want to make sure that it is empty.
    if (!TheFunction->empty()) {
        std::string msg = "Function can not be redefined";
        return (Function*)LogErrorV(msg);
    }

    // Create a new basic block and set the insert point to the end of it.
    BasicBlock *BB = BasicBlock::Create(*Context, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Add the argument names to the NamedValues so VarialbeExprAST can see them.
    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = Body->codegen()) {
        Builder->CreateRet(RetVal);
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}
