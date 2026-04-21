#ifndef PARSER_H
#define PARSER_H

#include "AST.hpp"
#include "codegen.hpp"
#include <cctype>
#include <cstdlib>
#include <map>
#include <memory>

extern std::map<char, int> BinopPrecedense;
extern std::unique_ptr<IRCodegen> IRCodegenerator;
int GetTokPrecedence();
void InitializeManagers();
void InitializeModule();
void InitializeJIT();

static int CurTok;
std::unique_ptr<ExprAST> ParseExpression();
int nextToken();
std::unique_ptr<ExprAST> ParseNumberExpr();
std::unique_ptr<ExprAST> ParseParenExpr();
std::unique_ptr<ExprAST> ParseIdentifierExpr();
std::unique_ptr<ExprAST> ParseIfExpr();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<ExprAST> ParseForExpr();
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS);
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<PrototypeAST> ParsePrototype();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();
void HandleDefinition();
void HandleTopLevelExpression();
void HandleExtern();
void Parse();

#endif
