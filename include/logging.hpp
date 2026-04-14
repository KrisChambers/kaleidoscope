#ifndef LOGGING_H
#define LOGGING_H

#include <memory>
#include "AST.hpp"

//
std::unique_ptr<ExprAST> LogError(std::string& Str);

//
std::unique_ptr<PrototypeAST> LogErrorP(std::string& Str);

#endif
