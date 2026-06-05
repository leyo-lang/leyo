#ifndef BYTECODE_H
#define BYTECODE_H

#define OP_PUT_A              0x01 // put onto next use stack A
#define OP_PUT_B              0x02 // put onto next use stack B
#define OP_PUT_S              0x03 // swap A and B around

#define OP_PUT_A_R            0x04 // put R onto next use stack A
#define OP_PUT_B_R            0x05 // put R onto next use stack B

#define OP_OPERATE_MUL        0x11 // mul A B -> R
#define OP_OPERATE_DIV        0x12 // div A B -> R
#define OP_OPERATE_ADD        0x13 // add A B -> R
#define OP_OPERATE_SUB        0x14 // sub A B -> R
#define OP_OPERATE_EXP        0x15 // exp A B -> R

#define OP_STORE              0x21 // store A into slot B
#define OP_LOAD               0x22 // load from slot A into R

#define OP_SS_PUSH_A          0x31 // push A onto speed stack (element -1)
#define OP_SS_PUSH_B          0x32 // push B onto speed stack (element -1)
#define OP_SS_POP             0x33 // pop from speed stack into R (element -1)

#define OP_CONST_LOAD         0x41 // takes slot from A and gets associated const into R

#define OP_FINISH             0xFF // EOF marker

#endif