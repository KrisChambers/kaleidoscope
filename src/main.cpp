// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/Support/raw_ostream.h"
#include "parser.hpp"
#include <cctype>
#include <cstdlib>
#include <map>
#include "llvm/Support/TargetSelect.h"

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


  InitializeModuleAndPassManagers();

  Parse();

  return 0;
}
