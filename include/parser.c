#include "parser.h"
#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int EOF_PREC = -1;
int PAREN_PREC = 0;

static void advance(ParseCache *cache, size_t step) { cache->current += step; }

static Token *peek(ParseCache *cache, size_t step) { return &cache->tokens[cache->current + step]; }

static void add_error(ParseCache *cache, char *message, Token *token) {
    Error *err = malloc(sizeof(Error));
    err->type = ParseError;
    err->message = malloc(strlen(message) + 1);
    strcpy(err->message, message);
    err->token = token;
    cache->err = err;
}

static void parse_fn_params(ParseCache *cache, FunctionType *fn_type) {
    Token *token = &cache->tokens[cache->current];
    fn_type->params_size = 0;
    fn_type->params = NULL;
    while (1) {
        Token *name = peek(cache, 0);
        if (name->ttype != Identifier) {
            add_error(cache, "expected parameter name", name);
            return;
        }
        if (peek(cache, 1)->ttype != Colon) {
            add_error(cache, "expected parameter type", peek(cache, 1));
            return;
        }
        advance(cache, 2);
        GenericDT *param_type = parse_type(cache);
        if (cache->err != NULL) {
            return;
        }
        FnParam param;
        fn_param_init(&param);
        param.datatype = param_type;
        param.name = name;
        fn_type->params_size++;
        fn_type->params = realloc(fn_type->params, sizeof(FnParam *) * fn_type->params_size);
        fn_type->params[fn_type->params_size - 1] = param;
        advance(cache, 1);
        if (peek(cache, 0)->ttype == Comma) {
            advance(cache, 1);
            continue;
        } else if (peek(cache, 0)->ttype == RParen) {
            break;
        } else {
            add_error(cache, "expected comma of right paren", peek(cache, 1));
            return;
        }
    }
}

static GenericDT *parse_type(ParseCache *cache) {
    printf("Parsing type\n");
    Token *token = &cache->tokens[cache->current];
    switch (token->ttype) {
    case IntType: {
        GenericDT *datatype = generic_datatype_create();
        datatype->type = Simple;
        datatype->data.simple_datatype = Int;
        return datatype;
    }
    case BoolType: {
        GenericDT *datatype = generic_datatype_create();
        datatype->type = Simple;
        datatype->data.simple_datatype = Bool;
        return datatype;
    }
    case Fn: {
        GenericDT *datatype = generic_datatype_create();
        FunctionType *fn_type = malloc(sizeof(FunctionType));
        function_type_init(fn_type);
        GenericDT *return_type;
        datatype->type = Complex;
        if (peek(cache, 1)->ttype != LParen) {
            add_error(cache, "expected list of arguments", token);
            return NULL;
        }
        fn_type->params_size = 0;
        fn_type->params = NULL;
        advance(cache, 2);
        if (peek(cache, 0)->ttype != RParen) {
            while (1) {
                FnParam param;
                fn_param_init(&param);
                GenericDT *param_type = parse_type(cache);
                if (cache->err != NULL) {
                    return NULL;
                }
                param.datatype = param_type;
                param.name = NULL;
                fn_type->params_size++;
                FnParam *new_params = realloc(fn_type->params, fn_type->params_size * sizeof(FnParam));
                new_params[fn_type->params_size - 1] = param;
                fn_type->params = new_params;
                advance(cache, 1);
                Token *current = peek(cache, 0);
                if (current->ttype == RParen) {
                    break;
                }
                if (current->ttype == Comma) {
                    advance(cache, 1);
                    continue;
                }
                add_error(cache, "expected comma or right paren", current);
                return NULL;
            }
        }
        Token *next = peek(cache, 1);
        if (next->ttype != Colon) {
            return_type = generic_datatype_create();
            return_type->type = Simple;
            return_type->data.simple_datatype = Void;
        } else {
            advance(cache, 2);
            return_type = parse_type(cache);
            if (cache->err != NULL) {
                return NULL;
            }
        }
        fn_type->return_type = return_type;
        datatype->data.fn_datatype = fn_type;
        return datatype;
    }
    default: {
        add_error(cache, "expected type specification", token);
        return NULL;
    }
    }
};

