#include "lexer.hpp"
#include <memory>

std::string IdentifierStr = "";
double NumVal = 0;

static std::unique_ptr<Token> create_def() {
  auto token = std::make_unique<Token>();
  token->tok_type = DEF;

  return token;
}

static std::unique_ptr<Token> create_eof() {
  auto token = std::make_unique<Token>();
  token->tok_type = EOF_TOKEN;

  return token;
}

static std::unique_ptr<Token> create_extern() {
  auto token = std::make_unique<Token>();
  token->tok_type = EXTERN;

  return token;
}

static std::unique_ptr<Token> create_ident(std::string Value) {
  auto token = std::make_unique<Token>();
  token->tok_type = IDENTIFIER;
  token->IdentifierStr = Value;

  return token;
}

static std::unique_ptr<Token> create_number(double Value) {
  auto token = std::make_unique<Token>();
  token->tok_type = NUMBER;
  token->NumVal = Value;

  return token;
}

static std::unique_ptr<Token> create_other(char Value) {
  auto token = std::make_unique<Token>();
  token->tok_type = OTHER;
  token->symbol = Value;

  return token;
}

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
    if (IdentifierStr == "if")
      return IF;
    if (IdentifierStr == "then")
      return THEN;
    if (IdentifierStr == "else")
      return ELSE;
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

// I think we want to do something like this.
std::unique_ptr<Token> gettok2() {
  static int LastChar = ' ';

  // Skip space
  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) { // identifier
    std::string IdStr = std::string{static_cast<char>(LastChar)};
    while (isalnum((LastChar = getchar())))
      IdStr += LastChar;

    if (IdStr == "def")
      return create_def();

    if (IdStr == "extern")
      return create_extern();
    return create_ident(IdStr);
  }

  // Number
  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    return create_number(strtod(NumStr.c_str(), 0));
  }

  // Comments
  if (LastChar == '#') {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok2();
  }

  // EOF
  if (LastChar == EOF)
    return create_eof();

  // Everything else
  int ThisChar = LastChar;
  LastChar = getchar();

  return create_other(ThisChar);
}
