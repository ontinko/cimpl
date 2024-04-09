#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "error.h"

typedef struct {
    Token **tokens;
    size_t tokens_size;
    size_t current;
    Error *err;
    TTIntHashTable *precs;
    TTIntHashTable *legal_infixes;
} ParseCache;

void parse(ParseCache *cache, int block, Stmt ***stmts, size_t *stmts_size);

static void advance(ParseCache *cache, size_t step);

static Token *peek(ParseCache *cache, size_t step);

static void add_error(ParseCache *cache, char *message, Token *token);

static Expression *parse_exp(ParseCache *cache, int prec, TokenType end);

static void parse_fn_params(ParseCache *cache, FunctionType *fn_type);

static GenericDT *parse_type(ParseCache *cache);

#endif
