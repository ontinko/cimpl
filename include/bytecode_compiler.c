#include "bytecode_compiler.h"
#include "ast.h"
#include "bytecode.h"
#include "token.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compile_cache_init(CompileCache *cache) {
    cache->source = NULL;
    cache->args = NULL;
    cache->commands = NULL;
    cache->memory = NULL;
    cache->scope_start_positions = NULL;
    cache->function_param_count = 0;
    cache->has_error = 0;
    cache->stack_index = -1;
    cache->memory_size = 0;
    cache->memory_capacity = 0;
    cache->program_size = 0;
}

void var_positions_init(VarPositions *table) {
    table->size = 0;
    table->positions = NULL;
    table->var_names = NULL;
}

void var_positions_destroy(VarPositions *table) {
    free(table->var_names);
    free(table->positions);
}

int hash(char *key) {
    int length = strlen(key);
    int sum = 0;
    for (int i = 0; i < length; i++) {
        sum += (i + 1) * key[i];
    }
    return sum % 512;
}

void var_positions_set(VarPositions *table, char *var_name, int position) {
    int index = hash(var_name);
    for (int i = 0; i < table->size; i++) {
        if (table->var_names[i][index] != NULL && strcmp(table->var_names[i][index], var_name)) {
            continue;
        }
        table->var_names[i][index] = var_name;
        table->positions[i][index] = position;
        return;
    }
    table->size++;
    char *(*new_var_names)[512] = realloc(table->var_names, table->size * sizeof(char *[512]));

    int(*new_positions)[512] = realloc(table->positions, table->size * sizeof(int[512]));
    for (int i = 0; i < 512; i++) {
        new_var_names[table->size - 1][i] = NULL;
    }
    new_var_names[table->size - 1][index] = var_name;
    new_positions[table->size - 1][index] = position;
    table->var_names = new_var_names;
    table->positions = new_positions;
}

int var_positions_get(VarPositions *table, char *var_name, int *position) {
    int index = hash(var_name);
    for (int i = 0; i < table->size; i++) {
        if (strcmp(table->var_names[i][index], var_name)) {
            continue;
        }
        *position = table->positions[i][index];
        return 0;
    }
    return 1;
}

void memory_store(VarPositions *memory, int memory_size, char *var_name, int var_scope, int position) {
    if (var_scope == -1) {
        var_scope = memory_size - 1;
    }
    VarPositions *scope = &(memory[var_scope]);
    var_positions_set(scope, var_name, position);
}

void memory_load(VarPositions *memory, int memory_size, char *var_name, int var_scope, int *position) {
    if (var_scope == -1) {
        for (int i = memory_size - 1; i >= 0; i--) {
            VarPositions *scope = &(memory[i]);
            int err = var_positions_get(scope, var_name, position);
            if (err) {
                continue;
            }
            break;
        }
        return;
    }
    VarPositions *scope = &(memory[var_scope]);
    var_positions_get(scope, var_name, position);
}

void memory_extend(CompileCache *cache) {
    if (cache->memory_capacity <= cache->memory_size) {
        cache->memory_capacity += 32;
        VarPositions *new_memory = realloc(cache->memory, cache->memory_capacity * sizeof(VarPositions));
        int *new_scope_starts = realloc(cache->scope_start_positions, cache->memory_capacity * sizeof(int));
        cache->memory = new_memory;
        cache->scope_start_positions = new_scope_starts;
    }
    VarPositions new_scope;
    var_positions_init(&new_scope);
    cache->memory[cache->memory_size] = new_scope;
    cache->scope_start_positions[cache->memory_size] = cache->stack_index;
    cache->memory_size++;
}

void memory_shrink(CompileCache *cache) {
    cache->memory_size--;
    cache->stack_index = cache->scope_start_positions[cache->memory_size];
    var_positions_destroy(&cache->memory[cache->memory_size]);
}

static void add_command(CompileCache *cache, OpCode command) {
    cache->program_size++;
    OpCode *new_commands = realloc(cache->commands, cache->program_size * sizeof(OpCode));
    new_commands[cache->program_size - 1] = command;
    cache->commands = new_commands;
}

static void add_constant(CompileCache *cache, Constant constant) {
    Constant *new_args = realloc(cache->args, cache->program_size * sizeof(Constant));
    new_args[cache->program_size - 1] = constant;
    cache->args = new_args;
}

static void compile_call(Call *call, CompileCache *cache) {
    int fn_def_index;
    char *fn_name = substring(cache->source, call->call_name->start, call->call_name->end);
    memory_load(cache->memory, cache->memory_size, fn_name, call->scope, &fn_def_index);
    free(fn_name);

    for (int i = 0; i < call->args_size; i++) {
        compile_expression(call->args + i, cache);
    }

    Constant call_index = {.int_data = fn_def_index};
    add_command(cache, CallCode);
    add_constant(cache, call_index);
    if (call->datatype->type != Simple || call->datatype->data.simple_datatype != Void) {
        cache->stack_index++;
    }
    cache->stack_index -= call->args_size;
}

