#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include "../include/native.h"
#include "../include/disassembler.h"
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
    uint32_t rtnAdr;
} Call;

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
    int constCount;

    Call *callStack;
    int callAmt;
    int callCap;
} VM;

VM vmStd = {0};
VM *vm;

static const char *valueToString(Value v, char *buf, size_t size) {
    switch (v.flag) {
        case VAL_INT:
            snprintf(buf, size, "%d", v.as.i);
            break;

        case VAL_FLOAT:
            snprintf(buf, size, "%g", v.as.f);
            break;

        case VAL_CHAR:
            snprintf(buf, size, "'%c'", v.as.c);
            break;

        case VAL_STR:
            snprintf(buf, size, "\"%s\"", v.as.s ? v.as.s : "(null)");
            break;

        default:
            snprintf(buf, size, "<unknown>");
            break;
    }

    return buf;
}

static void dumpState(uint8_t op) {
    char buf[512];
    char aBuf[64];
    char bBuf[64];
    char rBuf[64];

    snprintf(buf, sizeof(buf),
        "OP=0x%02X | A=%s B=%s R=%s | ip=%d | stackTop=%d",
        op,
        valueToString(vm->A, aBuf, sizeof(aBuf)),
        valueToString(vm->B, bBuf, sizeof(bBuf)),
        valueToString(vm->R, rBuf, sizeof(rBuf)),
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
    advanceByte();

    return low | (high << 8);
}

static uint32_t read32(void) {
    uint32_t low = read16();
    uint32_t high = read16();

    return low | (high << 16);
}

static Value *decodeConstPool(const uint8_t *data, int length, int *outCount) {
    logRuntime("Decoding constant pool");
    Value *values = NULL;
    int count = 0;
    int pos = 0;

    while (pos < length) {
        Value v = {0};
        uint8_t type = data[pos++];

        switch (type) {
            case 0x01: {
                int64_t raw = 0;
                memcpy(&raw, &data[pos], sizeof(raw));
                pos += (int)sizeof(raw);
                v.flag = VAL_INT;
                v.as.i = (int)raw;
                break;
            }

            case 0x02: {
                double raw = 0.0;
                memcpy(&raw, &data[pos], sizeof(raw));
                pos += (int)sizeof(raw);
                v.flag = VAL_FLOAT;
                v.as.f = raw;
                break;
            }

            case 0x03:
                v.flag = VAL_CHAR;
                v.as.c = (char)data[pos++];
                break;

            case 0x04: {
                uint32_t len = 0;
                memcpy(&len, &data[pos], sizeof(len));
                pos += (int)sizeof(len);

                char *s = malloc((size_t)len + 1);
                if (!s) {
                    for (int i = 0; i < count; i++) {
                        if (values[i].flag == VAL_STR) {
                            free(values[i].as.s);
                        }
                    }
                    free(values);
                    return NULL;
                }

                memcpy(s, &data[pos], len);
                s[len] = '\0';
                pos += (int)len;

                v.flag = VAL_STR;
                v.as.s = s;
                break;
            }

            default:
                for (int i = 0; i < count; i++) {
                    if (values[i].flag == VAL_STR) {
                        free(values[i].as.s);
                    }
                }
                free(values);
                return NULL;
        }

        Value *tmp = realloc(values, sizeof(Value) * (size_t)(count + 1));
        if (!tmp) {
            if (v.flag == VAL_STR) {
                free(v.as.s);
            }
            for (int i = 0; i < count; i++) {
                if (values[i].flag == VAL_STR) {
                    free(values[i].as.s);
                }
            }
            free(values);
            return NULL;
        }

        values = tmp;
        values[count++] = v;
    }

    *outCount = count;
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Constant pool decoded: %d values", count);
        logRuntime(buffer);
    }
    return values;
}

static void freeConstPool(Value *values, int count) {
    if (!values) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (values[i].flag == VAL_STR) {
            free(values[i].as.s);
        }
    }

    free(values);
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

static void checkCallStack() {
    if (vm->callAmt >= vm->callCap-1) {
        vm->callCap *= 2;
        vm->callStack = realloc(
            vm->callStack,
            vm->callCap * sizeof(Call)
        );
    }
}

