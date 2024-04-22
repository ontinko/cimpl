#ifndef bytecode_compiler_h
#define bytecode_compiler_h
#include "ast.h"
#include "bytecode.h"
#include "vm.h"

// a := 1 + 2;
// b := a + 1;
// OpCode *commands = {Push, Push, IntAdd, Store, Load, Push, Add, Store};
// Constant *data = {{.int_data = 1}, {.int_data = 2}, {.int_data = 1}}
// int *offsets = {1}

typedef struct {
    char *(*var_names)[512];
    size_t (*positions)[512];
    size_t size;
} VarPositions;

void var_positions_init(VarPositions *table);

void var_positions_destroy(VarPositions *table);

int hash(char *key);

void var_positions_set(VarPositions *table, char *var_name, size_t position);

void var_positions_get(VarPositions *table, char *var_name, size_t *position);

typedef struct {
    char *source;
    OpCode *commands;
    Constant *args;
    VarPositions *memory;
    int *definition_counts;
    size_t program_size;
    size_t memory_size;
    size_t memory_capacity;
    size_t stack_index;
    int has_error;
} CompileCache;

void compile_cache_init(CompileCache *cache);

void memory_store(VarPositions *memory, size_t memory_size, char *var_name, int var_scope, size_t position);

void memory_load(VarPositions *memory, size_t memory_size, char *var_name, int var_scope, size_t *position);

void memory_extend(CompileCache *cache);

void memory_shrink(CompileCache *cache);

void compile_to_bytecode(Stmt *stmts, size_t stmts_size, CompileCache *cache);
#endif
