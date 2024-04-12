#ifndef vm_h
#define vm_h
#include "bytecode.h"

typedef struct {
    char *(*const_names)[512];
    Constant (*const_values)[512];
    size_t size;
} Scope;

static void scope_init(Scope *scope);

static void scope_set(Scope *scope, char *const_name, Constant const_value);

static void scope_get(Scope *scope, char *const_name, Constant *const_value);

static void scope_destroy(Scope *scope);

typedef struct {
    Constant *stack;
    size_t stack_size;
    OpCode *commands;
    size_t commands_size;
    Constant *args;
    size_t args_size;
    int *ref_scopes;
    size_t ref_scopes_size;
    Scope **memory;
    size_t memory_size;
} VM;

void vm_init(VM *vm, OpCode *commands, size_t commands_size, Constant *args, size_t args_size, int *ref_scopes, size_t ref_scopes_size);

void vm_run(VM *vm);

static void stack_push(Constant **stack, size_t *stack_size, Constant constant);

static void stack_pop(Constant **stack, size_t *stack_size, Constant *constant);

void memory_visualize(Scope **memory, size_t memory_size);

static void memory_extend(Scope ***memory, size_t *memory_size);

static void memory_shrink(Scope ***memory, size_t *memory_size);

static void memory_store(Scope **memory, size_t memory_size, char *const_name, int const_scope, Constant constant);

static void memory_load(Scope **memory, size_t memory_size, char *const_name, int const_scope, Constant *constant);

#endif
