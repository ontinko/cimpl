#include "include/ast.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/token.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: cimpl [filename]\n");
        return 64;
    }

    char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    printf("Opened file\n");
    char *source = NULL;
    size_t size = 0;

    if (file == NULL) {
        printf("No file %s found\n", filename);
        return 64;
    }

    char ch;
    while (1) {
        char ch = fgetc(file);
        source = (char *)realloc(source, (size + 1) * sizeof(char));
        source[size] = ch;
        size++;
        if (ch == EOF) {
            break;
        }
    }

    printf("Read the file\n");
    TTHashTable preview = new_tt_ht();
    tt_ht_set(&preview, Illegal, "<ILLEGAL>");
    tt_ht_set(&preview, Eof, "<EOF>");
    tt_ht_set(&preview, Comma, ",");
    tt_ht_set(&preview, Semicolon, ";");
    tt_ht_set(&preview, LBrace, "{");
    tt_ht_set(&preview, RBrace, "}");
    tt_ht_set(&preview, LParen, "(");
    tt_ht_set(&preview, RParen, ")");
    tt_ht_set(&preview, Plus, "+");
    tt_ht_set(&preview, Minus, "-");
    tt_ht_set(&preview, Star, "*");
    tt_ht_set(&preview, Slash, "/");
    tt_ht_set(&preview, Mod, "%");
    tt_ht_set(&preview, Inc, "++");
    tt_ht_set(&preview, Dec, "--");
    tt_ht_set(&preview, True, "true");
    tt_ht_set(&preview, False, "false");
    tt_ht_set(&preview, Eq, "=");
    tt_ht_set(&preview, Colon, ":");
    tt_ht_set(&preview, ColEq, ":=");
    tt_ht_set(&preview, PlusEq, "+=");
    tt_ht_set(&preview, MinusEq, "-=");
    tt_ht_set(&preview, StarEq, "*=");
    tt_ht_set(&preview, SlashEq, "/=");
    tt_ht_set(&preview, ModEq, "%=");
    tt_ht_set(&preview, Gt, ">");
    tt_ht_set(&preview, Lt, "<");
    tt_ht_set(&preview, GtE, ">=");
    tt_ht_set(&preview, LtE, "<=");
    tt_ht_set(&preview, EqEq, "==");
    tt_ht_set(&preview, NotEq, "!=");
    tt_ht_set(&preview, Not, "!");
    tt_ht_set(&preview, Or, "||");
    tt_ht_set(&preview, And, "&&");
    tt_ht_set(&preview, If, "if");
    tt_ht_set(&preview, Else, "else");
    tt_ht_set(&preview, While, "while");
    tt_ht_set(&preview, For, "for");
    tt_ht_set(&preview, Break, "break");
    tt_ht_set(&preview, Continue, "continue");
    tt_ht_set(&preview, Def, "def");
    tt_ht_set(&preview, Return, "return");
    tt_ht_set(&preview, BoolType, "bool");
    tt_ht_set(&preview, IntType, "int");

    ParseSource p_source = tokenize(source, size);
    printf("Size: %lu\n", p_source.size);

    for (size_t i = 0; i < p_source.size; i++) {
        printf("%lu:   %s\n", i,
               token_view(&preview, p_source.tokens[i], source));
    }

    TTIntHashTable precs = new_tt_int_ht();
    tt_int_ht_set(&precs, Number, 1);
    tt_int_ht_set(&precs, Identifier, 1);
    tt_int_ht_set(&precs, True, 1);
    tt_int_ht_set(&precs, False, 1);

    tt_int_ht_set(&precs, Lt, 2);
    tt_int_ht_set(&precs, Gt, 2);
    tt_int_ht_set(&precs, EqEq, 2);
    tt_int_ht_set(&precs, NotEq, 2);

    tt_int_ht_set(&precs, Plus, 3);
    tt_int_ht_set(&precs, Minus, 3);

    tt_int_ht_set(&precs, Star, 4);
    tt_int_ht_set(&precs, Slash, 4);

    tt_int_ht_set(&precs, Mod, 5);

    tt_int_ht_set(&precs, Or, 6);
    tt_int_ht_set(&precs, And, 7);
    tt_int_ht_set(&precs, Not, 8);

    TTIntHashTable infixes = new_tt_int_ht();
    tt_int_ht_set(&infixes, Plus, 1);
    tt_int_ht_set(&infixes, Minus, 1);
    tt_int_ht_set(&infixes, Star, 1);
    tt_int_ht_set(&infixes, Slash, 1);
    tt_int_ht_set(&infixes, Mod, 1);
    tt_int_ht_set(&infixes, Or, 1);
    tt_int_ht_set(&infixes, And, 1);
    tt_int_ht_set(&infixes, EqEq, 1);
    tt_int_ht_set(&infixes, NotEq, 1);
    tt_int_ht_set(&infixes, Gt, 1);
    tt_int_ht_set(&infixes, Lt, 1);
    tt_int_ht_set(&infixes, GtE, 1);
    tt_int_ht_set(&infixes, LtE, 1);

    ParseCache cache = {.err = NULL,
                        .current = 0,
                        .legal_infixes = &infixes,
                        .precs = &precs,
                        .tokens = p_source.tokens,
                        .tokens_size = p_source.size};
    size_t pg_size = 0;
    Stmt **program = NULL;
    printf("Started parsing\n");
    parse(&cache, 0, &program, &pg_size); // breaks in the parser
    printf("Finished successfully!\n");
    if (cache.err != NULL) {
        printf("Error at token %s\n", token_view(&preview, cache.err->token, source));
    }
}
