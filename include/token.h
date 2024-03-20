#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

typedef enum {
    Illegal,
    Eof,

    Comma,     // ,
    Semicolon, // ;

    LBrace, // {
    RBrace, // }
    LParen, // (
    RParen, // )

    Plus,  // +
    Minus, // -
    Star,  // *
    Slash, // /
    Mod,   // %

    Inc, // ++
    Dec, // --

    Number, // any value that consists of numeric chars only
    True,   // true
    False,  // false

    Identifier, // any alphanumeric value starting with an alpha char
    Colon,

    Eq,      // =
    ColEq,   // :=
    PlusEq,  // +=
    MinusEq, // -=
    StarEq,  // *=
    SlashEq, // /=
    ModEq,   // %=

    Gt,  // >
    Lt,  // <
    GtE, // >=
    LtE, // <=

    EqEq,  // ==
    NotEq, // !=

    Not, // !

    Or,  // ||
    And, // &&

    If,    // if
    Else,  // else
    While, // while
    For,   // for

    Break,    // break
    Continue, // continue

    Def,    // def
    Return, // return

    IntType, // int
    BoolType // bool

} TokenType;

typedef struct {
    TokenType ttype;
    size_t ln;
    size_t ln_start;
    size_t start;
    size_t end;
} Token;

typedef struct {
    size_t size;
    Token *tokens;
} ParseSource;

typedef struct {
    char *values[46];
} TTHashTable;

typedef struct {
    TokenType values[400];
} LexHashTable;

typedef struct {
    TokenType values[300];
} ChHashTable;

typedef struct Node {
    char *key;
    TokenType ttype;
    struct Node *left;
    struct Node *right;
} LexMap;

void lm_insert(LexMap *map, char *lex, TokenType ttype);

TokenType lm_get(LexMap *map, char *lex);

LexHashTable new_l_ht();

TTHashTable new_tt_ht();

ChHashTable new_ch_ht();

void tt_ht_set(TTHashTable *table, TokenType ttype, char *lexem);

void l_ht_set(LexHashTable *table, char *lexem, TokenType ttype);

void ch_ht_set(ChHashTable *table, char ch, TokenType ttype);

char *tt_ht_get(TTHashTable *table, TokenType ttype);

TokenType l_ht_get(LexHashTable *table, char *lexem);

TokenType ch_ht_get(ChHashTable *table, char ch);

char *token_view(TTHashTable *preview, Token *token, char *source);

#endif
