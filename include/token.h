#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

// When adding new token types, DON'T FORGET TO CHECK TOKEN TYPE HASH TABLE BUCKET SIZES
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
    Text,   // strings

    Identifier, // any alphanumeric value starting with an alpha char
    Colon,      // :

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

    Fn,     // fn
    Return, // return

    IntType,    // int
    BoolType,   // bool
    StringType, // string

    // built in commands
    Println,
} TokenType;

typedef struct {
    TokenType ttype;
    size_t ln;
    size_t ln_start;
    size_t start;
    size_t end;
} Token;

typedef struct {
    Token *tokens;
    size_t size;
    size_t capacity;
} ParseSource;

typedef struct {
    char *values[49];
} TTHashTable;

typedef struct {
    TokenType values[400];
} LexHashTable;

typedef struct {
    TokenType values[300];
} ChHashTable;

typedef struct {
    int values[49];
} TTIntHashTable;

typedef struct LexMap {
    char *key;
    TokenType ttype;
    struct LexMap *left;
    struct LexMap *right;
} LexMap;

void lm_insert(LexMap *map, char *lex, TokenType ttype);

TokenType lm_get(LexMap *map, char *lex);

LexHashTable lex_hashtable_create();

TTHashTable tt_hashtable_create();

ChHashTable ch_hashtable_create();

TTIntHashTable tt_int_hashtable_create();

void tt_ht_set(TTHashTable *table, TokenType ttype, char *lexem);

void l_ht_set(LexHashTable *table, char *lexem, TokenType ttype);

void ch_ht_set(ChHashTable *table, char ch, TokenType ttype);

void tt_int_ht_set(TTIntHashTable *table, TokenType ttype, int prec);

int tt_int_ht_get(TTIntHashTable *table, TokenType ttype);

char *tt_ht_get(TTHashTable *table, TokenType ttype);

TokenType l_ht_get(LexHashTable *table, char *lexem);

TokenType ch_ht_get(ChHashTable *table, char ch);

char *token_view(TTHashTable *preview, Token *token, char *source);

#endif
