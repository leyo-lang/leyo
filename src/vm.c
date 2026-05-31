#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SPEED_STACK_MAX 256
#define GLOBALS_MAX 65536

typedef int Value;


typedef struct {
    uint8_t *code;
    int ip;

    Value A;
    Value B;
    Value R;

    Value speedStack[SPEED_STACK_MAX];
    int speedTop;

    Value globals[GLOBALS_MAX];
    
    
} VM;

VM vmStd = {0};
VM *vm;



static uint8_t readByte(void) {
    return vm->code[vm->ip++];
}

static void advanceByte(void) {
    vm->ip++;
}

int runVM(ByteCodeResult bc) {
    logRuntime("Starting VM execution");
    vmStd.code = bc.data;
    vmStd.ip = 0;
    vm = &vmStd;

    while (1) {
        uint8_t op = readByte();
        advanceByte();

        switch (op) {
            case OP_PUT_A:
                vm->A = readByte();

            case OP_PUT_B:
                vm->B = readByte();

            case OP_PUT_A_R:
                vm->A = vm->R;

            case OP_PUT_B_R:
                vm->B = vm->R;

            case OP_FINISH:
                return 0;

            default:
                printf("Unknown opcode: 0x%02X at %d\n", op, vm->ip - 1);
                exit(1);
        }
    }

    raise("Exited Early", vm->ip, 0);
    callAllErr();
    return 1;
}
