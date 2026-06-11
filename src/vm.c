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

// extern struct Value;

typedef struct {
    uint8_t *code;
    int ip;

    Value A;
    Value B;
    Value R;

    Value speedStack[SPEED_STACK_MAX];
    int speedTop;

    Value globals[GLOBALS_MAX];
    
    Value *consts;
} VM;

VM vmStd = {0};
VM *vm;

static void dumpState(uint8_t op) {
    char buf[512];

    snprintf(buf, sizeof(buf),
        "OP=0x%02X | A=%d B=%d R=%d | ip=%d | stackTop=%d",
        op,
        vm->A.as.i,
        vm->B.as.i,
        vm->R.as.i,
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

static void addition(void) {
    switch (vm->A.flag) {
        case VAL_CHAR:
            raise("Char addition unsupported", vm->ip, 0);
            callAllErr(); 
            break;
        
        case VAL_STR:
            raise("String addition unsupported", vm->ip, 0);
            callAllErr(); 
            /*vm->R.as.s = vm->A.as.s;
            strcat(vm->R.as.s,vm->B.as.s);
            vm->R.flag = VAL_STR;*/
            break;

        case VAL_INT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.i = vm->A.as.i + vm->B.as.i;
                vm->R.flag = VAL_INT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.i + vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot add text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        case VAL_FLOAT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.f = (double)(vm->A.as.f + vm->B.as.i);
                vm->R.flag = VAL_FLOAT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.f + vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot add text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        default:
            break;
    }
}

static void subtraction(void) {
    switch (vm->A.flag) {
        case VAL_CHAR:
            raise("Char subtraction unsupported", vm->ip, 0);
            callAllErr(); 
            break;
        
        case VAL_STR:
            raise("String subtraction unsupported", vm->ip, 0);
            callAllErr(); 
            break;

        case VAL_INT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.i = vm->A.as.i - vm->B.as.i;
                vm->R.flag = VAL_INT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.i - vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot subtract text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        case VAL_FLOAT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.f = (double)(vm->A.as.f - vm->B.as.i);
                vm->R.flag = VAL_FLOAT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.f - vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot add text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        default:
            break;
    }
}

static void multiplication(void) {
    switch (vm->A.flag) {
        case VAL_CHAR:
            raise("Char multiplication unsupported", vm->ip, 0);
            callAllErr(); 
            break;
        
        case VAL_STR:
            raise("String multiplication unsupported", vm->ip, 0);
            callAllErr(); 
            break;

        case VAL_INT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.i = vm->A.as.i * vm->B.as.i;
                vm->R.flag = VAL_INT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.i * vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot multiply text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        case VAL_FLOAT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.f = (double)(vm->A.as.f * vm->B.as.i);
                vm->R.flag = VAL_FLOAT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.f * vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot multiply text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        default:
            break;
    }
}

static void division(void) {
    switch (vm->A.flag) {
        case VAL_CHAR:
            raise("Char division unsupported", vm->ip, 0);
            callAllErr(); 
            break;
        
        case VAL_STR:
            raise("String division unsupported", vm->ip, 0);
            callAllErr(); 
            break;

        case VAL_INT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.i = vm->A.as.i / vm->B.as.i;
                vm->R.flag = VAL_INT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.i / vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot divide text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        case VAL_FLOAT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.f = (double)(vm->A.as.f / vm->B.as.i);
                vm->R.flag = VAL_FLOAT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)(vm->A.as.f / vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot divide text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        default:
            break;
    }
}

static void power() {
    switch (vm->A.flag) {
        case VAL_CHAR:
            raise("Char expodents unsupported", vm->ip, 0);
            callAllErr(); 
            break;
        
        case VAL_STR:
            raise("String expodents unsupported", vm->ip, 0);
            callAllErr(); 
            break;

        case VAL_INT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.i = pow(vm->A.as.i , vm->B.as.i);
                vm->R.flag = VAL_INT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)pow(vm->A.as.i , vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot expodent text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        case VAL_FLOAT:
            if (vm->B.flag == VAL_INT) {
                vm->R.as.f = (double)(pow(vm->A.as.f, vm->B.as.i));
                vm->R.flag = VAL_FLOAT;
                break;
            } else if (vm->B.flag == VAL_FLOAT) {
                vm->R.as.f = (double)pow(vm->A.as.f , vm->B.as.f);
                vm->R.flag = VAL_FLOAT;
                break;
            } else {
                raise("Cannot expodent text type (chr+str) to number type", vm->ip, 0);
                callAllErr();
                break;
            }
            break;

        default:
            break;
    }
}

int runVM(ByteCodeResult bc) {
    logRuntime("Starting VM execution");
    vmStd.code = bc.data;
    vmStd.ip = 0;
    //vmStd.consts = bc;
    vm = &vmStd;

    while (1) {
        uint8_t op = readByte();

        dumpState(op);

        advanceByte();

        switch (op) {
            case OP_PUT_A:
                vm->A.as.i = read16();
                vm->A.flag = VAL_INT;
                advanceByte();
                break;

            case OP_PUT_B:
                vm->B.as.i = read16();
                vm->A.flag = VAL_INT;
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
                addition();
                break;

            case OP_OPERATE_SUB:
                subtraction();
                break;

            case OP_OPERATE_MUL:
                multiplication();
                break;

            case OP_OPERATE_DIV:
                division();
                break;

            case OP_OPERATE_EXP:
                power();
                break;

            case OP_STORE:
                vm->globals[vm->B.as.i] = vm->A;
                break;

            case OP_CONST_LOAD:
                break;
                if (vm->A.as.i > 65535) {
                    raise("MEMORY ERROR CAUGHT - const load slot too high - DANGER", 0 ,0);
                    callAllErr();
                    break;
                }
                vm->R = vm->consts[vm->A.as.i];
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
