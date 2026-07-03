#ifndef BYTECODE_H
#define BYTECODE_H

#define OP_PUSH               0x01 // put onto next use stack A
#define OP_POP                0x02 // discard top of stack
#define OP_DUP                0x03 // duplicate top
#define OP_SWAP               0x04 // swap top two
#define OP_OVER               0x05 // copy second item

#define OP_OPERATE_MUL        0x11 // mul -2, -1 -> -1
#define OP_OPERATE_DIV        0x12 // div -2, -1 -> -1
#define OP_OPERATE_ADD        0x13 // add -2, -1 -> -1
#define OP_OPERATE_SUB        0x14 // sub -2, -1 -> -1
#define OP_OPERATE_EXP        0x15 // exp -2, -1 -> -1

#define OP_STORE              0x21 // store top into slot given
#define OP_LOAD               0x22 // load from slot given into top

#define OP_CONST_LOAD         0x41 // takes slot from A and gets associated const into R

#define OP_CALL_NATIVE        0x51 // calls native command 

#define OP_JUMP               0x61 // jump to position of uint32

#define OP_CALL               0x71 // jump to position of uint32 AND add to call stack
#define OP_RETURN             0x72 // remove from call stack

#define OP_FINISH             0xFF // EOF marker

#endif