static void compile_expression(Expression *exp, CompileCache *cache) {
    if (exp->type == FnCallExp) {
        compile_call(exp->data.fn_call, cache);
        return;
    }

    // Temporary, for now cannot assign functions
    if (exp->data.exp->datatype->type != Simple) {
        printf("Illegal expression type\n");
        cache->has_error = 1;
        return;
    }

    OpExpression *op_exp = exp->data.exp;
    switch (op_exp->token->ttype) {
    case Number: {
        char *str_value = substring(cache->source, op_exp->token->start, op_exp->token->end);
        int int_value = atoi(str_value);
        Constant constant = {.int_data = int_value};
        add_command(cache, PushCode);
        add_constant(cache, constant);
        cache->stack_index++;
        free(str_value);
        break;
    }
    case Text: {
        char *value = substring(cache->source, op_exp->token->start, op_exp->token->end);
        Constant constant = {.string_data = value};
        add_command(cache, PushCode);
        add_constant(cache, constant);
        cache->stack_index++;
        break;
    }
    case True:
    case False: {
        int bool_value = op_exp->token->ttype == True;
        Constant constant = {.int_data = bool_value};
        add_command(cache, PushCode);
        add_constant(cache, constant);
        cache->stack_index++;
        break;
    }
    case Not: {
        compile_expression(op_exp->left, cache);
        if (cache->has_error) {
            return;
        }
        add_command(cache, BoolNotCode);
        break;
    }
    case Identifier: {
        int var_position;
        char *var_name = substring(cache->source, op_exp->token->start, op_exp->token->end);
        memory_load(cache->memory, cache->memory_size, var_name, op_exp->scope, &var_position);
        Constant constant = {.int_data = cache->stack_index - var_position};
        add_command(cache, LoadCode);
        add_constant(cache, constant);
        cache->stack_index++;
        free(var_name);
        break;
    }
    case Plus:
    case Minus:
    case Star:
    case Slash:
    case Mod:
    case EqEq:
    case NotEq:
    case And:
    case Or:
    case Lt:
    case Gt:
    case LtE:
    case GtE: {
        compile_expression(op_exp->left, cache);
        if (cache->has_error) {
            return;
        }
        compile_expression(op_exp->right, cache);
        if (cache->has_error) {
            return;
        }
        OpCode command;
        switch (op_exp->token->ttype) {
        case Plus:
            command = IntAddCode;
            break;
        case Minus:
            command = IntSubtractCode;
            break;
        case Star:
            command = IntMultiplyCode;
            break;
        case Slash:
            command = IntDivideCode;
            break;
        case Mod:
            command = IntModCode;
            break;
        case EqEq:
            command = IntEqCode;
            break;
        case NotEq:
            command = IntNotEqCode;
            break;
        case Lt:
            command = IntLtCode;
            break;
        case Gt:
            command = IntGtCode;
            break;
        case LtE:
            command = IntLtECode;
            break;
        case GtE:
            command = IntGtECode;
            break;
        case And:
            command = BoolAndCode;
            break;
        case Or:
            command = BoolOrCode;
            break;
        default:
            printf("Illegal binary operator\n");
            return;
        }
        add_command(cache, command);
        cache->stack_index--;
        break;
    }
    default: {
        printf("Illegal operator in expression\n");
        cache->has_error = 1;
        return;
    }
    }
}

