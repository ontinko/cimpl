#include "bytecode_compiler.h"
#include "ast.h"
#include "bytecode.h"
#include "token.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void add_command(OpCode **commands, size_t *commands_size, OpCode command) {
    (*commands_size)++;
    OpCode *new_commands = realloc(*commands, *commands_size * sizeof(OpCode));
    new_commands[*commands_size - 1] = command;
    *commands = new_commands;
}

static void add_constant(Constant **args, size_t *args_size, Constant constant) {
    (*args_size)++;
    Constant *new_args = realloc(*args, *args_size * sizeof(Constant));
    new_args[*args_size - 1] = constant;
    *args = new_args;
}

static void add_ref_scopes(int **ref_scopes, size_t *ref_scopes_size, int scope) {
    (*ref_scopes_size)++;
    int *new_ref_scopes = realloc(*ref_scopes, *ref_scopes_size * sizeof(int));
    new_ref_scopes[*ref_scopes_size - 1] = scope;
    *ref_scopes = new_ref_scopes;
}

static void compile_expression(char *source, Expression *exp, OpCode **commands, size_t *commands_size, Constant **args, size_t *args_size,
                               int **ref_scopes, size_t *ref_scopes_size, int *has_error) {
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
        add_command(commands, commands_size, PushCode);
        add_constant(args, args_size, constant);
        printf("Added number\n");
        free(str_value);
        break;
    }
    case True:
    case False: {
        uint8_t bool_value = op_exp->token->ttype == True;
        Constant constant = {.bool_data = bool_value};
        add_command(commands, commands_size, PushCode);
        add_constant(args, args_size, constant);
        printf("Added bool\n");
        break;
    }
    case Not: {
        compile_expression(source, op_exp->left, commands, commands_size, args, args_size, ref_scopes, ref_scopes_size, has_error);
        if (*has_error) {
            return;
        }
        add_command(commands, commands_size, BoolNotCode);
        printf("Added bool\n");
        break;
    }
    case Identifier: {
        add_command(commands, commands_size, LoadCode);
        Constant constant = {.var_name = substring(source, op_exp->token->start, op_exp->token->end)};
        add_constant(args, args_size, constant);
        add_ref_scopes(ref_scopes, ref_scopes_size, op_exp->scope);
        printf("Added id\n");
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
        compile_expression(source, op_exp->left, commands, commands_size, args, args_size, ref_scopes, ref_scopes_size, has_error);
        if (*has_error) {
            return;
        }
        compile_expression(source, op_exp->right, commands, commands_size, args, args_size, ref_scopes, ref_scopes_size, has_error);
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
        add_command(commands, commands_size, command);
        printf("Added binary operator\n");
        break;
    }
    default: {
        printf("Illegal operator in expression\n");
        *has_error = 1;
        return;
    }
    }
}

void compile_to_bytecode(Stmt **stmts, size_t stmts_size, char *source, OpCode **commands, size_t *commands_size, Constant **args, size_t *args_size,
                         int **ref_scopes, size_t *ref_scopes_size, int *has_error) {
    printf("\nCompiling %lu statements to bytecode\n", stmts_size);
    if (stmts_size == 0) {
        return;
    }
    add_command(commands, commands_size, CreateScopeCode);
    for (int stmt_i = 0; stmt_i < stmts_size; stmt_i++) {
        Stmt *stmt = stmts[stmt_i];
        switch (stmt->type) {
        case OnelinerStmt: {
            Oneliner *oneliner = stmt->data.oneliner;
            switch (oneliner->type) {
            case AssignmentOL: {
                Assignment *ass = stmt->data.oneliner->data.assignment;
                compile_expression(source, ass->exp, commands, commands_size, args, args_size, ref_scopes, ref_scopes_size, has_error);
                if (*has_error) {
                    return;
                }
                OpCode command;
                switch (ass->op->ttype) {
                case ColEq:
                case Eq: {
                    add_command(commands, commands_size, StoreCode);
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
                    add_command(commands, commands_size, LoadCode);
                    add_constant(args, args_size, var_name);

                    switch (ass->op->ttype) {
                    case Inc:
                    case Dec: {
                        Constant exp_value = {.int_data = 1};
                        add_command(commands, commands_size, PushCode);
                        add_constant(args, args_size, exp_value);
                        break;
                    }
                    default:
                        compile_expression(source, ass->exp, commands, commands_size, args, args_size, ref_scopes, ref_scopes_size, has_error);
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
                    add_command(commands, commands_size, command);
                    add_command(commands, commands_size, StoreCode);
                    break;
                }
                default:
                    printf("Illegal assignment operator\n");
                    return;
                }
                Constant var_name = {.var_name = substring(source, ass->var->start, ass->var->end)};
                add_constant(args, args_size, var_name);
                add_ref_scopes(ref_scopes, ref_scopes_size, ass->scope);
                break;
            }
            default:
                printf("Illegal oneliner\n");
                return;
            }
            break;
        }
        case OpenScopeStmt:
            add_command(commands, commands_size, CreateScopeCode);
            break;
        case CloseScopeStmt:
            add_command(commands, commands_size, DestroyScopeCode);
            break;
        default:
            printf("Illegal statement\n");
            return;
        }
    }
    add_command(commands, commands_size, DestroyScopeCode);
}
