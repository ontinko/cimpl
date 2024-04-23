#ifndef bytecode_compiler_h
#define bytecode_compiler_h
#include "ast.h"
#include "bytecode.h"
#include "vm.h"

// a := 1 + 2;
// b := a + 1;
// OpCode *commands = {Push, Push, IntAdd, Store, Load, Push, Add, Store};
// Constant *data = {{.int_data = 1}, {.int_data = 2}, {.int_data = 0}, {.ind_data = 0}, {.int_data = 1}, {.int_data = 0}}

typedef struct {
    char *(*var_names)[512];
    int (*positions)[512];
    int size;
} VarPositions;

void var_positions_init(VarPositions *table);

void var_positions_destroy(VarPositions *table);

int hash(char *key);

void var_positions_set(VarPositions *table, char *var_name, int position);

int var_positions_get(VarPositions *table, char *var_name, int *position);

typedef struct {
    char *source;
    OpCode *commands;
    Constant *args;
    VarPositions *memory;
    int *scope_start_positions;
    int function_param_count;
    int program_size;
    int memory_size;
    int memory_capacity;
    int stack_index;
    int has_error;
} CompileCache;

void compile_cache_init(CompileCache *cache);

void memory_store(VarPositions *memory, int memory_size, char *var_name, int var_scope, int position);

void memory_load(VarPositions *memory, int memory_size, char *var_name, int var_scope, int *position);

void memory_extend(CompileCache *cache);

void memory_shrink(CompileCache *cache);

static void compile_expression(Expression *exp, CompileCache *cache);

static void compile_oneliner(Oneliner *oneliner, CompileCache *cache);

static void compile_call(Call *calll, CompileCache *cache);

void compile_to_bytecode(Stmt *stmts, int stmts_size, int does_wrap, CompileCache *cache);

void compile_program(Stmt *stmts, int stmts_size, CompileCache *cache);
#endif