int runVM(ByteCodeResult bc) {
    logRuntime("Starting VM execution");
    vmStd.code = bc.data;
    vmStd.ip = 0;
    vmStd.consts = NULL;
    vmStd.constCount = 0;
    vmStd.callAmt = 0;
    vmStd.callCap = 64;
    vmStd.callStack = malloc(vmStd.callCap * sizeof(Call));
    vm = &vmStd;

    if (bc.cb.length > 0 && bc.cb.data != NULL) {
        vmStd.consts = decodeConstPool(bc.cb.data, bc.cb.length, &vmStd.constCount);

        if (!vmStd.consts) {
            logRuntime("Failed to decode constant pool");
            raise("Failed to decode constant pool", vm->ip, 0);
            callAllErr();
            return 1;
        }

        logRuntime("Constant pool loaded into VM");
    }

    while (1) {
        uint8_t op = readByte();

        dumpState(op);

        char buf[64];
        snprintf(buf, 64, "%s", opcode_name(op));
        logRuntime(buf);

        advanceByte();

        switch (op) {
            case OP_PUT_A: {
                vm->A.as.i = read16();
                vm->A.flag = VAL_INT;
                break;
            }

            case OP_PUT_B: {
                vm->B.as.i = read16();
                vm->B.flag = VAL_INT;
                break;
            }

            case OP_PUT_S: {
                Value tmp = vm->B;
                vm->B = vm->A;
                vm->A = tmp;
                break;
            }

            case OP_PUT_A_R: {
                vm->A = vm->R;
                break;
            }

            case OP_PUT_B_R: {
                vm->B = vm->R;
                break;
            }

            case OP_OPERATE_ADD: {
                addition();
                break;
            }

            case OP_OPERATE_SUB: {
                subtraction();
                break;
            }

            case OP_OPERATE_MUL: {
                multiplication();
                break;
            }

            case OP_OPERATE_DIV: {
                division();
                break;
            }

            case OP_OPERATE_EXP: {
                power();
                break;
            }

            case OP_STORE: {
                vm->globals[vm->B.as.i] = vm->A;
                break;
            }

            case OP_LOAD: {
                if (vm->A.as.i < 0 || vm->A.as.i >= GLOBALS_MAX) {
                    raise("Global slot out of range", vm->ip, 0);
                    callAllErr();
                    freeConstPool(vmStd.consts, vmStd.constCount);
                    return 1;
                }
                vm->R = vm->globals[vm->A.as.i];
                break;
            }

            case OP_CONST_LOAD: {
                uint16_t constIndex = read16();

                if (constIndex >= (uint16_t)vm->constCount) {
                    raise("Const load slot out of range", vm->ip, 0);
                    callAllErr();
                    freeConstPool(vmStd.consts, vmStd.constCount);
                    return 1;
                }

                vm->R = vm->consts[constIndex];
                break;
            }
                
            case OP_CALL_NATIVE: {
                NativeCommand nc = readByte();
                advanceByte();
                switch (nc) {
                    case NAT_DUMP:
                        dumpState(op);
                        break;
                    default:
                        raise("Unkown Native Command", vm->ip,0);
                        callAllErr();
                        break;
                }
                break;
            }

            case OP_JUMP: {
                vm->ip = read32();
                break;
            }

            case OP_CALL: {
                checkCallStack();
                vm->callStack[vm->callAmt++] = (Call){.rtnAdr = vm->ip+4};
                vm->ip = read32();
                break;
            }

            case OP_RETURN: {
                vm->ip = vm->callStack[--vm->callAmt].rtnAdr;
                break;
            }

            case OP_FINISH: {
                freeConstPool(vmStd.consts, vmStd.constCount);
                return 0;
            }

            default: {
                logRuntime("Unknown opcode encountered");
                printf("Unknown opcode: 0x%02X at %d\n", op, vm->ip - 1);
                freeConstPool(vmStd.consts, vmStd.constCount);
                exit(1);
            }
        }
    }

    logRuntime("VM exited unexpectedly");
    raise("Exited Early", vm->ip, 0);
    freeConstPool(vmStd.consts, vmStd.constCount);
    callAllErr();
    return 1;
}
