#ifndef AST_H
#define AST_H
#include "token.h"

typedef enum { CallOL, AssignmentOL } OnelinerType;

typedef enum {
    OnelinerStmt,
    ConditionalStmt,
    ForStmt,
    DefStmt,
    BreakStmt,
    ContinueStmt,
    OpenScopeStmt,
    CloseScopeStmt,
    ReturnStmt
} StmtType;

typedef enum { ExpExp, FnCallExp } ExpType;

typedef enum { Bool, Int, Void } DataType;

typedef struct Stmt Stmt;

typedef union ExpUnion ExpUnion;

typedef struct Expression {
    ExpType type;
    ExpUnion *exp;
} Expression;

typedef struct {
    DataType datatype;
    Token *token;
    struct Expression *left;
    struct Expression *right;
} OpExpression;

typedef struct {
    DataType datatype;
    Token *call_name;
    Expression **args;
    size_t args_size;
} Call;

union ExpUnion {
    OpExpression exp;
    Call fn_call;
};

typedef struct {
    Token *var;
    DataType datatype;
    int new_var;
    Token *op;
    Expression *exp;
} Assignment;

typedef struct {
    DataType datatype;
    Token *name;
} DefParam;

typedef struct { // statement
    Token *name;
    DataType datatype;
    DefParam **params;
    size_t params_size;
    Stmt **body;
    size_t body_size;
} FnDef;

typedef union {
    Assignment assignment;
    Call call;
} OnelinerUnion;

typedef struct { // statement
    OnelinerType type;
    OnelinerUnion *stmt;
} Oneliner;

typedef struct ForLoop { // statement
    Oneliner *init;
    Expression *condition;
    Oneliner *after;
    Stmt **body;
    size_t body_size;
} ForLoop;

typedef struct { // statement
    Token *token;
    Expression *condition;
    Stmt **then_block;
    Stmt **else_block;
    size_t then_size;
    size_t else_size;
} Conditional;

typedef struct {
    Token *token;
} BreakCmd;

typedef struct {
    Token *token;
} ContinueCmd;

typedef struct {
    Token *token;
} OpenScopeCmd;

typedef struct {
    Token *token;
} CloseScopeCmd;

typedef struct {
    Token *token;
    Expression *exp;
} ReturnCmd;

typedef union {
    ForLoop for_loop;
    Conditional conditional;
    Oneliner oneliner;
    FnDef fn_def;
    BreakCmd break_cmd;
    ContinueCmd continue_cmd;
    OpenScopeCmd open_scope_cmd;
    CloseScopeCmd close_scope_cmd;
    ReturnCmd return_cmd;
} StmtUnion;

struct Stmt {
    StmtType type;
    StmtUnion *stmt;
};
#endif
