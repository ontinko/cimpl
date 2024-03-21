#include "parser.h"
#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int EOF_PREC = -1;
int PAREN_PREC = 0;

void advance(ParseCache *cache, size_t step) { cache->current += step; }

Token *peek(ParseCache *cache, size_t step) {
    return cache->tokens[cache->current + step];
}

Expression *parse_exp(ParseCache *cache, int prec, TokenType end);

void add_error(ParseCache *cache, char *message, Token *token) {
    printf("ERROR %lu:%lu: %s\n", token->ln, token->start - token->ln_start + 1,
           message);
    ParseError *err = malloc(sizeof(ParseError));
    err->message = malloc(strlen(message) + 1);
    strcpy(err->message, message);
    err->token = token;
    cache->err = err;
}

Call *parse_fn_call(ParseCache *cache) {
    printf("Parsing fn call\n");
    Call *call = malloc(sizeof(Call));
    Token *name = peek(cache, 0);
    call->call_name = name;
    call->args = NULL;
    advance(cache, 2);
    int has_args = peek(cache, 0)->ttype != RParen;
    if (has_args) {
        printf("Has arguments\n");
    }
    while (has_args) {
        Token *token = peek(cache, 0);
        Expression *exp = parse_exp(cache, PAREN_PREC, Comma);
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

Expression *parse_prefix(ParseCache *cache) {
    Token *token = peek(cache, 0);
    switch (token->ttype) {
    case Number: {
        Expression *exp = malloc(sizeof(Expression));
        exp->exp = malloc(sizeof(ExpUnion));
        OpExpression sub_exp = {
            .datatype = Int, .token = token, .right = NULL, .left = NULL};
        exp->exp->exp = sub_exp;
        return exp;
    }
    case True:
    case False: {
        Expression *exp = malloc(sizeof(Expression));
        exp->exp = malloc(sizeof(ExpUnion));
        OpExpression sub_exp = {
            .datatype = Bool, .token = token, .right = NULL, .left = NULL};
        exp->exp->exp = sub_exp;
        return exp;
    }
    case Not: {
        advance(cache, 1);
        Expression *sub_exp = parse_prefix(cache);
        if (cache->err != NULL) {
            return NULL;
        }
        Expression *exp = malloc(sizeof(Expression));
        exp->exp = malloc(sizeof(ExpUnion));
        OpExpression negation = {
            .datatype = Bool, .token = token, .right = NULL, .left = sub_exp};
        exp->exp->exp = negation;
        return exp;
    }
    case Identifier: {
        Expression *exp = malloc(sizeof(Expression));
        exp->exp = malloc(sizeof(ExpUnion));
        if (peek(cache, 1)->ttype != LParen) {
            OpExpression sub_exp = {
                .token = token, .right = NULL, .left = NULL};
            exp->exp->exp = sub_exp;
            return exp;
        }
        Call *fn_call = parse_fn_call(cache);
        Token *cur_t = peek(cache, 0);
        if (cache->err != NULL) {
            return NULL;
        }
        exp->exp->fn_call = *fn_call;
        return exp;
    }
    case LParen:
        advance(cache, 1);
        Expression *exp = parse_exp(cache, PAREN_PREC, RParen);
        if (cache->err != NULL) {
            return NULL;
        }
        advance(cache, 1);
        return exp;
    default:
        add_error(cache, "unexpected prefix", token);
        return NULL;
    }
};

Expression *parse_exp(ParseCache *cache, int prec, TokenType end) {
    Expression *left = parse_prefix(cache);
    if (cache->err != NULL) {
        return NULL;
    }
    while (1) {
        Token *op = peek(cache, 1);
        if (op->ttype == Eof) {
            add_error(cache, "expected ; at the end of the statement",
                      peek(cache, 0));
            return NULL;
        }
        int op_prec = tt_int_ht_get(cache->precs, op->ttype);
        if (op->ttype == end || prec >= op_prec) {
            break;
        }
        if (!tt_int_ht_get(cache->legal_infixes, op->ttype)) {
            add_error(cache, "invalid infix", op);
            return NULL;
        }
        Expression *next_left = malloc(sizeof(Expression));
        next_left->exp = malloc(sizeof(ExpUnion));
        next_left->type = ExpExp;
        advance(cache, 2);
        Expression *right = parse_exp(cache, op_prec, end);
        if (cache->err != NULL) {
            return NULL;
        }
        OpExpression op_exp = {.token = op, .left = left, .right = right};
        next_left->exp->exp = op_exp;
        left = next_left;
    }
    return left;
}

Oneliner *parse_oneliner(ParseCache *cache, TokenType end) {
    Oneliner *ol = malloc(sizeof(Oneliner));
    ol->stmt = malloc(sizeof(OnelinerUnion));
    Token *name = peek(cache, 0);
    Token *next = peek(cache, 1);
    if (next->ttype == LParen) {
        ol->type = CallOL;
        ol->stmt->call = *parse_fn_call(cache);
        return ol;
    }
    ol->type = AssignmentOL;
    Assignment *ass = malloc(sizeof(Assignment));
    ass->var = name;
    switch (next->ttype) {
    case Inc:
    case Dec:
        ass->op = next;
        ass->new_var = 0;
        ass->exp = NULL;
        ol->stmt->assignment = *ass;
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
        ass->new_var = 0;
        advance(cache, 1);
        break;
    case Colon:
        ass->new_var = 1;
        advance(cache, 2);
        Token *dt_token = peek(cache, 0);
        switch (dt_token->ttype) {
        case BoolType:
            ass->datatype = Bool;
            break;
        case IntType:
            ass->datatype = Int;
            break;
        default:
            add_error(cache, "invalid var type", dt_token);
        }
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
        printf("Added error\n");
        return NULL;
    }
    advance(cache, 1);
    Expression *exp = parse_exp(cache, EOF_PREC, end);
    if (cache->err != NULL) {
        return NULL;
    }
    ass->exp = exp;
    ol->stmt->assignment = *ass;

    return ol;
}

void parse(ParseCache *cache, int block, Stmt ***stmts, size_t *stmts_size) {
    while (cache->current < cache->tokens_size) {
        Token *token = peek(cache, 0);
        Stmt *stmt;
        switch (token->ttype) {
        case LBrace: {
            printf("LBrace at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = OpenScopeStmt;
            OpenScopeCmd o_cmd = {.token = token};
            stmt->stmt->open_scope_cmd = o_cmd;
            (*stmts_size)++;
            *stmts = realloc(*stmts, (*stmts_size) * sizeof(Stmt *));
            (*stmts)[*stmts_size - 1] = stmt;
            advance(cache, 1);
            parse(cache, 1, stmts, stmts_size);
            if (cache->err != NULL) {
                return;
            }
            printf("Parsed block\n");
            token = peek(cache, 0);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = CloseScopeStmt;
            CloseScopeCmd c_cmd = {.token = token};
            stmt->stmt->close_scope_cmd = c_cmd;
            (*stmts_size)++;
            *stmts = realloc(*stmts, (*stmts_size) * sizeof(Stmt *));
            (*stmts)[(*stmts_size) - 1] = stmt;
            advance(cache, 1);
            continue;
        }
        case RBrace: {
            printf("RBrace at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            if (!block) {
                printf("RBrace outside of a block\n");
                add_error(cache,
                          "unexpected right brace: no open scope to close",
                          token);
            }
            return;
        }
        case Break: {
            printf("Break at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache,
                          "expected semicolon at the end of the statement",
                          token);
                return;
            }
            stmt->type = BreakStmt;
            BreakCmd cmd = {.token = token};
            stmt->stmt->break_cmd = cmd;
            advance(cache, 1);
            break;
        }
        case Continue: {
            printf("Continue at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache,
                          "expected semicolon at the end of the statement",
                          token);
                return;
            }
            stmt->type = ContinueStmt;
            ContinueCmd cmd = {.token = token};
            stmt->stmt->continue_cmd = cmd;
            advance(cache, 1);
            break;
        }
        case Return: {
            printf("Return at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = ReturnStmt;
            Expression *cmd_exp = NULL;
            advance(cache, 1);
            if (peek(cache, 0)->ttype != Semicolon) {
                cmd_exp = parse_exp(cache, EOF_PREC, Semicolon);
                if (cache->err != NULL) {
                    return;
                }
                advance(cache, 1);
            }
            ReturnCmd cmd = {.token = token, .exp = cmd_exp};
            stmt->stmt->return_cmd = cmd;
            Token *last = peek(cache, 0);
            break;
        }
        case Identifier: {
            printf("Identifier at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = OnelinerStmt;
            Oneliner *ol = parse_oneliner(cache, Semicolon);
            if (cache->err != NULL) {
                return;
            }
            stmt->stmt->oneliner = *ol;
            advance(cache, 1);
            if (peek(cache, 0)->ttype != Semicolon) {
                printf("Expected semicolon\n");
                add_error(cache,
                          "expected semicolon at the end of the statement",
                          token);
                return;
            }
            break;
        }
        case If:
        case While: {
            printf("Conditional at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = ConditionalStmt;
            Conditional conditional = {.token = token};
            advance(cache, 1);
            Expression *cond = parse_exp(cache, EOF_PREC, LBrace);
            if (cache->err != NULL) {
                return;
            }
            conditional.condition = cond;
            if (peek(cache, 1)->ttype != LBrace) {
                add_error(cache, "no block provided for conditional", token);
                return;
            }
            advance(cache, 2);
            conditional.then_block = NULL;
            conditional.then_size = 0;
            parse(cache, 1, &conditional.then_block,
                  &(conditional.then_size)); // probably fine
            if (cache->err != NULL) {
                return;
            }
            conditional.else_block = NULL;
            conditional.else_size = 0;
            if (peek(cache, 1)->ttype != Else) {
                stmt->stmt->conditional = conditional;
                break;
            }
            if (peek(cache, 2)->ttype != LBrace) {
                add_error(cache, "no block provided for else", peek(cache, 0));
                return;
            }
            advance(cache, 3);
            parse(cache, 1, &conditional.else_block, &(conditional.else_size));
            if (cache->err != NULL) {
                return;
            }
            stmt->stmt->conditional = conditional;
            break;
        }
        case For: {
            printf("For at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = ForStmt;
            ForLoop for_loop;
            advance(cache, 1);
            Oneliner *init = parse_oneliner(cache, Semicolon);
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon after init",
                          peek(cache, 0));
                return;
            }
            for_loop.init = init;
            advance(cache, 2);
            Expression *cond = parse_exp(cache, EOF_PREC, Semicolon);
            if (peek(cache, 1)->ttype != Semicolon) {
                add_error(cache, "expected semicolon after condition",
                          peek(cache, 0));
                return;
            }
            for_loop.condition = cond;
            advance(cache, 2);
            Oneliner *after = parse_oneliner(cache, Semicolon);
            if (peek(cache, 1)->ttype != LBrace) {
                add_error(cache, "expected semicolon after after",
                          peek(cache, 0));
                return;
            }
            for_loop.after = after;
            advance(cache, 2);
            for_loop.body = NULL;
            for_loop.body_size = 0;
            parse(cache, 1, &for_loop.body, &(for_loop.body_size));
            if (cache->err != NULL) {
                return;
            }
            stmt->stmt->for_loop = for_loop;
            break;
        }
        case Def: {
            printf("Def at %lu:%lu\n", token->ln,
                   token->start - token->ln_start + 1);
            stmt = malloc(sizeof(Stmt));
            stmt->stmt = malloc(sizeof(StmtUnion));
            stmt->type = DefStmt;
            FnDef fn_def;
            if (peek(cache, 1)->ttype != Identifier) {
                add_error(cache, "expected function name", peek(cache, 1));
                return;
            }
            fn_def.name = peek(cache, 1);
            fn_def.params = NULL;
            fn_def.params_size = 0;
            if (peek(cache, 2)->ttype != LParen) {
                add_error(cache, "expected function parameters",
                          peek(cache, 2));
                return;
            }
            advance(cache, 3);
            int has_params = peek(cache, 0)->ttype != RParen;
            while (has_params) {
                printf("Parsing def param from token %lu\n", cache->current);
                DefParam *param = malloc(sizeof(DefParam));
                Token *name = peek(cache, 0);
                if (name->ttype != Identifier) {
                    add_error(cache, "expected parameter name", peek(cache, 0));
                    return;
                }
                param->name = name;
                advance(cache, 1);
                Token *type = peek(cache, 0);
                switch (type->ttype) {
                case IntType:
                    param->datatype = Int;
                    break;
                case BoolType:
                    param->datatype = Bool;
                    break;
                default: {
                    add_error(cache, "expected parameter type", peek(cache, 0));
                    return;
                }
                }
                fn_def.params_size++;
                fn_def.params = realloc(fn_def.params, sizeof(DefParam *) *
                                                           fn_def.params_size);
                fn_def.params[fn_def.params_size - 1] = param;
                advance(cache, 1);
                if (peek(cache, 0)->ttype == Comma) {
                    advance(cache, 1);
                    continue;
                } else if (peek(cache, 0)->ttype == RParen) {
                    break;
                } else {
                    add_error(cache, "unexpected comma of right paren",
                              peek(cache, 1));
                    return;
                }
            }
            advance(cache, 1);
            switch (peek(cache, 0)->ttype) {
            case IntType:
                fn_def.datatype = Int;
                advance(cache, 1);
                break;
            case BoolType:
                fn_def.datatype = Bool;
                advance(cache, 1);
                break;
            case LBrace:
                fn_def.datatype = Void;
                break;
            default: {
                printf("Error at token %lu\n", cache->current);
                add_error(cache, "expected return type", peek(cache, 0));
                return;
            }
            }
            if (fn_def.datatype != Void && peek(cache, 0)->ttype != LBrace) {
                add_error(cache, "expected function body", peek(cache, 0));
                return;
            }

            advance(cache, 1);
            fn_def.body = NULL;
            fn_def.body_size = 0;
            printf("Parsing fn body from token %lu\n", cache->current);
            parse(cache, 1, &fn_def.body, &(fn_def.body_size));
            if (cache->err != NULL) {
                return;
            }
            stmt->stmt->fn_def = fn_def;
            Token *last = peek(cache, 0);
            break;
        }
        case Eof:
            if (block) {
                add_error(cache, "unexpected EOF: scope not closed", token);
            }
            return;
        default: {
            add_error(cache, "default unexpected token", token);
            return;
        }
        }
        printf("Adding stmt\n");
        advance(cache, 1);
        (*stmts_size)++;
        size_t new_size = (*stmts_size) * sizeof(Stmt *);
        *stmts = realloc(*stmts, (*stmts_size) * sizeof(Stmt *));
        (*stmts)[*stmts_size - 1] = stmt;
    }
}
