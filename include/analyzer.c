#include "analyzer.h"
#include "ast.h"
#include "token.h"
#include "utils.h"
#include <error.h>
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

HashTable *hashtable_create() {
    HashTable *ht = malloc(sizeof(HashTable));
    ht->size = 0;
    ht->keys = NULL;
    ht->datatypes = NULL;
    return ht;
}

void hashtable_destroy(HashTable *ht) {
    for (int i = 0; i < ht->size; i++) {
        for (int j = 0; j < 512; j++) {
            free(ht->keys[i][j]);
        }
        free(ht->keys[i]);
        free(ht->datatypes[i]);
    }
    free(ht);
}

void hashtable_set(HashTable *ht, char *key, GenericDT *value) {
    int index = hash(key);
    for (int i = 0; i < ht->size; i++) {
        if (ht->keys[i][index] != NULL && strcmp(ht->keys[i][index], key)) {
            continue;
        }
        ht->keys[i][index] = key;
        ht->datatypes[i][index] = value;
        return;
    }
    ht->size++;
    char *(*new_keys)[512] = realloc(ht->keys, ht->size * sizeof(char *[512]));
    GenericDT *(*new_datatypes)[512] = realloc(ht->datatypes, ht->size * sizeof(GenericDT *[512]));
    for (int i = 0; i < 512; i++) {
        new_datatypes[ht->size - 1][i] = NULL;
        new_keys[ht->size - 1][i] = NULL;
    }
    ht->keys = new_keys;
    ht->datatypes = new_datatypes;
    ht->keys[ht->size - 1][index] = key;
    ht->datatypes[ht->size - 1][index] = value;
}

void hashtable_get(HashTable *ht, char *key, GenericDT **datatype) {
    *datatype = NULL;
    int index = hash(key);
    for (int i = 0; i < ht->size; i++) {
        if (ht->keys[i][index] == NULL) {
            return;
        }
        if (strcmp(ht->keys[i][index], key)) {
            continue;
        }
        *datatype = ht->datatypes[i][index];
        return;
    }
}

AnalysisCache *analysis_cache_create(char *source) {
    AnalysisCache *cache = malloc(sizeof(AnalysisCache));
    cache->source = source;
    cache->defs = NULL;
    cache->current_function = NULL;
    cache->errors = NULL;
    cache->errors_size = 0;
    cache->cache_size = 0;
    cache->in_loop = 0;
    cache->current_scope = 0;
    analysis_cache_extend(cache);
    return cache;
}

static void analysis_cache_get(AnalysisCache *cache, Token *var_token, GenericDT **datatype, int *scope) {
    char *var_name = substring(cache->source, var_token->start, var_token->end);
    *datatype = NULL;
    for (int i = cache->cache_size - 1; i >= 0; i--) {
        hashtable_get(cache->defs[i], var_name, datatype);
        if (*datatype != NULL) {
            *scope = i;
            return;
        }
    }
}

static void analysis_cache_set(AnalysisCache *cache, Token *var_token, GenericDT *datatype, int scope) {
    if (scope == -1) {
        scope = cache->cache_size - 1;
    }
    char *var_name = substring(cache->source, var_token->start, var_token->end);
    HashTable *current_scope = cache->defs[scope];
    hashtable_set(current_scope, var_name, datatype);
}

static void analysis_cache_extend(AnalysisCache *cache) {
    cache->cache_size++;
    HashTable **new_defs = realloc(cache->defs, cache->cache_size * sizeof(HashTable *));
    HashTable *new_ht = hashtable_create();
    new_defs[cache->cache_size - 1] = new_ht;
    cache->defs = new_defs;
}

static void analysis_cache_shrink(AnalysisCache *cache) {
    cache->cache_size--;
    hashtable_destroy(cache->defs[cache->cache_size]);
    HashTable **new_defs = realloc(cache->defs, cache->cache_size * sizeof(HashTable *));
    cache->defs = new_defs;
}

