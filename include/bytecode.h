#ifndef BYTECODE_H
#define BYTECODE_H
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    ShiftStackCode,
    PushCode,
    LoadCode,
    ReturnCode,
    StoreCode,
    CallCode,

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

    GotoIfCode,
    GotoCode,
    ResumeCode,

    PrintlnIntCode,
    PrintlnBoolCode,
    PrintlnStrCode,

    EndCode,
} OpCode;

// cond := true;
// if cond {
//     cond = false;
// } else {
//     cond = true;
// }
// answer := 42;
//
// 0: push true
// 1: store cond
//
// 2: load cond
// 3: goto_if 5
// 4: goto 10
// 5: cscope
// 6: push false
// 7: store cond
// 8: dscope
// 9: goto end
// 10: cscope
// 11: push true
// 12: store cond
// 13: dscope
// 14: push 42
// 15: store answer
//
//
//
// while a == 1 {
//     b := 1;
//     a = b + 1;
// } else {
//     a = 0;
// }
// final := 42;
//
// 0: load a
// 1: push 1
// 2: eq
// 3: goto_if 10
// 4: goto 18
// 5: load a
// 6: push 1
// 7: eq
// 8: goto_if 10
// 9: goto 22
// 10: cscope
// 11: push 1
// 12: store b
// 13: load b
// 14: push 1
// 15: store a
// 16: dscope
// 17: goto 8
// 18: cscope
// 19: push 0
// 20: store a
// 21: dscope
//
// 22: push 42
// 23: store final
//
//
//
// total := 0;
// for i := 0; i < 10; i++ {
//     total += 3;
// }
// result := total + 1;
//
// 0: cscope
// 1: push 0
// 2: store i
// 3: load i
// 4: push 10
// 5: int_lt
// 6: goto_if 8
// 7: goto 18
// 8: cscope
// 9: load total
// 10: push 3
// 11: int_add
// 12: store total
// 13: load i
// 14: push 1
// 15: int_add
// 16: store i
// 17: goto 4
// 18: dscope
// 19: dscope
// 20: load total
// 21: push 1
// 22: int_add
// 23: store result
//
//
// fn sum(a:int, b:int) int {
//     return a + b;
// }
// result := sum(1, 2);
//
//
// 0: load 1
// 1: load 1
// 2: add
// 3: return 0
// 4: cscope
// 5: push 1
// 6: store arg_offset1
// 7: push 2
// 8: store arg_offset2
// 9: goto 0
// 10: store result

typedef union {
    int int_data;
    char *string_data;
} Constant;

void bytecode_visualize(OpCode *commands, Constant *args, size_t program_size);

#endif
