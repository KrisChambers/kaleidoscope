#include "AST.hpp"
#include "logging.hpp"

std::unique_ptr<ExprAST> LogError(std::string &Str) {
  fprintf(stderr, "Error: %s\n", Str.c_str());
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(std::string &Str) {
  LogError(Str);
  return nullptr;
}

Value *LogErrorV(std::string &Str) {
    LogError(Str);
    return nullptr;
}