static int analysis_cache_defined(AnalysisCache *cache, Token *var_token) {
    char *var_name = substring(cache->source, var_token->start, var_token->end);
    for (int i = cache->cache_size - 1; i >= 0; i--) {
        HashTable *current_scope = cache->defs[i];
        GenericDT *dt;
        hashtable_get(current_scope, var_name, &dt);
        if (dt != NULL) {
            return 1;
        }
    }
    return 0;
}

static int analysis_cache_defined_in_current_scope(AnalysisCache *cache, Token *var_token) {
    char *var_name = substring(cache->source, var_token->start, var_token->end);
    HashTable *current_scope = cache->defs[cache->cache_size - 1];
    GenericDT *dt;
    hashtable_get(current_scope, var_name, &dt);
    return dt != NULL;
}

static void analysis_cache_add_error(AnalysisCache *cache, char *message, ErrorType type, Token *token) {
    cache->errors_size++;
    Error **new_errors = realloc(cache->errors, sizeof(Error *) * cache->errors_size);
    cache->errors = new_errors;
    Error *err = error_create();
    err->message = message;
    err->type = type;
    err->token = token;
    cache->errors[cache->errors_size - 1] = err;
}

static void analysis_cache_process_expression(AnalysisCache *cache, Expression *exp, GenericDT **datatype) {
    if (exp == NULL) {
    } else {
    }
    switch (exp->type) {
    case ExpExp: {
        OpExpression *op_exp = exp->data.exp;
        switch (op_exp->token->ttype) {
        case Lt:
        case Gt:
        case GtE:
        case LtE:
        case Plus:
        case Minus:
        case Star:
        case Slash:
        case Mod: {
            *datatype = op_exp->datatype;
            GenericDT *left_exp_dt;
            GenericDT *right_exp_dt;
            analysis_cache_process_expression(cache, op_exp->left, &left_exp_dt);
            if (left_exp_dt->type != Simple || left_exp_dt->data.simple_datatype != Int) {
                analysis_cache_add_error(cache, "invalid operation for given type", TypeError, op_exp->token);
            }
            analysis_cache_process_expression(cache, op_exp->right, &right_exp_dt);
            if (right_exp_dt->type != Simple || right_exp_dt->data.simple_datatype != Int) {
                analysis_cache_add_error(cache, "expected int", TypeError, op_exp->token);
            }
            break;
        }
        case Not: {
            *datatype = op_exp->datatype;
            GenericDT *sub_exp_dt;
            analysis_cache_process_expression(cache, op_exp->left, &sub_exp_dt);
            if (sub_exp_dt->type != Simple || sub_exp_dt->data.simple_datatype != Bool) {
                analysis_cache_add_error(cache, "expected bool", TypeError, op_exp->token);
            }
            break;
        }
        case Or:
        case And: {
            *datatype = op_exp->datatype;
            GenericDT *left_exp_dt;
            GenericDT *right_exp_dt;
            analysis_cache_process_expression(cache, op_exp->left, &left_exp_dt);
            if (left_exp_dt->type != Simple || left_exp_dt->data.simple_datatype != Bool) {
                analysis_cache_add_error(cache, "invalid operation for given type", TypeError, op_exp->token);
            }
            analysis_cache_process_expression(cache, op_exp->right, &right_exp_dt);
            if (right_exp_dt->type != Simple || right_exp_dt->data.simple_datatype != Bool) {
                analysis_cache_add_error(cache, "expected bool", TypeError, op_exp->token);
            }
            break;
        }
        case NotEq:
        case EqEq: {
            *datatype = op_exp->datatype;
            GenericDT *left_exp_dt;
            GenericDT *right_exp_dt;
            analysis_cache_process_expression(cache, op_exp->left, &left_exp_dt);
            analysis_cache_process_expression(cache, op_exp->right, &right_exp_dt);
            if (!generic_datatype_compare(left_exp_dt, right_exp_dt)) {
                analysis_cache_add_error(cache, "cannot compare different types", TypeError, op_exp->token);
            }
            break;
        }
        case True:
        case False:
        case Number: {
            *datatype = op_exp->datatype;
            break;
        }
        default: {
            GenericDT *exp_dt = NULL;
            int scope;
            analysis_cache_get(cache, op_exp->token, &exp_dt, &scope);
            if (exp_dt == NULL) {
                analysis_cache_add_error(cache, "undefined variable", ReferenceError, op_exp->token);
            }
            *datatype = exp_dt;
            break;
        }
        }
        break;
    }
    case FnCallExp: {
        Call *call = exp->data.fn_call;
        GenericDT *fn_datatype = NULL;
        int scope;
        analysis_cache_get(cache, call->call_name, &fn_datatype, &scope);
        int is_defined = 1;
        if (fn_datatype == NULL) {
            is_defined = 0;
            analysis_cache_add_error(cache, "undefined function", ReferenceError, call->call_name);
        } else if (fn_datatype->type != Complex) {
            is_defined = 0;
            analysis_cache_add_error(cache, "not a function", TypeError, call->call_name);
        }
        if (!is_defined) {
            call->datatype = NULL;
        } else {
            call->datatype = fn_datatype->data.fn_datatype->return_type;
        }
        *datatype = call->datatype;

        int has_validatable_params = is_defined && call->args_size == fn_datatype->data.fn_datatype->params_size;
        if (!has_validatable_params) {
            analysis_cache_add_error(cache, "wrong number of arguments", TypeError, call->call_name);
        }
        for (int i = 0; i < call->args_size; i++) {
            GenericDT *arg_dt;
            analysis_cache_process_expression(cache, call->args[i], &arg_dt);
            if (has_validatable_params && arg_dt != NULL && !generic_datatype_compare(arg_dt, fn_datatype->data.fn_datatype->params[i]->datatype)) {
                analysis_cache_add_error(cache, "argument has wrong type", TypeError, call->call_name);
            }
        }
        break;
    }
    }
}

