#include "lexer.hpp"

int gettok() {
  static int LastChar = ' ';

  // Skip space
  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) { // identifier
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "def")
      return DEF;

    if (IdentifierStr == "extern")
      return EXTERN;
    return IDENTIFIER;
  }

  // Number
  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), 0);
    return NUMBER;
  }

  // Comments
  if (LastChar == '#') {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // EOF
  if (LastChar == EOF)
    return EOF_TOKEN;

  // Everything else
  int ThisChar = LastChar;
  LastChar = getchar();

  return ThisChar;
}
