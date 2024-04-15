#ifndef vm_h
#define vm_h
#include "bytecode.h"

typedef struct {
    Constant *stack;
    int stack_size;
    int stack_capacity;
    OpCode *commands;
    Constant *args;
    int *offsets;
    int *scope_starts;
    int scope_starts_size;
    int scope_starts_capacity;
    size_t program_size;
} VM;

void vm_init(VM *vm, OpCode *commands, Constant *args, size_t program_size);

void vm_run(VM *vm);

void vm_set_scope_start(VM *vm, int index);

void vm_clear_last_scope(VM *vm);

static void stack_push(Constant **stack, int *stack_size, int *stack_capacity, Constant constant);

static void stack_pop(Constant **stack, int *stack_size, int *stack_capacity, Constant *constant);

#endif
