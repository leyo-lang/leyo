#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define SPEED_STACK_MAX 256
#define GLOBALS_MAX 65536

typedef uint16_t Value;


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

static void dumpState(uint8_t op) {
    char buf[512];

    snprintf(buf, sizeof(buf),
        "OP=0x%02X | A=%d B=%d R=%d | ip=%d | stackTop=%d",
        op,
        vm->A,
        vm->B,
        vm->R,
        vm->ip,
        vm->speedTop
    );

    logRuntime(buf);
}

static uint8_t readByte(void) {
    return vm->code[vm->ip];
}

static void advanceByte(void) {
    vm->ip++;
}

static uint16_t read16(void) {
    uint16_t low = readByte();
    advanceByte();
    uint16_t high = readByte();
    

    return low | (high << 8);
}

int runVM(ByteCodeResult bc) {
    logRuntime("Starting VM execution");
    vmStd.code = bc.data;
    vmStd.ip = 0;
    vm = &vmStd;

    while (1) {
        uint8_t op = readByte();

        dumpState(op);

        advanceByte();

        switch (op) {
            case OP_PUT_A:
                vm->A = read16();
                advanceByte();
                break;

            case OP_PUT_B:
                vm->B = read16();
                advanceByte();
                break;

            case OP_PUT_S: {
                Value tmp = vm->B;
                vm->B = vm->A;
                vm->A = tmp;
                break;
            }

            case OP_PUT_A_R:
                vm->A = vm->R;
                break;

            case OP_PUT_B_R:
                vm->B = vm->R;
                break;

            case OP_OPERATE_ADD:
                vm->R = vm->A + vm->B;
                break;

            case OP_OPERATE_SUB:
                vm->R = vm->A - vm->B;
                break;

            case OP_OPERATE_MUL:
                vm->R = vm->A * vm->B;
                break;

            case OP_OPERATE_DIV:
                vm->R = vm->A / vm->B;
                break;

            case OP_OPERATE_EXP:
                vm->R = pow(vm->A, vm->B);
                break;

            case OP_STORE:
                vm->globals[vm->B] = vm->A;
                break;

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
