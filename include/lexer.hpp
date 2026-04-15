#ifndef LEXER_H
#define LEXER_H

#include <string>

extern std::string IdentifierStr;
extern double NumVal;

enum TokenType {
  EOF_TOKEN = -1,
  DEF = -2,
  EXTERN = -3,
  IDENTIFIER = -4,
  NUMBER = -5,
  OTHER = -6
};

struct Token {
    TokenType tok_type;
    char symbol;
    std::string IdentifierStr;
    double NumVal;
};


//
int gettok();

#endif
