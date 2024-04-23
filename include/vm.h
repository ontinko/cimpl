#ifndef vm_h
#define vm_h
#include "bytecode.h"

typedef struct {
    Constant *stack;
    int stack_size;
    int stack_capacity;
    OpCode *commands;
    Constant *args;
    size_t program_size;
    int *command_return_points;
    int *stack_return_points;
    int fn_calls_size;
    int fn_calls_capacity;
} VM;

void vm_init(VM *vm, OpCode *commands, Constant *args, size_t program_size);

void vm_run(VM *vm);

static void stack_resize(Constant **stack, int stack_size, int *stack_capacity);

static void stack_push(Constant **stack, int *stack_size, int *stack_capacity, Constant constant);

static void stack_pop(Constant **stack, int *stack_size, int *stack_capacity, Constant *constant);

#endif
