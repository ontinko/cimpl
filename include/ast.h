#ifndef AST_H
#define AST_H
#include "token.h"

typedef enum { CallOL, AssignmentOL, PrintlnOL } OnelinerType;

typedef enum { OnelinerStmt, ConditionalStmt, ForStmt, FnStmt, BreakStmt, ContinueStmt, OpenScopeStmt, CloseScopeStmt, ReturnStmt } StmtType;

typedef enum { ExpExp, FnCallExp } ExpType;

typedef enum { Bool, Int, String, Void } DataType;

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
    FnParam *params;
    size_t params_size;
    GenericDT *return_type;
};

void function_type_init(FunctionType *fn_type);

typedef struct {
    GenericDT *datatype;
    Token *token;
    int scope;
    struct Expression *left;
    struct Expression *right;
} OpExpression;

void op_expression_init(OpExpression *exp);

typedef struct {
    GenericDT *datatype;
    Token *call_name;
    struct Expression *args;
    size_t args_size;
    int scope;
} Call;

void call_init(Call *call);

union ExpUnion {
    OpExpression *exp;
    Call *fn_call;
};

typedef struct Expression {
    ExpType type;
    ExpUnion data;
} Expression;

typedef struct {
    Token *var;
    GenericDT *datatype;
    int new_var;
    Token *op;
    Expression *exp;
    int scope;
} Assignment;

void assignment_init(Assignment *ass);

struct FnParam {
    GenericDT *datatype;
    Token *name;
};

void fn_param_init(FnParam *param);

typedef struct {
    Token *name;
    FunctionType *datatype;
    Stmt *body;
    size_t body_size;
} FnDefinition;

void fn_definition_init(FnDefinition *fn_def);

// TODO: add initializers
typedef struct {
    Token *token;
    Expression *exp;
} PrintlnCmd;

typedef union {
    Assignment *assignment;
    Call *call;
    PrintlnCmd *println;
} OnelinerUnion;

typedef struct {
    OnelinerType type;
    OnelinerUnion data;
} Oneliner;

typedef struct ForLoop {
    Token *token;
    Oneliner *init;
    Expression *condition;
    Oneliner *after;
    Stmt *body;
    size_t body_size;
} ForLoop;

void for_loop_init(ForLoop *loop);

typedef struct {
    Token *token;
    Expression *condition;
    Stmt *then_block;
    Stmt *else_block;
    size_t then_size;
    size_t else_size;
} Conditional;

void conditional_init(Conditional *cond);

typedef struct {
    Token *token;
} BreakCmd;

void break_cmd_init(BreakCmd *cmd);

typedef struct {
    Token *token;
} ContinueCmd;

void continue_cmd_init(ContinueCmd *cmd);

typedef struct {
    Token *token;
} OpenScopeCmd;

void open_scope_cmd_init(OpenScopeCmd *cmd);

typedef struct {
    Token *token;
} CloseScopeCmd;

void close_scope_cmd_init(CloseScopeCmd *cmd);

typedef struct {
    Token *token;
    Expression *exp;
} ReturnCmd;

void return_cmd_init(ReturnCmd *cmd);

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

static void generic_datatype_view(GenericDT *datatype, char *source);

int generic_datatype_compare(GenericDT *first, GenericDT *second);

void visualize_program(Stmt *stmts, size_t stmts_size, int tab_size, char *source);

static void visualize_expression(Expression *exp, char *source);
#endif