static void analysis_cache_process_oneliner(AnalysisCache *cache, Oneliner *oneliner) {
    switch (oneliner->type) {
    case AssignmentOL: {
        Assignment *ass = oneliner->data.assignment;
        int is_defined_in_current_scope = analysis_cache_defined_in_current_scope(cache, ass->var);
        GenericDT *defined_var_datatype;
        int scope;
        analysis_cache_get(cache, ass->var, &defined_var_datatype, &scope);
        if (ass->new_var && is_defined_in_current_scope) {
            analysis_cache_add_error(cache, "variable redefinition is not allowed", ReferenceError, ass->var);
        }
        switch (ass->op->ttype) {
        case Inc:
        case Dec: {
            GenericDT *datatype;
            int scope;
            analysis_cache_get(cache, ass->var, &datatype, &scope);
            if (datatype == NULL) {
                analysis_cache_add_error(cache, "undefined variable", ReferenceError, ass->var);
            } else if (datatype->type != Simple || datatype->data.simple_datatype != Int) {
                analysis_cache_add_error(cache, "invalid operation for given type", TypeError, ass->var);
            } else {
                ass->datatype = datatype;
                if (cache->current_function == NULL) {
                    ass->scope = scope;
                } else {
                    ass->scope = -1;
                }
            }
            break;
        }
        default: {
            GenericDT *exp_datatype;
            analysis_cache_process_expression(cache, ass->exp, &exp_datatype);
            switch (ass->op->ttype) {
            case PlusEq:
            case MinusEq:
            case StarEq:
            case SlashEq:
            case ModEq: {
                GenericDT *var_datatype;
                int scope;
                analysis_cache_get(cache, ass->var, &var_datatype, &scope);
                ass->datatype = var_datatype;
                if (var_datatype == NULL) {
                    analysis_cache_add_error(cache, "undefined variable", ReferenceError, ass->var);
                } else if (var_datatype != NULL && var_datatype->data.simple_datatype != Int) {
                    analysis_cache_add_error(cache, "invalid operation for given type", TypeError, ass->var);
                } else if (cache->current_function == NULL) {
                    ass->scope = scope;
                } else {
                    ass->scope = -1;
                }
                if (exp_datatype != NULL && exp_datatype->type != Simple || exp_datatype->data.simple_datatype != Int) {
                    analysis_cache_add_error(cache, "expected a number", TypeError, ass->var);
                }
                break;
            }
            case ColEq: {
                if (is_defined_in_current_scope) {
                    break;
                }
                if (cache->current_function == NULL) {
                    ass->scope = cache->cache_size - 1;
                } else {
                    ass->scope = -1;
                }
                ass->datatype = exp_datatype;
                analysis_cache_set(cache, ass->var, exp_datatype, -1);
                break;
            }
            default: {
                if (ass->new_var && is_defined_in_current_scope) {
                    break;
                }
                if (ass->new_var) {
                    analysis_cache_set(cache, ass->var, ass->datatype, -1);
                    if (!generic_datatype_compare(ass->datatype, exp_datatype)) {
                        analysis_cache_add_error(cache, "invalid type", TypeError, ass->var);
                    }
                    if (cache->current_function == NULL) {
                        ass->scope = cache->cache_size - 1;
                    } else {
                        ass->scope = -1;
                    }
                    break;
                }
                GenericDT *var_datatype;
                int scope;
                analysis_cache_get(cache, ass->var, &var_datatype, &scope);
                ass->datatype = var_datatype;
                if (var_datatype == NULL) {
                    analysis_cache_add_error(cache, "undefined variable", ReferenceError, ass->var);
                } else if (!generic_datatype_compare(exp_datatype, var_datatype)) {
                    analysis_cache_add_error(cache, "invalid type", TypeError, ass->var);
                }
                if (cache->current_function == NULL) {
                    ass->scope = scope;
                } else {
                    ass->scope = -1;
                }
                break;
            }
            }
        }
        }
        break;
    }
    case CallOL: {
        Call *call = oneliner->data.call;
        GenericDT *datatype;
        int scope;
        analysis_cache_get(cache, call->call_name, &datatype, &scope);
        call->datatype = datatype;
        int is_defined = datatype != NULL;
        int is_a_function = is_defined && datatype->type != Simple;
        int returns_void = is_a_function && datatype->data.fn_datatype->return_type->type == Simple &&
                           datatype->data.fn_datatype->return_type->data.simple_datatype == Void;
        if (!is_defined) {
            analysis_cache_add_error(cache, "undefined function", ReferenceError, call->call_name);
        } else if (!is_a_function) {
            analysis_cache_add_error(cache, "is not a function", TypeError, call->call_name);
        } else if (!returns_void) {
            analysis_cache_add_error(cache, "void call returns a value", TypeError, call->call_name);
        } else if (cache->current_function == NULL) {
            call->scope = cache->cache_size - 1;
        } else {
            call->scope = -1;
        }

        int is_args_count_valid = is_a_function && call->args_size == datatype->data.fn_datatype->params_size;

        for (int i = 0; i < call->args_size; i++) {
            GenericDT *arg_datatype;
            analysis_cache_process_expression(cache, call->args[i], &arg_datatype);
            if (is_args_count_valid && !generic_datatype_compare(arg_datatype, datatype->data.fn_datatype->params[i]->datatype)) {
                analysis_cache_add_error(cache, "wrong parameter type for the function", TypeError, call->call_name);
            }
        }

        break;
    }
    }
}

