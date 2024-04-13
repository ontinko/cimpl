#include "bytecode_compiler.h"
#include "ast.h"
#include "bytecode.h"
#include "token.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void add_command(OpCode **commands, size_t *program_size, OpCode command) {
    (*program_size)++;
    OpCode *new_commands = realloc(*commands, *program_size * sizeof(OpCode));
    new_commands[*program_size - 1] = command;
    *commands = new_commands;
}

static void add_constant(Constant **args, size_t *program_size, Constant constant) {
    Constant *new_args = realloc(*args, *program_size * sizeof(Constant));
    new_args[*program_size - 1] = constant;
    *args = new_args;
}

static void add_ref_scopes(int **ref_scopes, size_t *program_size, int scope) {
    int *new_ref_scopes = realloc(*ref_scopes, *program_size * sizeof(int));
    new_ref_scopes[*program_size - 1] = scope;
    *ref_scopes = new_ref_scopes;
}

static void compile_expression(char *source, Expression *exp, OpCode **commands, Constant **args, int **ref_scopes, size_t *program_size,
                               int *has_error) {
    // Temporary, for now cannot call and assign functions
    if (exp->type != ExpExp || exp->data.exp->datatype->type != Simple) {
        printf("Illegal expression type\n");
        *has_error = 1;
        return;
    }
    OpExpression *op_exp = exp->data.exp;
    switch (op_exp->token->ttype) {
    case Number: {
        char *str_value = substring(source, op_exp->token->start, op_exp->token->end);
        int int_value = atoi(str_value);
        Constant constant = {.int_data = int_value};
        add_command(commands, program_size, PushCode);
        add_constant(args, program_size, constant);
        free(str_value);
        break;
    }
    case True:
    case False: {
        uint8_t bool_value = op_exp->token->ttype == True;
        Constant constant = {.bool_data = bool_value};
        add_command(commands, program_size, PushCode);
        add_constant(args, program_size, constant);
        break;
    }
    case Not: {
        compile_expression(source, op_exp->left, commands, args, ref_scopes, program_size, has_error);
        if (*has_error) {
            return;
        }
        add_command(commands, program_size, BoolNotCode);
        break;
    }
    case Identifier: {
        add_command(commands, program_size, LoadCode);
        Constant constant = {.var_name = substring(source, op_exp->token->start, op_exp->token->end)};
        add_constant(args, program_size, constant);
        add_ref_scopes(ref_scopes, program_size, op_exp->scope);
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
        compile_expression(source, op_exp->left, commands, args, ref_scopes, program_size, has_error);
        if (*has_error) {
            return;
        }
        compile_expression(source, op_exp->right, commands, args, ref_scopes, program_size, has_error);
        if (*has_error) {
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
        add_command(commands, program_size, command);
        break;
    }
    default: {
        printf("Illegal operator in expression\n");
        *has_error = 1;
        return;
    }
    }
}

static void compile_oneliner(char *source, Oneliner *oneliner, OpCode **commands, Constant **args, int **ref_scopes, size_t *program_size,
                             int *has_error) {
    switch (oneliner->type) {
    case AssignmentOL: {
        Assignment *ass = oneliner->data.assignment;
        if (ass->exp != NULL) {
            compile_expression(source, ass->exp, commands, args, ref_scopes, program_size, has_error);
            if (*has_error) {
                return;
            }
        }
        OpCode command;
        switch (ass->op->ttype) {
        case ColEq:
        case Eq: {
            break;
        }
        case PlusEq:
        case MinusEq:
        case StarEq:
        case SlashEq:
        case ModEq:
        case Inc:
        case Dec: {
            Constant var_name = {.var_name = substring(source, ass->var->start, ass->var->end)};
            add_command(commands, program_size, LoadCode);
            add_constant(args, program_size, var_name);
            add_ref_scopes(ref_scopes, program_size, ass->scope);

            switch (ass->op->ttype) {
            case Inc:
            case Dec: {
                Constant exp_value = {.int_data = 1};
                add_command(commands, program_size, PushCode);
                add_constant(args, program_size, exp_value);
                break;
            }
            default:
                compile_expression(source, ass->exp, commands, args, ref_scopes, program_size, has_error);
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
            add_command(commands, program_size, command);
            break;
        }
        default:
            printf("Illegal assignment operator\n");
            return;
        }
        char *var_name_string = substring(source, ass->var->start, ass->var->end);
        Constant var_name = {.var_name = var_name_string};
        add_command(commands, program_size, StoreCode);
        add_constant(args, program_size, var_name);
        add_ref_scopes(ref_scopes, program_size, ass->scope);
        break;
    }
    default:
        printf("Illegal oneliner\n");
        return;
    }
}

void compile_to_bytecode(Stmt **stmts, size_t stmts_size, char *source, OpCode **commands, Constant **args, int **ref_scopes, size_t *program_size,
                         int *has_error) {
    if (stmts_size == 0) {
        return;
    }
    add_command(commands, program_size, CreateScopeCode);
    for (int stmt_i = 0; stmt_i < stmts_size; stmt_i++) {
        Stmt *stmt = stmts[stmt_i];
        switch (stmt->type) {
        case OnelinerStmt: {
            Oneliner *oneliner = stmt->data.oneliner;
            compile_oneliner(source, oneliner, commands, args, ref_scopes, program_size, has_error);
            break;
        }
        case OpenScopeStmt:
            add_command(commands, program_size, CreateScopeCode);
            break;
        case CloseScopeStmt:
            add_command(commands, program_size, DestroyScopeCode);
            break;
        case ConditionalStmt: {
            Conditional *conditional = stmt->data.conditional;
            Expression *condition = conditional->condition;
            size_t start_index = *program_size;
            compile_expression(source, condition, commands, args, ref_scopes, program_size, has_error);
            if (conditional->token->ttype == If) {
                Constant goto_then_arg = {.int_data = *program_size + 2};
                add_command(commands, program_size, GotoIfCode);
                add_constant(args, program_size, goto_then_arg);
                Constant goto_else_arg = {.int_data = -1};
                size_t goto_else_arg_index = *program_size;
                add_command(commands, program_size, GotoCode);
                add_constant(args, program_size, goto_else_arg);
                if (conditional->then_size) {
                    compile_to_bytecode(conditional->then_block, conditional->then_size, source, commands, args, ref_scopes, program_size, has_error);
                }
                Constant goto_end_arg = {.int_data = -1};
                size_t goto_end_arg_index = *program_size;
                add_command(commands, program_size, GotoCode);
                add_constant(args, program_size, goto_end_arg);
                (*args)[goto_else_arg_index].int_data = *program_size;
                if (conditional->else_size) {
                    compile_to_bytecode(conditional->else_block, conditional->else_size, source, commands, args, ref_scopes, program_size, has_error);
                }
                (*args)[goto_end_arg_index].int_data = *program_size;
            } else {
                Constant goto_then_arg = {.int_data = -1};
                size_t goto_then_arg_index = *program_size;
                add_command(commands, program_size, GotoIfCode);
                add_constant(args, program_size, goto_then_arg);
                Constant goto_else_arg = {.int_data = -1};
                size_t goto_else_arg_index = *program_size;
                add_command(commands, program_size, GotoCode);
                add_constant(args, program_size, goto_else_arg);
                size_t start_command_index = *program_size;
                compile_expression(source, condition, commands, args, ref_scopes, program_size, has_error);
                goto_then_arg.int_data = *program_size + 2;
                add_command(commands, program_size, GotoIfCode);
                add_constant(args, program_size, goto_then_arg);
                Constant goto_end_arg = {.int_data = -1};
                size_t goto_end_arg_index = *program_size;
                add_command(commands, program_size, GotoCode);
                add_constant(args, program_size, goto_end_arg);
                (*args)[goto_then_arg_index].int_data = *program_size;
                if (conditional->then_size) {
                    compile_to_bytecode(conditional->then_block, conditional->then_size, source, commands, args, ref_scopes, program_size, has_error);
                }
                Constant goto_start_arg = {.int_data = start_command_index};
                add_command(commands, program_size, GotoCode);
                add_constant(args, program_size, goto_start_arg);
                (*args)[goto_else_arg_index].int_data = *program_size;
                if (conditional->else_size) {
                    compile_to_bytecode(conditional->else_block, conditional->else_size, source, commands, args, ref_scopes, program_size, has_error);
                }
                (*args)[goto_end_arg_index].int_data = *program_size;
            }
            break;
        }
        case ForStmt: {
            ForLoop *for_loop = stmt->data.for_loop;
            Oneliner *init = for_loop->init;
            Expression *condition = for_loop->condition;
            Oneliner *after = for_loop->after;
            Stmt **body = for_loop->body;
            size_t body_size = for_loop->body_size;

            add_command(commands, program_size, CreateScopeCode);
            compile_oneliner(source, init, commands, args, ref_scopes, program_size, has_error);
            size_t start_command_index = *program_size;
            compile_expression(source, condition, commands, args, ref_scopes, program_size, has_error);
            Constant goto_then_arg = {.int_data = *program_size + 2};
            add_command(commands, program_size, GotoIfCode);
            add_constant(args, program_size, goto_then_arg);
            Constant goto_end_arg = {.int_data = -1};
            size_t goto_end_arg_index = *program_size;
            add_command(commands, program_size, GotoCode);
            add_constant(args, program_size, goto_end_arg);
            if (body_size) {
                compile_to_bytecode(body, body_size, source, commands, args, ref_scopes, program_size, has_error);
            }
            compile_oneliner(source, after, commands, args, ref_scopes, program_size, has_error);
            Constant goto_start_arg = {.int_data = start_command_index};
            add_command(commands, program_size, GotoCode);
            add_constant(args, program_size, goto_start_arg);

            (*args)[goto_end_arg_index].int_data = *program_size;
            add_command(commands, program_size, DestroyScopeCode);
            break;
        }
        default:
            printf("Illegal statement\n");
            return;
        }
    }
    add_command(commands, program_size, DestroyScopeCode);
}
