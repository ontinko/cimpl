#ifndef AST_H
#define AST_H
#include "token.h"

typedef enum { CallOL, AssignmentOL } OnelinerType;

typedef enum { OnelinerStmt, ConditionalStmt, ForStmt, FnStmt, BreakStmt, ContinueStmt, OpenScopeStmt, CloseScopeStmt, ReturnStmt } StmtType;

typedef enum { ExpExp, FnCallExp } ExpType;

typedef enum { Bool, Int, Void } DataType;

typedef enum { Simple, Complex } VarType;

typedef struct Stmt Stmt;

typedef struct FnParam FnParam;

typedef struct FunctionType FunctionType;

typedef union ExpUnion ExpUnion;

typedef union {
    DataType simple_datatype;
    FunctionType *fn_datatype;
} DTUnion;

typedef struct {
    VarType type;
    DTUnion data;
} GenericDT;

GenericDT *generic_datatype_create();

struct FunctionType {
    FnParam **params;
    size_t params_size;
    GenericDT *return_type;
};

FunctionType *function_type_create();

typedef struct {
    GenericDT *datatype;
    Token *token;
    int scope;
    struct Expression *left;
    struct Expression *right;
} OpExpression;

OpExpression *op_expression_create();

typedef struct {
    GenericDT *datatype;
    Token *call_name;
    struct Expression **args;
    size_t args_size;
    int scope;
} Call;

Call *call_create();

union ExpUnion {
    OpExpression *exp;
    Call *fn_call;
};

typedef struct Expression {
    ExpType type;
    ExpUnion data;
} Expression;

Expression *expression_create();

typedef struct {
    Token *var;
    GenericDT *datatype;
    int new_var;
    Token *op;
    Expression *exp;
    int scope;
} Assignment;

Assignment *assignment_create();

struct FnParam {
    GenericDT *datatype;
    Token *name;
};

FnParam *fn_param_create();

typedef struct {
    Token *name;
    FunctionType *datatype;
    Stmt **body;
    size_t body_size;
} FnDefinition;

FnDefinition *fn_definition_create();

typedef union {
    Assignment *assignment;
    Call *call;
} OnelinerUnion;

typedef struct {
    OnelinerType type;
    OnelinerUnion data;
} Oneliner;

Oneliner *oneliner_create();

typedef struct ForLoop {
    Token *token;
    Oneliner *init;
    Expression *condition;
    Oneliner *after;
    Stmt **body;
    size_t body_size;
} ForLoop;

ForLoop *for_loop_create();

typedef struct {
    Token *token;
    Expression *condition;
    Stmt **then_block;
    Stmt **else_block;
    size_t then_size;
    size_t else_size;
} Conditional;

Conditional *conditional_create();

typedef struct {
    Token *token;
} BreakCmd;

BreakCmd *break_cmd_create();

typedef struct {
    Token *token;
} ContinueCmd;

ContinueCmd *continue_cmd_create();

typedef struct {
    Token *token;
} OpenScopeCmd;

OpenScopeCmd *open_scope_cmd_create();

typedef struct {
    Token *token;
} CloseScopeCmd;

CloseScopeCmd *close_scope_cmd_create();

typedef struct {
    Token *token;
    Expression *exp;
} ReturnCmd;

ReturnCmd *return_cmd_create();

typedef union {
    ForLoop *for_loop;
    Conditional *conditional;
    Oneliner *oneliner;
    FnDefinition *fn_def;
    BreakCmd *break_cmd;
    ContinueCmd *continue_cmd;
    OpenScopeCmd *open_scope_cmd;
    CloseScopeCmd *close_scope_cmd;
    ReturnCmd *return_cmd;
} StmtUnion;

struct Stmt {
    StmtType type;
    StmtUnion data;
};

Stmt *stmt_create();

static void generic_datatype_view(GenericDT *datatype, char *source);

int generic_datatype_compare(GenericDT *first, GenericDT *second);

void visualize_program(Stmt **stmts, size_t stmts_size, int tab_size, char *source);

static void visualize_expression(Expression *exp, char *source);
#endif