void validate(AnalysisCache *cache, Stmt **stmts, size_t stmts_size) {
    for (int i = 0; i < stmts_size; i++) {
        Stmt *stmt = stmts[i];
        switch (stmt->type) {
        case OpenScopeStmt:
            analysis_cache_extend(cache);
            break;
        case CloseScopeStmt:
            analysis_cache_shrink(cache);
            break;
        case BreakStmt:
            if (!cache->in_loop) {
                analysis_cache_add_error(cache, "break statement outside of a loop", SyntaxError, stmt->data.break_cmd->token);
            }
            break;
        case ContinueStmt:
            if (!cache->in_loop) {
                analysis_cache_add_error(cache, "continue statement outside of a loop", SyntaxError, stmt->data.continue_cmd->token);
            }
            break;
        case ReturnStmt: {
            ReturnCmd *rtrn = stmt->data.return_cmd;
            if (cache->current_function == NULL) {
                analysis_cache_add_error(cache, "return statement outside of a function body", SyntaxError, stmt->data.return_cmd->token);
            } else if (stmt->data.return_cmd->exp != NULL) {
                GenericDT *return_type;
                analysis_cache_process_expression(cache, stmt->data.return_cmd->exp, &return_type);
                if (!generic_datatype_compare(cache->current_function->return_type, return_type)) {
                    analysis_cache_add_error(cache, "returning wrong type", TypeError, stmt->data.return_cmd->token);
                }
            }
            break;
        }
        case OnelinerStmt: {
            analysis_cache_process_oneliner(cache, stmt->data.oneliner);
            break;
        }
        case ConditionalStmt: {
            Conditional *cond = stmt->data.conditional;
            GenericDT *condition_datatype;
            analysis_cache_process_expression(cache, cond->condition, &condition_datatype);
            if (condition_datatype->type != Simple || condition_datatype->data.simple_datatype != Bool) {
                analysis_cache_add_error(cache, "condition must be a boolean expression", TypeError, cond->token);
            }
            if (cond->then_size) {
                analysis_cache_extend(cache);
                validate(cache, cond->then_block, cond->then_size);
                analysis_cache_shrink(cache);
            }
            if (cond->else_size) {
                analysis_cache_extend(cache);
                validate(cache, cond->else_block, cond->else_size);
                analysis_cache_shrink(cache);
            }
            break;
        }
        case ForStmt: {
            ForLoop *loop = stmt->data.for_loop;
            GenericDT *cond_datatype;
            analysis_cache_extend(cache);
            analysis_cache_process_oneliner(cache, loop->init);
            analysis_cache_process_expression(cache, loop->condition, &cond_datatype);
            if (cond_datatype->type != Simple || cond_datatype->data.simple_datatype != Bool) {
                analysis_cache_add_error(cache, "condition must be a boolean expression", TypeError, loop->token);
            }
            analysis_cache_process_oneliner(cache, loop->after);
            if (loop->body_size) {
                analysis_cache_extend(cache);
                cache->in_loop++;
                validate(cache, loop->body, loop->body_size);
                analysis_cache_shrink(cache);
                cache->in_loop--;
            }
            analysis_cache_shrink(cache);
            break;
        }
        case FnStmt: {
            FnDefinition *fn = stmt->data.fn_def;
            int fn_is_redefined = 0;
            int is_invalid = 0;
            {
                GenericDT *defined_var_datatype;
                int scope;
                analysis_cache_get(cache, fn->name, &defined_var_datatype, &scope);
                if (defined_var_datatype != NULL) {
                    fn_is_redefined = 1;
                    analysis_cache_add_error(cache, "variable redefinition is not allowed", ReferenceError, fn->name);
                }
            }

            analysis_cache_extend(cache);
            for (int i = 0; i < fn->datatype->params_size; i++) {
                int param_is_redefined = analysis_cache_defined_in_current_scope(cache, fn->datatype->params[i]->name);
                if (param_is_redefined) {
                    analysis_cache_add_error(cache, "parameter with the same name already exists for given function", ReferenceError,
                                             fn->datatype->params[i]->name);
                } else {
                    analysis_cache_set(cache, fn->datatype->params[i]->name, fn->datatype->params[i]->datatype, -1);
                }
            }
            if (is_invalid) {
                fn->datatype = NULL;
            }
            if (!fn_is_redefined) {
                GenericDT *datatype = generic_datatype_create();
                datatype->type = Complex;
                datatype->data.fn_datatype = fn->datatype;
                analysis_cache_set(cache, fn->name, datatype, cache->cache_size - 2);
            }

            cache->current_function = fn->datatype;
            if (fn->body_size) {
                validate(cache, fn->body, fn->body_size);
            }

            analysis_cache_shrink(cache);
            cache->current_function = NULL;
            break;
        }
        }
    }
}
