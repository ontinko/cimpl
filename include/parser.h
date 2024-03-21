#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "token.h"

typedef struct ParseError {
    char *message;
    Token *token;
} ParseError;

typedef struct {
    Token **tokens;
    size_t tokens_size;
    size_t current;
    ParseError *err;
    TTIntHashTable *precs;
    TTIntHashTable *legal_infixes;
} ParseCache;

void parse(ParseCache *cache, int block, Stmt ***stmts, size_t *stmts_size);

#endif
