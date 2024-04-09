#include "include/analyzer.h"
#include "include/ast.h"
#include "include/error.h"
#include "include/lexer.h"
#include "include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    int debug_lexer = 0;
    int visual_debug = 0;
    char *filename = NULL;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (arg[0] == '-') {
            if (!strcmp(arg, "-l")) {
                debug_lexer = 1;
            } else if (!strcmp(arg, "-v")) {
                visual_debug = 1;
            } else {
                printf("Usage: cimpl [filename] [-l] [-p] [-v]\n");
                return 64;
            }
        } else if (filename != NULL) {
            printf("Usage: cimpl [filename] [-l] [-p] [-v]\n");
            return 64;
        } else {
            filename = argv[i];
        }
    }

    if (filename == NULL) {
        printf("Usage: cimpl [filename] [-l] [-p] [-v]\n");
        return 64;
    }

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
    TTHashTable preview = tt_hashtable_create();
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
    tt_ht_set(&preview, Fn, "fn");
    tt_ht_set(&preview, Return, "return");
    tt_ht_set(&preview, BoolType, "bool");
    tt_ht_set(&preview, IntType, "int");

    TTIntHashTable precs = tt_int_hashtable_create();
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

    TTIntHashTable infixes = tt_int_hashtable_create();
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

    ParseSource p_source = tokenize(source, size);
    printf("Size: %lu\n", p_source.size);

    if (debug_lexer) {
        for (size_t i = 0; i < p_source.size; i++) {
            printf("%lu:   %s\n", i, token_view(&preview, p_source.tokens[i], source));
        }
    }

    ParseCache cache = {
        .err = NULL, .current = 0, .legal_infixes = &infixes, .precs = &precs, .tokens = p_source.tokens, .tokens_size = p_source.size};
    size_t pg_size = 0;
    Stmt **program = NULL;
    printf("Running the parser...\n");
    parse(&cache, 0, &program, &pg_size);
    if (cache.err != NULL) {
        error_print(cache.err);
        return 1;
    }
    AnalysisCache *an_cache = analysis_cache_create(source);
    printf("Running static analysis...\n");
    validate(an_cache, program, pg_size);
    if (an_cache->errors_size) {
        for (int i = 0; i < an_cache->errors_size; i++) {
            error_print(an_cache->errors[i]);
        }
        return 1;
    }
    if (visual_debug) {
        visualize_program(program, pg_size, 0, source);
        printf("\n");
        printf("Visualized successfully!\n");
    }
}