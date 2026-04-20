// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/Support/raw_ostream.h"
#include "parser.hpp"
#include "llvm/IR/IRBuilder.h"
#include <cctype>
#include <cstdlib>
#include <map>
#include <memory>

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
