#ifndef BYTECODE_H
#define BYTECODE_H
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    CreateScopeCode,
    DestroyScopeCode,
    PushCode,
    LoadCode,
    ReturnCode,

    StoreCode,

    IntAddCode,
    IntSubtractCode,
    IntMultiplyCode,
    IntDivideCode,
    IntModCode,
    IntEqCode,
    IntNotEqCode,
    IntGtCode,
    IntLtCode,
    IntGtECode,
    IntLtECode,

    BoolNotCode,
    BoolAndCode,
    BoolOrCode,
} OpCode;

typedef enum { IntConstant, BoolConstant } ConstantType;

typedef union {
    int int_data;
    uint8_t bool_data;
    char *var_name;
} Constant;

void bytecode_visualize(OpCode *commands, size_t commands_size, Constant *args, size_t args_size, int *ref_scopes, size_t ref_scopes_size);

#endif
