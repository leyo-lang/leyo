#ifndef BYTECODE_H
#define BYTECODE_H


#define OP_PUSH_CONST      0x01

#define OP_ADD             0x10
#define OP_SUB             0x11
#define OP_MUL             0x12
#define OP_DIV             0x13

#define OP_STORE_GLOBAL    0x20
#define OP_LOAD_GLOBAL     0x21

#define OP_JMP             0x30
#define OP_JMP_IF_FALSE    0x31

#define OP_EQ              0x40
#define OP_NEQ             0x41
#define OP_GT              0x42
#define OP_LT              0x43

#define OP_CALL_NATIVE     0x50

#define OP_CALL            0x60
#define OP_RETURN          0x61

#define OP_HALT            0xFF


#endif