static void compile_oneliner(Oneliner *oneliner, CompileCache *cache) {
    switch (oneliner->type) {
    case PrintlnOL: {
        compile_expression(oneliner->data.println->exp, cache);
        DataType simple_dt;
        switch (oneliner->data.println->exp->type) {
        case ExpExp:
            simple_dt = oneliner->data.println->exp->data.exp->datatype->data.simple_datatype;
            break;
        case FnCallExp:
            simple_dt = oneliner->data.println->exp->data.fn_call->datatype->data.simple_datatype;
            break;
        }
        switch (simple_dt) {
        case Int:
            add_command(cache, PrintlnIntCode);
            cache->stack_index--;
            break;
        case Bool:
            add_command(cache, PrintlnBoolCode);
            cache->stack_index--;
            break;
        case String:
            add_command(cache, PrintlnStrCode);
            cache->stack_index--;
            break;
        default:
            printf("Invalid type for println\n");
            cache->has_error = 1;
            break;
        }
        break;
    }
    case AssignmentOL: {
        Assignment *ass = oneliner->data.assignment;
        if (ass->exp != NULL) {
        }
        OpCode command;
        switch (ass->op->ttype) {
        case ColEq:
        case Eq: {
            compile_expression(ass->exp, cache);
            if (cache->has_error) {
                return;
            }
            break;
        }
        case PlusEq:
        case MinusEq:
        case StarEq:
        case SlashEq:
        case ModEq:
        case Inc:
        case Dec: {
            int var_position;
            char *var_name = substring(cache->source, ass->var->start, ass->var->end);
            memory_load(cache->memory, cache->memory_size, var_name, ass->scope, &var_position);
            Constant offset = {.int_data = cache->stack_index - var_position};
            add_command(cache, LoadCode);
            add_constant(cache, offset);
            cache->stack_index++;
            free(var_name);

            switch (ass->op->ttype) {
            case Inc:
            case Dec: {
                Constant exp_value = {.int_data = 1};
                add_command(cache, PushCode);
                add_constant(cache, exp_value);
                cache->stack_index++;
                break;
            }
            default:
                compile_expression(ass->exp, cache);
                break;
            }

            OpCode command;
            switch (ass->op->ttype) {
            case Inc:
            case PlusEq:
                command = IntAddCode;
                break;
            case Dec:
            case MinusEq:
                command = IntSubtractCode;
                break;
            case StarEq:
                command = IntMultiplyCode;
                break;
            case SlashEq:
                command = IntDivideCode;
                break;
            default:
                command = IntModCode;
                break;
            }
            add_command(cache, command);
            cache->stack_index--;
            break;
        }
        default:
            printf("Illegal assignment operator\n");
            return;
        }
        Constant offset;
        char *var_name = substring(cache->source, ass->var->start, ass->var->end);
        if (ass->new_var) {
            offset.int_data = 0;
        } else {
            int var_position;
            memory_load(cache->memory, cache->memory_size, var_name, ass->scope, &var_position);
            offset.int_data = cache->stack_index - var_position;
        }
        if (!ass->new_var) {
            add_command(cache, StoreCode);
            add_constant(cache, offset);
            cache->stack_index--;
            free(var_name);
        } else {
            memory_store(cache->memory, cache->memory_size, var_name, ass->scope, cache->stack_index);
        }
        break;
    }
    case CallOL: {
        Call *call = oneliner->data.call;
        compile_call(call, cache);
        break;
    }
    }
}

void compile_program(Stmt *stmts, int stmts_size, CompileCache *cache) {
    if (stmts_size == 0) {
        return;
    }
    memory_extend(cache);
    compile_to_bytecode(stmts, stmts_size, 0, cache);
    add_command(cache, EndCode);
    memory_shrink(cache);
}

