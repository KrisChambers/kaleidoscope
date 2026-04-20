#include "parser.hpp"
#include "llvm/Support/TargetSelect.h"
#include <cctype>
#include <cstdlib>
#include <map>

int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  BinopPrecedense['<'] = 10;
  BinopPrecedense['+'] = 20;
  BinopPrecedense['-'] = 20;
  BinopPrecedense['*'] = 40;

  fprintf(stdout, "ready> ");
  nextToken();

  // Initialize JIT before module (needed for DataLayout)
  InitializeJIT();
  InitializeManagers(); // Create persistent pass managers
  InitializeModule();   // Create first module

  Parse();

  return 0;
}