Call *parse_fn_call(ParseCache *cache) {
    Call *call = malloc(sizeof(Call));
    call_init(call);
    Token *name = peek(cache, 0);
    call->call_name = name;
    advance(cache, 2);
    Token *current = peek(cache, 0);
    int has_args = peek(cache, 0)->ttype != RParen;
    if (has_args) {
    } else {
    }
    while (has_args) {
        Token *token = peek(cache, 0);
        Expression exp;
        parse_exp(cache, PAREN_PREC, Comma, &exp);
        if (cache->err != NULL) {
            return NULL;
        }
        call->args_size++;
        call->args = realloc(call->args, sizeof(Expression) * call->args_size);
        call->args[call->args_size - 1] = exp;
        Token *next = peek(cache, 1);
        if (next->ttype == Comma) {
            advance(cache, 2);
            continue;
        } else if (next->ttype == RParen) {
            advance(cache, 1);
            break;
        } else {
            add_error(cache, "expected comma or right paren", next);
            return NULL;
        }
    }

    return call;
}

void parse_prefix(ParseCache *cache, Expression *exp) {
    printf("Parsing prefix\n");
    Token *token = peek(cache, 0);
    switch (token->ttype) {
    case Number: {
        OpExpression *number = malloc(sizeof(OpExpression));
        op_expression_init(number);
        GenericDT *datatype = generic_datatype_create();
        exp->type = ExpExp;
        datatype->type = Simple;
        datatype->data.simple_datatype = Int;
        number->datatype = datatype;
        number->token = token;
        number->right = NULL;
        number->left = NULL;
        exp->data.exp = number;
        return;
    }
    case True:
    case False: {
        OpExpression *bool = malloc(sizeof(OpExpression));
        op_expression_init(bool);
        GenericDT *datatype = generic_datatype_create();
        exp->type = ExpExp;
        datatype->type = Simple;
        datatype->data.simple_datatype = Bool;
        bool->datatype = datatype;
        bool->token = token;
        bool->left = NULL;
        bool->right = NULL;
        exp->data.exp = bool;
        return;
    }
    case Not: {
        advance(cache, 1);
        Expression *sub_exp = malloc(sizeof(Expression));
        parse_prefix(cache, sub_exp);
        if (cache->err != NULL) {
            return;
        }
        OpExpression *bool = malloc(sizeof(OpExpression));
        op_expression_init(bool);
        GenericDT *datatype = generic_datatype_create();
        exp->type = ExpExp;
        datatype->type = Simple;
        datatype->data.simple_datatype = Bool;
        bool->datatype = datatype;
        bool->token = token;
        bool->left = sub_exp;
        exp->data.exp = bool;
        return;
    }
    case Identifier: {
        if (peek(cache, 1)->ttype != LParen) {
            OpExpression *id = malloc(sizeof(OpExpression));
            op_expression_init(id);
            id->token = token;
            exp->type = ExpExp;
            exp->data.exp = id;
            return;
        }
        exp->type = FnCallExp;
        Call *fn_call = parse_fn_call(cache);
        Token *cur_t = peek(cache, 0);
        if (cache->err != NULL) {
            return;
        }
        exp->data.fn_call = fn_call;
        return;
    }
    case LParen:
        advance(cache, 1);
        parse_exp(cache, PAREN_PREC, RParen, exp);
        if (cache->err != NULL) {
            return;
        }
        advance(cache, 1);
        return;
    default:
        add_error(cache, "unexpected prefix", token);
        return;
    }
}

