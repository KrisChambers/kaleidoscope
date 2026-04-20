#ifndef LOGGING_H
#define LOGGING_H

#include "AST.hpp"
#include <memory>

//
std::unique_ptr<ExprAST> LogError(std::string &Str);

//
std::unique_ptr<PrototypeAST> LogErrorP(std::string &Str);

//
Value *LogErrorV(std::string &Str);

#endif
