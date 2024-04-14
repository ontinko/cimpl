#include "ast.h"
#include "token.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

GenericDT *generic_datatype_create() {
    GenericDT *datatype = malloc(sizeof(GenericDT));
    return datatype;
}

void function_type_init(FunctionType *fn_type) {
    fn_type->params_size = 0;
    fn_type->params = NULL;
    fn_type->return_type = NULL;
}

void op_expression_init(OpExpression *exp) {
    exp->scope = -1;
    exp->datatype = NULL;
    exp->left = NULL;
    exp->right = NULL;
    exp->token = NULL;
}

void call_init(Call *call) {
    call->datatype = NULL;
    call->args = NULL;
    call->args_size = 0;
    call->call_name = NULL;
    call->scope = -1;
}

void assignment_init(Assignment *ass) {
    ass->exp = NULL;
    ass->var = NULL;
    ass->op = NULL;
    ass->datatype = NULL;
    ass->new_var = 0;
    ass->scope = -1;
}

void fn_param_init(FnParam *param) {
    param->name = NULL;
    param->datatype = NULL;
}

void fn_definition_init(FnDefinition *fn_def) {
    fn_def->name = NULL;
    fn_def->datatype = NULL;
    fn_def->body = NULL;
    fn_def->body_size = 0;
}

void for_loop_init(ForLoop *loop) {
    loop->token = NULL;
    loop->init = NULL;
    loop->condition = NULL;
    loop->after = NULL;
    loop->body = NULL;
    loop->body_size = 0;
}

void conditional_init(Conditional *cond) {
    cond->token = NULL;
    cond->condition = NULL;
    cond->then_block = NULL;
    cond->else_block = NULL;
    cond->then_size = 0;
    cond->else_size = 0;
}

void break_cmd_init(BreakCmd *cmd) { cmd->token = NULL; }

void continue_cmd_init(ContinueCmd *cmd) { cmd->token = NULL; }

void return_cmd_init(ReturnCmd *cmd) {
    cmd->token = NULL;
    cmd->exp = NULL;
}

void open_scope_cmd_init(OpenScopeCmd *cmd) { cmd->token = NULL; }

void close_scope_cmd_init(CloseScopeCmd *cmd) { cmd->token = NULL; }

int generic_datatype_compare(GenericDT *first, GenericDT *second) {
    if (first == NULL || second == NULL) {
        return 1;
    }
    if (first->type != second->type) {
        return 0;
    }
    switch (first->type) {
    case Simple:
        return first->data.simple_datatype == second->data.simple_datatype;
    default:
        if (!generic_datatype_compare(first->data.fn_datatype->return_type, second->data.fn_datatype->return_type)) {
            return 0;
        }
        if (first->data.fn_datatype->params_size != second->data.fn_datatype->params_size) {
            return 0;
        }
        for (int i = 0; i < first->data.fn_datatype->params_size; i++) {
            if (!generic_datatype_compare(first->data.fn_datatype->params[i].datatype, second->data.fn_datatype->params[i].datatype)) {
                return 0;
            }
        }
    }
    return 1;
}

void tab(int tab_size) {
    printf("\n");
    for (int t = 0; t < tab_size; t++) {
        printf(" ");
    }
}

void visualize_oneliner(Oneliner *oneliner, char *source) {
    switch (oneliner->type) {
    case AssignmentOL: {
        Assignment *ass = oneliner->data.assignment;
        printf("%s : ", substring(source, ass->var->start, ass->var->end));
        generic_datatype_view(ass->datatype, source);
        printf(" %s", substring(source, ass->op->start, ass->op->end));
        if (ass->op->ttype != Inc && ass->op->ttype != Dec) {
            printf(" ");
            visualize_expression(ass->exp, source);
        }
        break;
    }
    case CallOL: {
        Call *call = oneliner->data.call;
        printf("%s(", substring(source, call->call_name->start, call->call_name->end));
        for (int i = 0; i < call->args_size; i++) {
            visualize_expression(&call->args[i], source);
            if (i != call->args_size - 1) {
                printf(", ");
            }
        }
        printf(") : ");
        generic_datatype_view(call->datatype, source);
        break;
    }
    }
}