static void parse_exp(ParseCache *cache, int prec, TokenType end, Expression *exp) {
    printf("Parsing expression\n");
    Expression left;
    parse_prefix(cache, &left);
    printf("Parsed prefix\n");
    if (cache->err != NULL) {
        return;
    }
    while (1) {
        Token *op = peek(cache, 1);
        if (op->ttype == Eof) {
            add_error(cache, "expected ; at the end of the statement", peek(cache, 0));
            return;
        }
        int op_prec = tt_int_ht_get(cache->precs, op->ttype);
        if (op->ttype == end || prec >= op_prec) {
            break;
        }
        if (!tt_int_ht_get(cache->legal_infixes, op->ttype)) {
            add_error(cache, "invalid infix", op);
            return;
        }
        Expression next_left;
        GenericDT *datatype = generic_datatype_create();
        OpExpression *op_exp = malloc(sizeof(OpExpression));
        op_expression_init(op_exp);
        next_left.type = ExpExp;
        datatype->type = Simple;
        switch (op->ttype) {
        case Plus:
        case Minus:
        case Star:
        case Slash:
        case Mod: {
            datatype->data.simple_datatype = Int;
            break;
        }
        default: {
            datatype->data.simple_datatype = Bool;
            break;
        }
        }
        advance(cache, 2);
        Expression *right = malloc(sizeof(Expression));
        parse_exp(cache, op_prec, end, right);
        if (cache->err != NULL) {
            return;
        }
        Expression *allocated_left = malloc(sizeof(Expression));
        *allocated_left = left;
        op_exp->token = op;
        op_exp->left = allocated_left;
        op_exp->right = right;
        op_exp->datatype = datatype;
        next_left.data.exp = op_exp;
        left = next_left;
    }
    *exp = left;
}

Oneliner *parse_oneliner(ParseCache *cache, TokenType end) {
    printf("Parsing oneliner\n");
    Oneliner *ol = malloc(sizeof(Oneliner));
    Token *name = peek(cache, 0);
    Token *next = peek(cache, 1);
    if (next->ttype == LParen) {
        ol->type = CallOL;
        ol->data.call = parse_fn_call(cache);
        if (cache->err != NULL) {
            return NULL;
        }
        return ol;
    }
    ol->type = AssignmentOL;
    Assignment *ass = malloc(sizeof(Assignment));
    assignment_init(ass);
    ass->var = name;
    switch (next->ttype) {
    case Inc:
    case Dec:
        ass->op = next;
        ass->new_var = 0;
        ass->exp = NULL;
        ol->data.assignment = ass;
        advance(cache, 1);
        return ol;
    case Eq:
    case ColEq:
    case PlusEq:
    case MinusEq:
    case StarEq:
    case SlashEq:
    case ModEq:
        ass->op = next;
        ass->new_var = next->ttype == ColEq;
        advance(cache, 1);
        break;
    case Colon:
        ass->new_var = 1;
        advance(cache, 2);
        GenericDT *datatype = parse_type(cache);
        if (cache->err != NULL) {
            return NULL;
        }
        ass->datatype = datatype;
        next = peek(cache, 1);
        if (next->ttype != Eq) {
            add_error(cache, "expected =", next);
            return NULL;
        }
        ass->op = next;
        advance(cache, 1);
        break;
    default:
        add_error(cache, "unexpected operator", next);
        return NULL;
    }
    advance(cache, 1);
    Expression *exp = malloc(sizeof(Expression));
    parse_exp(cache, EOF_PREC, end, exp);
    if (cache->err != NULL) {
        return NULL;
    }
    ass->exp = exp;
    ol->data.assignment = ass;

    return ol;
}

