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

// ===================== STACK OPS =====================

static void push(VM *vm, Value v) {
    if (vm->sp >= STACK_MAX) {
        printf("Stack overflow\n");
        exit(1);
    }
    vm->stack[vm->sp++] = v;
}

static Value pop(VM *vm) {
    if (vm->sp <= 0) {
        printf("Stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->sp];
}

// ===================== BYTE READ =====================

static uint8_t readByte(VM *vm) {
    return vm->code[vm->ip++];
}

// ===================== VM EXECUTION =====================
void runVM(ByteCodeResult bc) {

    VM vm = {0};
    vm.code = bc.data;
    vm.ip = 0;
    vm.sp = 0;

    for (;;) {

        uint8_t op = readByte(&vm);

        switch (op) {

            case OP_PUSH_CONST: {
                Value v;
                v.asInt = readByte(&vm);
                push(&vm, v);
                break;
            }

            case OP_ADD: {
                Value b = pop(&vm);
                Value a = pop(&vm);

                Value r;
                r.asInt = a.asInt + b.asInt;

                push(&vm, r);
                break;
            }

            case OP_SUB: {
                Value b = pop(&vm);
                Value a = pop(&vm);

                Value r;
                r.asInt = a.asInt - b.asInt;

                push(&vm, r);
                break;
            }

            case OP_MUL: {
                Value b = pop(&vm);
                Value a = pop(&vm);

                Value r;
                r.asInt = a.asInt * b.asInt;

                push(&vm, r);
                break;
            }

            case OP_DIV: {
                Value b = pop(&vm);
                Value a = pop(&vm);

                if (b.asInt == 0) {
                    printf("Runtime error: division by zero\n");
                    exit(1);
                }

                Value r;
                r.asInt = a.asInt / b.asInt;

                push(&vm, r);
                break;
            }

            case OP_STORE_GLOBAL: {
                uint8_t slot = readByte(&vm);
                vm.globals[slot] = pop(&vm);
                break;
            }

            case OP_LOAD_GLOBAL: {
                uint8_t slot = readByte(&vm);
                push(&vm, vm.globals[slot]);
                break;
            }

            case OP_HALT:
                return;

            default:
                printf("Unknown opcode: 0x%02X at %d\n", op, vm.ip - 1);
                exit(1);
        }
    }
}