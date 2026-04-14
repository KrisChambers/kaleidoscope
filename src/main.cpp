// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/Support/raw_ostream.h"
#include <cctype>
#include <cstdlib>
#include <map>
#include "lexer.hpp"
#include "parser.hpp"


int main() {
  BinopPrecedense['<'] = 10;
  BinopPrecedense['+'] = 20;
  BinopPrecedense['-'] = 20;
  BinopPrecedense['*'] = 40;

  fprintf(stdout, "ready> ");
  nextToken();

  Parse();

  return 0;
}