void visualize_program(Stmt *stmts, size_t stmts_size, int tab_size, char *source) {
    for (int i = 0; i < stmts_size; i++) {
        tab(tab_size);
        Stmt stmt = stmts[i];
        switch (stmt.type) {
        case OnelinerStmt: {
            Oneliner *oneliner = stmt.data.oneliner;
            visualize_oneliner(oneliner, source);
            printf(";");
            break;
        }
        case ConditionalStmt: {
            Conditional *cond = stmt.data.conditional;
            printf("%s ", substring(source, cond->token->start, cond->token->end));
            visualize_expression(cond->condition, source);
            if (cond->then_size) {
                printf(" {");
                visualize_program(cond->then_block, cond->then_size, tab_size + 4, source);
                tab(tab_size);
                printf("}");
            } else {
                printf("{}");
            }
            if (cond->else_size) {
                printf(" else {");
                visualize_program(cond->else_block, cond->else_size, tab_size + 4, source);
                tab(tab_size);
                printf("}");
            }
            break;
        }
        case ForStmt: {
            ForLoop *for_loop = stmt.data.for_loop;
            printf("for ");
            visualize_oneliner(for_loop->init, source);
            printf("; ");
            visualize_expression(for_loop->condition, source);
            printf("; ");
            visualize_oneliner(for_loop->after, source);
            if (for_loop->body_size) {
                printf(" {");
                visualize_program(for_loop->body, for_loop->body_size, tab_size + 4, source);
                tab(tab_size);
                printf("}");
            } else {
                printf(" {}");
            }
            break;
        }
        case FnStmt: {
            FnDefinition *fn = stmt.data.fn_def;
            GenericDT dt = {.type = Complex};
            dt.data.fn_datatype = fn->datatype;
            printf("%s : ", substring(source, fn->name->start, fn->name->end));
            generic_datatype_view(&dt, source);
            printf("{");
            if (fn->body_size != 0) {
                visualize_program(fn->body, fn->body_size, tab_size + 4, source);
                tab(tab_size);
            }
            printf("}");
            break;
        }
        case BreakStmt: {
            printf("break;");
            break;
        }
        case ContinueStmt:
            printf("continue;");
            break;
        case ReturnStmt: {
            ReturnCmd *cmd = stmt.data.return_cmd;
            printf("return");
            if (cmd->exp != NULL) {
                printf(" ");
                visualize_expression(cmd->exp, source);
            }
            printf(";");
            break;
        }
        case OpenScopeStmt:
            printf("{");
            tab_size += 4;
            break;
        case CloseScopeStmt:
            printf("}");
            tab_size -= 4;
            break;
        }
    }
}

void visualize_expression(Expression *exp, char *source) {
    switch (exp->type) {
    case ExpExp: {
        OpExpression *op_exp = exp->data.exp;
        switch (op_exp->token->ttype) {
        case Not:
            printf("! ");
            visualize_expression(op_exp->left, source);
            break;
        case Identifier:
        case Number:
        case True:
        case False: {
            printf("%s:", substring(source, op_exp->token->start, op_exp->token->end));
            generic_datatype_view(op_exp->datatype, source);
            break;
        }
        default: {
            printf("(");
            visualize_expression(op_exp->left, source);
            printf(" %s ", substring(source, op_exp->token->start, op_exp->token->end));
            visualize_expression(op_exp->right, source);
            printf("):");
            generic_datatype_view(op_exp->datatype, source);
            break;
        }
        }
        break;
    }
    case FnCallExp: {
        Call *call = exp->data.fn_call;
        printf("%s(", substring(source, call->call_name->start, call->call_name->end));
        for (int i = 0; i < call->args_size; i++) {
            visualize_expression(&call->args[i], source);
            if (i != call->args_size - 1) {
                printf(", ");
            }
        }
        printf(") : ");
        generic_datatype_view(call->datatype, source);
    }
    }
}

static void generic_datatype_view(GenericDT *datatype, char *source) {
    if (datatype == NULL) {
        printf("invalid");
        return;
    }
    switch (datatype->type) {
    case Simple: {
        switch (datatype->data.simple_datatype) {
        case Int:
            printf("int");
            break;
        case Bool:
            printf("bool");
            break;
        case Void:
            printf("void");
            break;
        }
        break;
    }
    default: {
        printf("fn(");
        for (int i = 0; i < datatype->data.fn_datatype->params_size; i++) {
            FnParam param = datatype->data.fn_datatype->params[i];
            generic_datatype_view(param.datatype, source);
            if (i != datatype->data.fn_datatype->params_size - 1) {
                printf(", ");
            }
        }
        printf(") : ");
        generic_datatype_view(datatype->data.fn_datatype->return_type, source);
    }
    }
}
