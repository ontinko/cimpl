#include "vm.h"
#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hash(char *key) {
    size_t length = strlen(key);
    size_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += (i + 1) * key[i];
    }
    return sum % 512;
}

static void scope_init(Scope *scope) {
    scope->size = 0;
    scope->const_names = NULL;
    scope->const_values = NULL;
}

static void scope_destroy(Scope *scope) {
    if (scope->const_names != NULL) {
        for (int i = 0; i < scope->size; i++) {
            for (int j = 0; j < 512; j++) {
                free(scope->const_names[i][j]);
            }
            free(scope->const_values[i]);
        }
    }
}

static void scope_set(Scope *scope, char *const_name, Constant const_value) {
    int index = hash(const_name);
    for (int i = 0; i < scope->size; i++) {
        if (scope->const_names[i][index] != NULL && strcmp(scope->const_names[i][index], const_name)) {
            continue;
        }
        scope->const_names[i][index] = const_name;
        scope->const_values[i][index] = const_value;
        return;
    }
    scope->size++;
    char *(*new_const_names)[512] = realloc(scope->const_names, scope->size * sizeof(char *[512]));
    Constant(*new_const_values)[512] = realloc(scope->const_values, scope->size * sizeof(Constant *[512]));
    for (int i = 0; i < 512; i++) {
        new_const_names[scope->size - 1][i] = NULL;
    }
    new_const_names[scope->size - 1][index] = const_name;
    new_const_values[scope->size - 1][index] = const_value;
    scope->const_names = new_const_names;
    scope->const_values = new_const_values;
}

static void scope_get(Scope *scope, char *const_name, Constant *const_value) {
    int index = hash(const_name);
    for (int i = 0; i < scope->size; i++) {
        if (strcmp(scope->const_names[i][index], const_name)) {
            continue;
        }
        *const_value = scope->const_values[i][index];
        return;
    }
}

static void memory_extend(Scope **memory, size_t *memory_size, int *memory_capacity) {
    if (*memory_capacity <= *memory_size) {
        *memory_capacity += 32;
        Scope *new_memory = realloc(*memory, *memory_capacity * sizeof(Scope));
        *memory = new_memory;
    }
    Scope new_scope;
    scope_init(&new_scope);
    (*memory)[*memory_size] = new_scope;
    (*memory_size)++;
}

static void memory_shrink(Scope **memory, size_t *memory_size) {
    (*memory_size)--;
    scope_destroy(&(*memory)[*memory_size]);
}

static void memory_store(Scope *memory, size_t memory_size, char *const_name, int const_scope, Constant constant) {
    if (const_scope == -1) {
        const_scope = memory_size - 1;
    }
    Scope *scope = &(memory[const_scope]);
    scope_set(scope, const_name, constant);
}

static void memory_load(Scope *memory, size_t memory_size, char *const_name, int const_scope, Constant *constant) {
    if (const_scope == -1) {
        const_scope = memory_size - 1;
    }
    Scope *scope = &(memory[const_scope]);
    scope_get(scope, const_name, constant);
}

void memory_visualize(Scope *memory, size_t memory_size) {
    printf("\nState of the memory:\n");
    for (int i = 0; i < memory_size; i++) {
        printf("SCOPE %d\n", i);
        Scope scope = memory[i];
        if (scope.const_names == NULL) {
            continue;
        }
        for (int j = 0; j < scope.size; j++) {
            for (int k = 0; k < 512; k++) {
                if (scope.const_names[j][k] == NULL) {
                    continue;
                }
                Constant val = scope.const_values[j][k];
                printf("%s = %d\n", scope.const_names[j][k], val.int_data);
            }
        }
    }
}

static void stack_push(Constant **stack, size_t *stack_size, int *stack_capacity, Constant constant) {
    if (*stack_capacity <= *stack_size) {
        *stack_capacity *= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    }
    (*stack)[*stack_size] = constant;
    (*stack_size)++;
}

static void stack_pop(Constant **stack, size_t *stack_size, int *stack_capacity, Constant *constant) {
    if (*stack_capacity > 1024 && *stack_size <= *stack_capacity / 3) {
        *stack_capacity /= 2;
        Constant *new_stack = realloc(*stack, *stack_capacity * sizeof(Constant));
        *stack = new_stack;
    }
    (*stack_size)--;
    *constant = (*stack)[*stack_size];
}

void vm_init(VM *vm, OpCode *commands, Constant *args, int *ref_scopes, size_t program_size) {
    vm->memory = NULL;
    vm->memory_size = 0;
    vm->memory_capacity = 0;
    vm->program_size = program_size;
    vm->commands = commands;
    vm->args = args;
    vm->ref_scopes = ref_scopes;
    vm->stack_size = 0;
    vm->stack_capacity = 128;
    Constant *stack = malloc(sizeof(Constant) * vm->stack_capacity);
    vm->stack = stack;
}

void vm_run(VM *vm) {
    int command_counter = 0;
    while (command_counter < vm->program_size) {
        OpCode command = vm->commands[command_counter];
        switch (command) {
        case CreateScopeCode:
            memory_extend(&vm->memory, &vm->memory_size, &vm->memory_capacity);
            break;
        case DestroyScopeCode:
            memory_shrink(&vm->memory, &vm->memory_size);
            break;
        case PushCode:
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, vm->args[command_counter]);
            break;
        case LoadCode: {
            Constant constant;
            char *const_name = vm->args[command_counter].var_name;
            int const_scope = vm->ref_scopes[command_counter];
            memory_load(vm->memory, vm->memory_size, const_name, const_scope, &constant);
            stack_push(&vm->stack, &vm->stack_size, &vm->stack_capacity, constant);
            break;
        }
        case StoreCode: {
            Constant constant;
            char *const_name = vm->args[command_counter].var_name;
            int const_scope = vm->ref_scopes[command_counter];
            stack_pop(&vm->stack, &vm->stack_size, &vm->stack_capacity, &constant);
            memory_store(vm->memory, vm->memory_size, const_name, const_scope, constant);
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
                result.bool_data = left.int_data == right.int_data && command == IntEqCode;
                break;
            case IntGtCode:
                result.bool_data = left.int_data > right.int_data;
                break;
            case IntLtCode:
                result.bool_data = left.int_data < right.int_data;
                break;
            case IntGtECode:
                result.bool_data = left.int_data >= right.int_data;
                break;
            case IntLtECode:
                result.bool_data = left.int_data <= right.int_data;
                break;
            case BoolAndCode:
                result.bool_data = left.bool_data && right.bool_data;
                break;
            case BoolOrCode:
                result.bool_data = left.bool_data || right.bool_data;
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
            result.bool_data = !exp.bool_data;
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
            if (condition.bool_data) {
                command_counter = vm->args[command_counter].int_data;
                continue;
            }
            break;
        }
        case ReturnCode:
        default:
            printf("Illegal instruction at %d\n", command_counter);
            break;
        }
        command_counter++;
    }
}
