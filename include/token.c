#include "token.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lm_cmp(char *key1, char *key2) {
    int len1 = strlen(key1);
    int len2 = strlen(key2);
    int min_len = min(len1, len2);
    for (int i = 0; i < min_len; i++) {
        if (key1[i] < key2[i]) {
            return -1;
        }
        if (key1[i] > key2[i]) {
            return 1;
        }
    }
    if (len1 < len2) {
        return -1;
    }
    if (len1 > len2) {
        return 1;
    }
    return 0;
}

void lm_insert(LexMap *map, char *lex, TokenType ttype) {
    if (map->key == NULL) {
        map->key = lex;
        map->ttype = ttype;
        return;
    }
    int cmp = lm_cmp(map->key, lex);
    switch (cmp) {
    case -1:
        if (map->right == NULL) {
            LexMap *right = malloc(sizeof(LexMap));
            right->key = lex;
            right->ttype = ttype;
            right->left = NULL;
            right->right = NULL;
            map->right = right;
            return;
        }
        lm_insert(map->right, lex, ttype);
        return;
    case 1:
        if (map->left == NULL) {
            LexMap *left = malloc(sizeof(LexMap));
            left->key = lex;
            left->ttype = ttype;
            left->left = NULL;
            left->right = NULL;
            map->left = left;
            return;
        }
        lm_insert(map->left, lex, ttype);
        return;
    default:
        map->ttype = ttype;
        return;
    }
}

TokenType lm_get(LexMap *map, char *lex) {
    if (map == NULL) {
        return Illegal;
    }
    int cmp = lm_cmp(map->key, lex);
    switch (cmp) {
    case -1:
        return lm_get(map->right, lex);
    case 1:
        return lm_get(map->left, lex);
    default:
        return map->ttype;
    }
}

LexHashTable new_l_ht() {
    LexHashTable result = {.values = {Illegal}};
    return result;
}

TTHashTable new_tt_ht() {
    TTHashTable result = {.values = {NULL}};
    return result;
}

ChHashTable new_ch_ht() {
    ChHashTable result = {.values = {0}};
    return result;
}

TTIntHashTable new_tt_int_ht() {
    TTIntHashTable result = {.values = {-1}};
    return result;
}

void tt_ht_set(TTHashTable *table, TokenType ttype, char *lexem) {
    table->values[(size_t)ttype] = malloc(strlen(lexem) + 1);
    strcpy(table->values[(size_t)ttype], lexem);
}

char *tt_ht_get(TTHashTable *table, TokenType ttype) {
    return table->values[(size_t)ttype];
}

void l_ht_set(LexHashTable *table, char *lexem, TokenType ttype) {
    int hash = lexem[0];
    if (strlen(lexem) > 1) {
        hash += lexem[1] * 2;
    }
    table->values[hash] = ttype;
}

TokenType l_ht_get(LexHashTable *table, char *lexem) {
    int hash = lexem[0];
    if (strlen(lexem) > 1) {
        hash += lexem[1] * 2;
    }
    return table->values[hash];
}

void ch_ht_set(ChHashTable *table, char ch, TokenType ttype) {
    table->values[ch] = ttype;
}

TokenType ch_ht_get(ChHashTable *table, char ch) { return table->values[ch]; }

char *token_view(TTHashTable *preview, Token *token, char *source) {
    switch (token->ttype) {
    case Number:
        return substring(source, token->start, token->end);
    case Identifier:
        return substring(source, token->start, token->end);
    default:
        return tt_ht_get(preview, token->ttype);
    }
}

void tt_int_ht_set(TTIntHashTable *table, TokenType ttype, int prec) {
    table->values[(size_t)ttype] = prec;
}

int tt_int_ht_get(TTIntHashTable *table, TokenType ttype) {
    return table->values[(size_t)ttype];
}
