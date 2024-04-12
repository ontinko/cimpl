#include "bytecode.h"
#include <stdio.h>

void bytecode_visualize(OpCode *commands, size_t commands_size, Constant *args, size_t args_size, int *ref_scopes, size_t ref_scopes_size) {
    printf("\nVisualizing bytecode\n\n");
    size_t args_count = 0;
    size_t scopes_count = 0;
    for (int i = 0; i < commands_size; i++) {
        switch (commands[i]) {
        case CreateScopeCode:
            printf("CSCOPE\n");
            break;
        case DestroyScopeCode:
            printf("DSCOPE\n");
            break;
        case PushCode:
            printf("PUSH %d\n", args[args_count].int_data);
            args_count++;
            break;
        case LoadCode:
            printf("LOAD %s\n", args[args_count].var_name);
            args_count++;
            break;
        case StoreCode:
            printf("STORE %s to %d\n", args[args_count].var_name, ref_scopes[scopes_count]);
            args_count++;
            scopes_count++;
            break;
        case ReturnCode:
            printf("RETURN\n");
            break;
        case IntAddCode:
            printf("ADD\n");
            break;
        case IntSubtractCode:
            printf("SUBTRACT\n");
            break;
        case IntMultiplyCode:
            printf("MUL\n");
            break;
        case IntDivideCode:
            printf("DIV\n");
            break;
        case IntModCode:
            printf("MOD\n");
            break;
        case IntEqCode:
            printf("EQ\n");
            break;
        case IntNotEqCode:
            printf("NOT_EQ\n");
            break;
        case IntGtCode:
            printf("GT\n");
            break;
        case IntLtCode:
            printf("LT\n");
            break;
        case IntGtECode:
            printf("GT_E\n");
            break;
        case IntLtECode:
            printf("LT_E\n");
            break;
        case BoolNotCode:
            printf("NOT\n");
            break;
        case BoolAndCode:
            printf("AND\n");
            break;
        case BoolOrCode:
            printf("OR\n");
            break;
        }
    }
}