void compile_to_bytecode(Stmt *stmts, int stmts_size, int does_wrap, CompileCache *cache) {
    if (stmts_size == 0) {
        return;
    }
    if (does_wrap) {
        memory_extend(cache);
    }
    for (int stmt_i = 0; stmt_i < stmts_size; stmt_i++) {
        Stmt *stmt = &stmts[stmt_i];
        switch (stmt->type) {
        case OnelinerStmt: {
            Oneliner *oneliner = stmt->data.oneliner;
            compile_oneliner(oneliner, cache);
            break;
        }
        case OpenScopeStmt:
            memory_extend(cache);
            break;
        case CloseScopeStmt: {
            Constant shift_to = {.int_data = cache->scope_start_positions[cache->memory_size - 1]};
            add_command(cache, ShiftStackCode);
            add_constant(cache, shift_to);
            memory_shrink(cache);
            break;
        }
        case ConditionalStmt: {
            Conditional *conditional = stmt->data.conditional;
            Expression *condition = conditional->condition;
            int start_index = cache->program_size;
            compile_expression(condition, cache);
            if (conditional->token->ttype == If) {
                Constant goto_then_arg = {.int_data = cache->program_size + 2};
                add_command(cache, GotoIfCode);
                add_constant(cache, goto_then_arg);
                cache->stack_index--;
                Constant goto_else_arg = {.int_data = -1};
                int goto_else_arg_index = cache->program_size;
                add_command(cache, GotoCode);
                add_constant(cache, goto_else_arg);
                if (conditional->then_size) {
                    compile_to_bytecode(conditional->then_block, conditional->then_size, 1, cache);
                }
                Constant goto_end_arg = {.int_data = -1};
                int goto_end_arg_index = cache->program_size;
                add_command(cache, GotoCode);
                add_constant(cache, goto_end_arg);
                (cache->args)[goto_else_arg_index].int_data = cache->program_size;
                if (conditional->else_size) {
                    compile_to_bytecode(conditional->else_block, conditional->else_size, 1, cache);
                }
                (cache->args)[goto_end_arg_index].int_data = cache->program_size;
            } else {
                Constant goto_then_arg = {.int_data = -1};
                int goto_then_arg_index = cache->program_size;
                add_command(cache, GotoIfCode);
                add_constant(cache, goto_then_arg);
                cache->stack_index--;
                Constant goto_else_arg = {.int_data = -1};
                int goto_else_arg_index = cache->program_size;
                add_command(cache, GotoCode);
                add_constant(cache, goto_else_arg);
                int start_command_index = cache->program_size;
                compile_expression(condition, cache);
                goto_then_arg.int_data = cache->program_size + 2;
                add_command(cache, GotoIfCode);
                add_constant(cache, goto_then_arg);
                cache->stack_index--;
                Constant goto_end_arg = {.int_data = -1};
                int goto_end_arg_index = cache->program_size;
                add_command(cache, GotoCode);
                add_constant(cache, goto_end_arg);
                (cache->args)[goto_then_arg_index].int_data = cache->program_size;
                if (conditional->then_size) {
                    compile_to_bytecode(conditional->then_block, conditional->then_size, 1, cache);
                }
                Constant goto_start_arg = {.int_data = start_command_index};
                add_command(cache, GotoCode);
                add_constant(cache, goto_start_arg);
                (cache->args)[goto_else_arg_index].int_data = cache->program_size;
                if (conditional->else_size) {
                    compile_to_bytecode(conditional->else_block, conditional->else_size, 1, cache);
                }
                (cache->args)[goto_end_arg_index].int_data = cache->program_size;
            }
            break;
        }
        case ForStmt: {
            ForLoop *for_loop = stmt->data.for_loop;
            Oneliner *init = for_loop->init;
            Expression *condition = for_loop->condition;
            Oneliner *after = for_loop->after;
            Stmt *body = for_loop->body;
            int body_size = for_loop->body_size;

            memory_extend(cache);
            compile_oneliner(init, cache);
            int start_command_index = cache->program_size;
            compile_expression(condition, cache);
            Constant goto_then_arg = {.int_data = cache->program_size + 2};
            add_command(cache, GotoIfCode);
            add_constant(cache, goto_then_arg);
            cache->stack_index--;
            Constant goto_end_arg = {.int_data = -1};
            int goto_end_arg_index = cache->program_size;
            add_command(cache, GotoCode);
            add_constant(cache, goto_end_arg);
            if (body_size) {
                compile_to_bytecode(body, body_size, 1, cache);
            }
            compile_oneliner(after, cache);
            Constant goto_start_arg = {.int_data = start_command_index};
            add_command(cache, GotoCode);
            add_constant(cache, goto_start_arg);

            (cache->args)[goto_end_arg_index].int_data = cache->program_size;

            Constant shift_to = {.int_data = cache->scope_start_positions[cache->memory_size - 1]};
            add_command(cache, ShiftStackCode);
            add_constant(cache, shift_to);

            memory_shrink(cache);
            break;
        }
        case FnStmt: {
            add_command(cache, GotoCode);
            Constant goto_arg = {.int_data = -1};
            int goto_arg_index = cache->program_size - 1;
            add_constant(cache, goto_arg);

            FnDefinition *fn_def = stmt->data.fn_def;
            char *fn_name = substring(cache->source, fn_def->name->start, fn_def->name->end);
            memory_store(cache->memory, cache->memory_size, fn_name, -1, cache->program_size); // storing command index, not stack index

            memory_extend(cache);
            for (int i = 0; i < fn_def->datatype->params_size; i++) {
                cache->stack_index++;
                FnParam param = fn_def->datatype->params[i];
                char *param_name = substring(cache->source, param.name->start, param.name->end);
                memory_store(cache->memory, cache->memory_size, param_name, -1, cache->stack_index);
            }
            cache->function_param_count = fn_def->datatype->params_size;
            compile_to_bytecode(fn_def->body, fn_def->body_size, 0, cache);
            memory_shrink(cache);
            Constant shift = {.int_data = fn_def->datatype->params_size};
            ;
            add_command(cache, ResumeCode);
            add_constant(cache, shift);
            (cache->args)[goto_arg_index].int_data = cache->program_size;
            break;
        }
        case ReturnStmt: {
            Constant shift = {.int_data = cache->function_param_count};
            ReturnCmd *cmd = stmt->data.return_cmd;
            if (cmd->exp == NULL) {
                add_command(cache, ResumeCode);
                add_constant(cache, shift);
                break;
            }
            compile_expression(cmd->exp, cache);
            add_command(cache, ReturnCode);
            add_constant(cache, shift);
            break;
        }
        default:
            printf("Illegal statement\n");
            return;
        }
    }
    if (does_wrap) {
        Constant shift_to = {.int_data = cache->scope_start_positions[cache->memory_size - 1]};
        add_command(cache, ShiftStackCode);
        add_constant(cache, shift_to);
        memory_shrink(cache);
    }
}
