#include "vm.h"
#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>

static void stack_resize(Constant **stack, int stack_size, int *stack_capacity) {
    if (*stack_capacity <= stack_size) {
        *stack_capacity *= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    } else if (*stack_capacity > 1024 && stack_size <= *stack_capacity / 3) {
        *stack_capacity /= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    }
}

static void stack_push(Constant **stack, int *stack_size, int *stack_capacity, Constant constant) {
    (*stack_size)++;
    stack_resize(stack, *stack_size, stack_capacity);
    (*stack)[*stack_size - 1] = constant;
}

static void stack_pop(Constant **stack, int *stack_size, int *stack_capacity, Constant *constant) {
    (*stack_size)--;
    *constant = (*stack)[*stack_size];
    stack_resize(stack, *stack_size, stack_capacity);
}

static void vm_calls_resize(VM *vm) {
    if (vm->fn_calls_capacity <= vm->fn_calls_size) {
        vm->fn_calls_capacity *= 2;
        int *new_command_stack = realloc(vm->command_return_points, vm->fn_calls_capacity * sizeof(int));
        int *new_constant_stack = realloc(vm->stack_return_points, vm->fn_calls_capacity * sizeof(int));
        vm->command_return_points = new_command_stack;
        vm->stack_return_points = new_constant_stack;
    } else if (vm->fn_calls_capacity > 256 && vm->fn_calls_size <= vm->fn_calls_capacity / 3) {
        vm->fn_calls_capacity /= 2;
        int *new_command_stack = realloc(vm->command_return_points, vm->fn_calls_capacity * sizeof(int));
        int *new_constant_stack = realloc(vm->stack_return_points, vm->fn_calls_capacity * sizeof(int));
        vm->command_return_points = new_command_stack;
        vm->stack_return_points = new_constant_stack;
    }
}

static void vm_calls_push(VM *vm, int stack_index, int command_index) {
    vm->fn_calls_size++;
    vm_calls_resize(vm);
    vm->stack_return_points[vm->fn_calls_size - 1] = stack_index;
    vm->command_return_points[vm->fn_calls_size - 1] = command_index;
}

static void vm_calls_pop(VM *vm, int *stack_index, int *command_index) {
    vm->fn_calls_size--;
    *stack_index = vm->stack_return_points[vm->fn_calls_size];
    *command_index = vm->command_return_points[vm->fn_calls_size];
    vm_calls_resize(vm);
}

void vm_init(VM *vm, OpCode *commands, Constant *args, size_t program_size) {
    vm->program_size = program_size;
    vm->commands = commands;
    vm->args = args;
    vm->stack_size = 0;
    vm->stack_capacity = 128;
    vm->fn_calls_size = 0;
    vm->fn_calls_capacity = 32;
    Constant *stack = malloc(sizeof(Constant) * vm->stack_capacity);
    int *command_return_points = malloc(sizeof(int) * vm->fn_calls_capacity);
    int *stack_return_points = malloc(sizeof(int) * vm->fn_calls_capacity);
    vm->stack = stack;
    vm->command_return_points = command_return_points;
    vm->stack_return_points = stack_return_points;
}

void vm_run(VM *vm) {
    int command_counter = 0;
    while (command_counter < vm->program_size) {
        OpCode command = vm->commands[command_counter];
        switch (command) {
        case ShiftStackCode: {
            int position = vm->args[command_counter].int_data;
            vm->stack_size = position;
            break;
        }
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
        case CallCode: {
            int resume_stack_index = vm->stack_size - 1;
            int resume_command_index = command_counter + 1;
            vm_calls_push(vm, resume_stack_index, resume_command_index);
            int call_to = vm->args[command_counter].int_data;
            command_counter = call_to;
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
        case ResumeCode: {
            int stack_position;
            int command_position;
            int shift = vm->args[command_counter].int_data;
            vm_calls_pop(vm, &stack_position, &command_position);
            command_counter = command_position;
            vm->stack_size = stack_position + 1 - shift;
            continue;
        }
        case ReturnCode: {
            Constant value;
            int shift = vm->args[command_counter].int_data;
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &value);
            int stack_position;
            int command_position;
            vm_calls_pop(vm, &stack_position, &command_position);
            command_counter = command_position;
            vm->stack_size = stack_position + 1 - shift;
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, value);
            continue;
        }
        case EndCode: {
            vm->stack_size = 0;
            vm->stack_capacity = 0;
            vm->program_size = 0;
            vm->fn_calls_size = 0;
            vm->fn_calls_capacity = 0;
            // printf("Freeing %p\n", vm->stack);
            // free(vm->stack);
            // printf("Works 2\n");
            // free(vm->commands);
            // printf("Works 3\n");
            // free(vm->args);
            // printf("Works 4\n");
            // free(vm->stack_return_points);
            // printf("Works 5\n");
            // free(vm->command_return_points);
            // printf("Works 6\n");
            break;
        }
        default:
            printf("Illegal instruction at %d\n", command_counter);
            break;
        }
        command_counter++;
    }
}
