#ifndef ANALYZER_H
#define ANALYZER_H
#include "ast.h"
#include "error.h"
#include "token.h"
#include <stdlib.h>

typedef struct {
    char *(*keys)[512];
    GenericDT *(*datatypes)[512];
    size_t size;
} HashTable;

HashTable *hashtable_create();

void hashtable_set(HashTable *ht, char *key, GenericDT *value);

void hashtable_get(HashTable *ht, char *key, GenericDT **datatype);

void hashtable_destroy(HashTable *ht);

typedef struct {
    char *source;
    HashTable **defs;
    size_t cache_size;
    Error **errors;
    size_t errors_size;
    FunctionType *current_function;
    int current_scope;
    int in_loop;
} AnalysisCache;

AnalysisCache *analysis_cache_create(char *source);

static void analysis_cache_get(AnalysisCache *cache, Token *var_token, GenericDT **datatype, int *scope);

static void analysis_cache_set(AnalysisCache *cache, Token *var_token, GenericDT *datatype, int scope);

static int analysis_cache_defined(AnalysisCache *cache, Token *var_token);

static int analysis_cache_defined_in_current_scope(AnalysisCache *cache, Token *var_token);

static void analysis_cache_extend(AnalysisCache *cache);

static void analysis_cache_shrink(AnalysisCache *cache);

static void analysis_cache_add_error(AnalysisCache *cache, char *message, ErrorType type, Token *token);

static void analysis_cache_process_expression(AnalysisCache *cache, Expression *exp, GenericDT **datatype);

static void analysis_cache_process_oneliner(AnalysisCache *cache, Oneliner *oneliner);

void validate(AnalysisCache *cache, Stmt **stmts, size_t stmts_size);

#endif