void parse(ParseCache *cache, int block, Stmt **stmts, size_t *stmts_size, size_t *stmts_capacity) {
    while (cache->current < cache->tokens_size) {
        Token *token = peek(cache, 0);
        Stmt stmt;
        switch (token->ttype) {
        case LBrace: {
            stmt.type = OpenScopeStmt;
            OpenScopeCmd *o_cmd = malloc(sizeof(OpenScopeCmd));
            o_cmd->token = token;
            stmt.data.open_scope_cmd = o_cmd;
            (*stmts_size)++;
            *stmts = realloc(*stmts, (*stmts_size) * sizeof(Stmt));
            (*stmts)[*stmts_size - 1] = stmt;
            advance(cache, 1);
            parse(cache, 1, stmts, stmts_size, stmts_capacity);
            if (cache->err != NULL) {
                return;
            }
            token = peek(cache, 0);
            Stmt close_scope;
            close_scope.type = CloseScopeStmt;
            CloseScopeCmd *c_cmd = malloc(sizeof(CloseScopeCmd));
            c_cmd->token = token;
            close_scope.data.close_scope_cmd = c_cmd;
            (*stmts_size)++;
            *stmts = realloc(*stmts, (*stmts_size) * sizeof(Stmt));
            (*stmts)[(*stmts_size) - 1] = close_scope;
            advance(cache, 1);
            continue;
        }
        case RBrace: {
            if (!block) {
                add_error(cache, "unexpected right brace: no open scope to close", token);
            }
            return;
        }
        case Break: {
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon at the end of the statement", token);
                return;
            }
            stmt.type = BreakStmt;
            BreakCmd *cmd = malloc(sizeof(BreakCmd));
            cmd->token = token;
            stmt.data.break_cmd = cmd;
            advance(cache, 1);
            break;
        }
        case Continue: {
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon at the end of the statement", token);
                return;
            }
            stmt.type = ContinueStmt;
            ContinueCmd *cmd = malloc(sizeof(ContinueCmd));
            cmd->token = token;
            stmt.data.continue_cmd = cmd;
            advance(cache, 1);
            break;
        }
        case Return: {
            ReturnCmd *cmd = malloc(sizeof(ReturnCmd));
            return_cmd_init(cmd);
            advance(cache, 1);
            if (peek(cache, 0)->ttype != Semicolon) {
                cmd->exp = malloc(sizeof(Expression));
                parse_exp(cache, EOF_PREC, Semicolon, cmd->exp);
                if (cache->err != NULL) {
                    return;
                }
                advance(cache, 1);
            }
            stmt.type = ReturnStmt;
            cmd->token = token;
            stmt.data.return_cmd = cmd;
            Token *last = peek(cache, 0);
            if (last->ttype != Semicolon) {
                add_error(cache, "expected semicolon at the end of the statement", last);
                return;
            }
            break;
        }
        case Identifier: {
            stmt.type = OnelinerStmt;
            Oneliner *ol = parse_oneliner(cache, Semicolon);
            if (cache->err != NULL) {
                return;
            }
            stmt.data.oneliner = ol;
            advance(cache, 1);
            if (peek(cache, 0)->ttype != Semicolon) {
                add_error(cache, "expected semicolon at the end of the statement", token);
                return;
            }
            break;
        }
        case If:
        case While: {
            Conditional *conditional = malloc(sizeof(Conditional));
            conditional_init(conditional);
            stmt.type = ConditionalStmt;
            conditional->token = token;
            advance(cache, 1);
            Expression *cond = malloc(sizeof(Expression));
            parse_exp(cache, EOF_PREC, LBrace, cond);
            if (cache->err != NULL) {
                return;
            }
            conditional->condition = cond;
            if (peek(cache, 1)->ttype != LBrace) {
                add_error(cache, "no block provided for conditional", token);
                return;
            }
            advance(cache, 2);
            size_t then_capacity = 0;
            parse(cache, 1, &conditional->then_block, &(conditional->then_size), &then_capacity);
            if (cache->err != NULL) {
                return;
            }
            if (peek(cache, 1)->ttype != Else) {
                stmt.data.conditional = conditional;
                break;
            }
            if (peek(cache, 2)->ttype != LBrace) {
                add_error(cache, "no block provided for else", peek(cache, 0));
                return;
            }
            advance(cache, 3);
            size_t else_capacity = 0;
            parse(cache, 1, &conditional->else_block, &(conditional->else_size), &else_capacity);
            if (cache->err != NULL) {
                return;
            }
            stmt.data.conditional = conditional;
            break;
        }
        case For: {
            stmt.type = ForStmt;
            ForLoop *for_loop = malloc(sizeof(ForLoop));
            for_loop_init(for_loop);
            for_loop->token = token;
            advance(cache, 1);
            Oneliner *init = parse_oneliner(cache, Semicolon);
            if (cache->err != NULL) {
                return;
            }
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon after init", peek(cache, 0));
                return;
            }
            for_loop->init = init;
            advance(cache, 2);
            Expression *cond = malloc(sizeof(Expression));
            parse_exp(cache, EOF_PREC, Semicolon, cond);
            if (cache->err != NULL) {
                return;
            }
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon after condition", peek(cache, 0));
                return;
            }
            for_loop->condition = cond;
            advance(cache, 2);
            Oneliner *after = parse_oneliner(cache, Semicolon);
            if (cache->err != NULL) {
                return;
            }
            if (peek(cache, 1)->ttype != LBrace) {
                add_error(cache, "expected semicolon after after", peek(cache, 0));
                return;
            }
            for_loop->after = after;
            for_loop->body = NULL;
            for_loop->body_size = 0;
            advance(cache, 2);
            size_t body_capacity = 0;
            parse(cache, 1, &for_loop->body, &(for_loop->body_size), &body_capacity);
            if (cache->err != NULL) {
                return;
            }
            stmt.data.for_loop = for_loop;
            break;
        }
        case Fn: {
            Token *name_token = peek(cache, 1);
            if (name_token->ttype != Identifier) {
                add_error(cache, "expected function name", peek(cache, 1));
                return;
            }
            if (peek(cache, 2)->ttype != LParen) {
                add_error(cache, "expected function parameters", peek(cache, 2));
                return;
            }
            FnDefinition *fn_def = malloc(sizeof(FnDefinition));
            fn_definition_init(fn_def);
            FunctionType *fn_type = malloc(sizeof(FunctionType));
            function_type_init(fn_type);
            GenericDT *return_type;
            stmt.type = FnStmt;
            fn_def->name = name_token;
            advance(cache, 3);
            int has_params = peek(cache, 0)->ttype != RParen;
            if (has_params) {
                parse_fn_params(cache, fn_type);
            }
            if (cache->err != NULL) {
                return;
            }
            if (peek(cache, 1)->ttype == Colon) {
                advance(cache, 2);
                return_type = parse_type(cache);
                if (cache->err != NULL) {
                    return;
                }
            } else {
                return_type = generic_datatype_create();
                return_type->type = Simple;
                return_type->data.simple_datatype = Void;
            }
            if (peek(cache, 1)->ttype != LBrace) {
                add_error(cache, "expected function body", peek(cache, 0));
                return;
            }

            fn_type->return_type = return_type;
            fn_def->datatype = fn_type;
            fn_def->body = NULL;
            fn_def->body_size = 0;
            advance(cache, 2);
            size_t body_capacity = 0;
            parse(cache, 1, &fn_def->body, &(fn_def->body_size), &body_capacity);
            if (cache->err != NULL) {
                return;
            }
            stmt.data.fn_def = fn_def;
            Token *last = peek(cache, 0);
            break;
        }
        case Eof:
            if (block) {
                add_error(cache, "unexpected EOF: scope not closed", token);
            }
            return;
        default: {
            add_error(cache, "unexpected token", token);
            return;
        }
        }
        advance(cache, 1);
        if (*stmts_capacity <= *stmts_size) {
            if (*stmts_capacity == 0) {
                *stmts_capacity = 128;
            } else {
                *stmts_capacity *= 2;
            }
            Stmt *new_stmts = realloc(*stmts, (*stmts_capacity) * sizeof(Stmt));
            *stmts = new_stmts;
        }
        (*stmts)[*stmts_size] = stmt;
        (*stmts_size)++;
    }
}
