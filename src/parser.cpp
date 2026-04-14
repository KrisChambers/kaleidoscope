#include "parser.hpp"
#include "lexer.hpp"
#include "logging.hpp"

int nextToken() { return CurTok = gettok(); };

std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  nextToken();
  return std::move(Result);
}

std::string FormatToken(int token) {
  switch (token) {
  case TokenType::EOF_TOKEN:
    return "EOF_TOKEN";
  case TokenType::DEF:
    return "DEF";
  case TokenType::EXTERN:
    return "EXTERN";
  default:
    return std::to_string(static_cast<char>(token)).data();
  }
}

std::unique_ptr<ExprAST> ParseParenExpr() {
  nextToken(); // skip (

  auto Expr = ParseExpression();
  if (!Expr)
    return nullptr;

  if (CurTok != ')') {
    std::string msg = "expected ')' but found " + FormatToken(CurTok);
    return LogError(msg);
  }
  nextToken();
  return Expr;
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  nextToken();

  if (CurTok != '(')
    return std::make_unique<VariableExprAST>(IdName);

  nextToken();
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',') {
        std::string msg = "Expected ')' or ',' in argument list";
        return LogError(msg);
      }

      nextToken();
    }
  }

  nextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default: {
    std::string msg = "unknown token "  + FormatToken(CurTok) + " : expected an expression";
    return LogError(msg);
  }
  case IDENTIFIER:
    return ParseIdentifierExpr();
  case NUMBER:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  int TokPrec = BinopPrecedense[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {

  while (true) {
    int TokPrec = GetTokPrecedence();
    if (TokPrec < ExprPrec)
      return LHS;

    int BinOp = CurTok;
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (CurTok != IDENTIFIER) {
    std::string msg = "Expected function name in prototype";
    return LogErrorP(msg);
  }

  std::string FnName = IdentifierStr;
  nextToken();

  if (CurTok != '(') {
    std::string msg = "Expected '(' in prototype";
    return LogErrorP(msg);
  }

  std::vector<std::string> Params;
  while (nextToken() == IDENTIFIER)
    Params.push_back(IdentifierStr);
  if (CurTok != ')') {
    std::string msg = "Expected ')' in prototype";
    return LogErrorP(msg);
  }

  nextToken();

  return std::make_unique<PrototypeAST>(FnName, std::move(Params));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  nextToken(); // move ahead of 'def'
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto Expr = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));

  return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
  nextToken(); // move ahead of 'extern'
  return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto Expr = ParseExpression()) {
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expr));
  }

  return nullptr;
}

/// TOP LEVEL PARSING
///

void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    nextToken();
  }
}

void HandleTopLevelExpression() {
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr.\n");
  } else {
    nextToken();
  }
}

void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed a top-level expr.\n");
  } else {
    nextToken();
  }
}

void Parse() {
  while (true) {
    //fprintf(stdout, "ready> ");
    switch (CurTok) {
    case EOF_TOKEN:
      return;
    case ';':
      nextToken();
      break;
    case DEF:
      HandleDefinition();
      break;
    case EXTERN:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}
