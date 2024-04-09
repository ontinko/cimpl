#include "token.h"
#include "utils.h"
#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParseSource tokenize(char *source, size_t source_size) {
    ParseSource result = {.size = 0};

    ChHashTable singlechars = ch_hashtable_create();
    ch_ht_set(&singlechars, ',', Comma);
    ch_ht_set(&singlechars, ';', Semicolon);
    ch_ht_set(&singlechars, '{', LBrace);
    ch_ht_set(&singlechars, '}', RBrace);
    ch_ht_set(&singlechars, '(', LParen);
    ch_ht_set(&singlechars, ')', RParen);

    ChHashTable operators = ch_hashtable_create();
    ch_ht_set(&operators, '+', Plus);
    ch_ht_set(&operators, '-', Minus);
    ch_ht_set(&operators, '*', Star);
    ch_ht_set(&operators, '/', Slash);
    ch_ht_set(&operators, '%', Mod);
    ch_ht_set(&operators, '<', Lt);
    ch_ht_set(&operators, '>', Gt);
    ch_ht_set(&operators, '!', Not);
    ch_ht_set(&operators, ':', Colon);
    ch_ht_set(&operators, '=', Eq);

    ChHashTable eq_modifiers = ch_hashtable_create();
    ch_ht_set(&eq_modifiers, '+', PlusEq);
    ch_ht_set(&eq_modifiers, '-', MinusEq);
    ch_ht_set(&eq_modifiers, '*', StarEq);
    ch_ht_set(&eq_modifiers, '/', SlashEq);
    ch_ht_set(&eq_modifiers, '%', ModEq);
    ch_ht_set(&eq_modifiers, '<', LtE);
    ch_ht_set(&eq_modifiers, '>', GtE);
    ch_ht_set(&eq_modifiers, '!', NotEq);
    ch_ht_set(&eq_modifiers, ':', ColEq);
    ch_ht_set(&eq_modifiers, '=', EqEq);

    ChHashTable self_modifiers = ch_hashtable_create();
    ch_ht_set(&self_modifiers, '+', Inc);
    ch_ht_set(&self_modifiers, '-', Dec);

    LexMap keywords = {.key = NULL, .left = NULL, .right = NULL};
    lm_insert(&keywords, "if", If);
    lm_insert(&keywords, "else", Else);
    lm_insert(&keywords, "for", For);
    lm_insert(&keywords, "while", While);
    lm_insert(&keywords, "break", Break);
    lm_insert(&keywords, "continue", Continue);
    lm_insert(&keywords, "fn", Fn);
    lm_insert(&keywords, "return", Return);
    lm_insert(&keywords, "int", IntType);
    lm_insert(&keywords, "bool", BoolType);
    lm_insert(&keywords, "true", True);
    lm_insert(&keywords, "false", False);

    size_t start = 0;
    size_t end;
    size_t line = 1;
    size_t line_start = 0;
    printf("Started lexing\n");
    while (start < source_size) {
        end = start + 1;
        Token *token = malloc(sizeof(Token));
        token->ln = line;
        token->start = start;
        token->ln_start = line_start;
        char ch = source[start];
        switch (ch) {
        case ' ':
            start++;
            continue;
        case '\n':
            line++;
            start++;
            line_start = start;
            continue;
        case '#':
            while (start < source_size && source[start] != '\n') {
                start++;
            }
            continue;
        case '|':
            if (source[start + 1] == '|') {
                token->ttype = Or;
                start += 2;
            } else {
                token->ttype = Illegal;
                start++;
            }
            end = start;
            break;
        case '&':
            if (source[start + 1] == '&') {
                token->ttype = And;
                start += 2;
            } else {
                token->ttype = Illegal;
                start++;
            }
            end = start;
            break;
        case -1:
            token->ttype = Eof;
            start++;
            break;
        default: {
            TokenType op_ttype = ch_ht_get(&operators, ch);
            if (op_ttype != Illegal) {
                if (source[start + 1] == '=') {
                    token->ttype = ch_ht_get(&eq_modifiers, ch);
                    start += 2;
                    end = start;
                    break;
                }
                if (source[start + 1] == ch) {
                    TokenType ttype = ch_ht_get(&self_modifiers, ch);
                    if (ttype != Illegal) {
                        token->ttype = ttype;
                        start += 2;
                        end = start;
                        break;
                    }
                }
                start++;
                token->ttype = op_ttype;
                break;
            }
            TokenType schar_ttype = ch_ht_get(&singlechars, ch);
            if (schar_ttype != Illegal) {
                token->ttype = schar_ttype;
                start++;
                break;
            }
            if (isdigit(ch)) {
                while (isdigit(source[end])) {
                    end++;
                }
                token->ttype = Number;
                start = end;
                break;
            }
            if (isalpha(ch) || ch == '_') {
                while (isdigit(source[end]) || isalpha(source[end]) || source[end] == '_') {
                    end++;
                }
                TokenType kwtype = lm_get(&keywords, substring(source, start, end));
                if (kwtype != Illegal) {
                    token->ttype = kwtype;
                } else {
                    token->ttype = Identifier;
                }
                start = end;
                break;
            }
            token->ttype = Illegal;
            start++;
        }
        }
        token->end = end;
        result.size++;
        result.tokens = realloc(result.tokens, result.size * sizeof(Token));
        result.tokens[result.size - 1] = token;
    }
    return result;
}
