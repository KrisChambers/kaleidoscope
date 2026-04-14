#ifndef LEXER_H
#define LEXER_H

#include <string>

static std::string IdentifierStr;
static double NumVal;

enum TokenType {
  EOF_TOKEN = -1,
  DEF = -2,
  EXTERN = -3,
  IDENTIFIER = -4,
  NUMBER = -5,
};


//
int gettok();

#endif
