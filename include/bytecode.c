#include "bytecode.h"
#include <stdio.h>

void bytecode_visualize(OpCode *commands, Constant *args, size_t program_size) {
    printf("\nVisualizing bytecode\n\n");
    for (int i = 0; i < program_size; i++) {
        printf("%d: ", i);
        switch (commands[i]) {
        case CreateScopeCode:
            printf("CSCOPE\n");
            break;
        case DestroyScopeCode:
            printf("DSCOPE\n");
            break;
        case GotoCode:
            printf("GOTO %d\n", args[i].int_data);
            break;
        case GotoIfCode:
            printf("GOTO_IF %d\n", args[i].int_data);
            break;
        case PushCode:
            printf("PUSH %d\n", args[i].int_data);
            break;
        case LoadCode:
            printf("LOAD with offset %d\n", args[i].int_data);
            break;
        case StoreCode:
            printf("STORE with offset %d\n", args[i].int_data);
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
