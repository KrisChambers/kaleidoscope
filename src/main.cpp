// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/Support/raw_ostream.h"
#include <cctype>
#include <cstdlib>
#include "llvm/IR/IRBuilder.h"
#include <map>
#include <memory>
#include "parser.hpp"



int main() {
  BinopPrecedense['<'] = 10;
  BinopPrecedense['+'] = 20;
  BinopPrecedense['-'] = 20;
  BinopPrecedense['*'] = 40;

  fprintf(stdout, "ready> ");
  nextToken();

  InitializeModule();

  Parse();

  return 0;
}
