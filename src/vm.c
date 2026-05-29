#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STACK_MAX 256
#define GLOBAL_MAX 256

typedef struct {
    int asInt;
} Value;

typedef struct {
    uint8_t *code;
    int ip;

    Value stack[STACK_MAX];
    int sp;

    Value globals[GLOBAL_MAX];
} VM;

VM vmStd = {0};
VM *vm;

static void push(Value v) {
    if (vm->sp >= STACK_MAX) {
        printf("Stack overflow\n");
        exit(1);
    }
    vm->stack[vm->sp++] = v;
}

static Value pop() {
    if (vm->sp <= 0) {
        printf("Stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->sp];
}

static uint8_t readByte() {
    return vm->code[vm->ip++];
}

void runVM(ByteCodeResult bc) {
    vmStd.code = bc.data;
    vmStd.ip = 0;
    vmStd.sp = 0;
    vm = &vmStd;

    while (1) {
        uint8_t op = readByte();

        switch (op) {
            case OP_PUSH_CONST: {
                Value v;
                v.asInt = readByte();
                push(v);
                break;
            }

            case OP_ADD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.asInt = a.asInt + b.asInt;
                push(r);
                break;
            }
            case OP_SUB: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.asInt = a.asInt - b.asInt;
                push(r);
                break;
            }

            case OP_MUL: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.asInt = a.asInt * b.asInt;
                push(r);
                break;
            }

            case OP_DIV: {
                Value b = pop();
                Value a = pop();
                if (b.asInt == 0) {
                    printf("Runtime error: division by zero\n");
                    exit(1);
                }
                Value r;
                r.asInt = a.asInt / b.asInt;
                push(r);
                break;
            }

            case OP_STORE_GLOBAL: {
                uint8_t slot = readByte();
                vm->globals[slot] = pop();
                break;
            }

            case OP_LOAD_GLOBAL: {
                uint8_t slot = readByte();
                push(vm->globals[slot]);
                break;
            }

            case OP_HALT:
                return;

            default:
                printf("Unknown opcode: 0x%02X at %d\n", op, vm->ip - 1);
                exit(1);
        }
    }
}
