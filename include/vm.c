#include "vm.h"
#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>

static void stack_push(Constant **stack, int *stack_size, int *stack_capacity, Constant constant) {
    if (*stack_capacity <= *stack_size) {
        *stack_capacity *= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    }
    (*stack)[*stack_size] = constant;
    (*stack_size)++;
}

static void stack_pop(Constant **stack, int *stack_size, int *stack_capacity, Constant *constant) {
    if (*stack_capacity > 1024 && *stack_size <= *stack_capacity / 3) {
        *stack_capacity /= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    }
    (*stack_size)--;
    *constant = (*stack)[*stack_size];
}

void vm_init(VM *vm, OpCode *commands, Constant *args, size_t program_size) {
    vm->program_size = program_size;
    vm->commands = commands;
    vm->args = args;
    vm->stack_size = 0;
    vm->stack_capacity = 128;
    vm->scope_starts_size = 0;
    vm->scope_starts_capacity = 32;
    Constant *stack = malloc(sizeof(Constant) * vm->stack_capacity);
    int *stack_starts = malloc(sizeof(int) * vm->scope_starts_capacity);
    vm->stack = stack;
    vm->scope_starts = stack_starts;
}

void vm_set_scope_start(VM *vm, int index) {
    if (vm->scope_starts_capacity <= vm->scope_starts_size) {
        vm->scope_starts_capacity *= 2;
        int *new_scope_starts = realloc(vm->scope_starts, vm->scope_starts_capacity);
        vm->scope_starts = new_scope_starts;
    }
    vm->scope_starts[vm->scope_starts_size] = index;
    vm->scope_starts_size++;
}

void vm_clear_last_scope(VM *vm) {
    vm->scope_starts_size--;
    vm->stack_size = vm->scope_starts[vm->scope_starts_size];
}

void vm_run(VM *vm) {
    int command_counter = 0;
    while (command_counter < vm->program_size) {
        OpCode command = vm->commands[command_counter];
        switch (command) {
        case CreateScopeCode:
            vm_set_scope_start(vm, vm->stack_size);
            break;
        case DestroyScopeCode:
            vm_clear_last_scope(vm);
            break;
        case PushCode:
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, vm->args[command_counter]);
            break;
        case LoadCode: {
            int offset = vm->args[command_counter].int_data;
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, vm->stack[vm->stack_size - offset - 1]);
            break;
        }
        case StoreCode: {
            int offset = vm->args[command_counter].int_data;
            if (offset != 0) {
                Constant constant;
                stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &constant);
                vm->stack[vm->stack_size - offset] = constant; // no "offset - 1" because stack_size is decreased by pop
            }
            break;
        }
        case IntAddCode:
        case IntSubtractCode:
        case IntMultiplyCode:
        case IntDivideCode:
        case IntModCode:
        case IntEqCode:
        case IntNotEqCode:
        case IntGtCode:
        case IntLtCode:
        case IntGtECode:
        case IntLtECode:
        case BoolAndCode:
        case BoolOrCode: {
            Constant result;
            Constant left;
            Constant right;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &right);
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &left);
            switch (command) {
            case IntAddCode:
                result.int_data = left.int_data + right.int_data;
                break;
            case IntSubtractCode:
                result.int_data = left.int_data - right.int_data;
                break;
            case IntMultiplyCode:
                result.int_data = left.int_data * right.int_data;
                break;
            case IntDivideCode:
                result.int_data = left.int_data / right.int_data;
                break;
            case IntModCode:
                result.int_data = left.int_data % right.int_data;
                break;
            case IntEqCode:
            case IntNotEqCode:
                result.int_data = left.int_data == right.int_data && command == IntEqCode;
                break;
            case IntGtCode:
                result.int_data = left.int_data > right.int_data;
                break;
            case IntLtCode:
                result.int_data = left.int_data < right.int_data;
                break;
            case IntGtECode:
                result.int_data = left.int_data >= right.int_data;
                break;
            case IntLtECode:
                result.int_data = left.int_data <= right.int_data;
                break;
            case BoolAndCode:
                result.int_data = left.int_data && right.int_data;
                break;
            case BoolOrCode:
                result.int_data = left.int_data || right.int_data;
                break;
            default:
                printf("Illegal instruction\n");
                return;
            }
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, result);
            break;
        }
        case BoolNotCode: {
            Constant result;
            Constant exp;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &exp);
            result.int_data = !exp.int_data;
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, result);
            break;
        }
        case GotoCode: {
            command_counter = vm->args[command_counter].int_data;
            continue;
        }
        case GotoIfCode: {
            Constant condition;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &condition);
            if (condition.int_data) {
                command_counter = vm->args[command_counter].int_data;
                continue;
            }
            break;
        }
        case PrintlnIntCode: {
            Constant data;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &data);
            printf("%d\n", data.int_data);
            break;
        }
        case PrintlnBoolCode: {
            Constant data;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &data);
            switch (data.int_data) {
            case 0:
                printf("false\n");
                break;
            default:
                printf("true\n");
                break;
            }
            break;
        }
        case PrintlnStrCode: {
            Constant data;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &data);
            printf("%s\n", data.string_data);
            break;
        }
        default:
            printf("Illegal instruction at %d\n", command_counter);
            break;
        }
        command_counter++;
    }
}
