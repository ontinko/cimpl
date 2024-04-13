#ifndef bytecode_compiler_h
#define bytecode_compiler_h
#include "ast.h"
#include "bytecode.h"
#include <stdlib.h>

// a := 1 + 2;
// OpCode *commands = {Push, Push, IntAdd, Store};
// Constant *data = {{.int_data = 1}, {.int_data = 2}, {.var_name = "a"}}
// int *ref_scopes = {0}
void compile_to_bytecode(Stmt **stmts, size_t stmts_size, char *source, OpCode **commands, Constant **args, int **ref_scopes, size_t *program_size,
                         int *has_error);
#endif
