#ifndef CODEGEN_H
#define CODEGEN_H

#include <memory>
#include <map>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

using namespace llvm;

std::unique_ptr<LLVMContext> Context;
std::unique_ptr<IRBuilder<>> Builder;
std::unique_ptr<Module> Module;
std::map<std::string, Value*> NamedValues;


#endif
