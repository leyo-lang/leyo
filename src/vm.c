#include "../include/errors.h"
#include "../include/codes.h"
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
// #define GLOBALS_MAX 65535

// extern struct Value;

typedef struct {
    uint32_t rtnAdr;
    //uint32_t stackPos;
} Call;

typedef struct {
    uint8_t *code;
    uint32_t ip;

    char filename[512];

    Value *stack;
    uint32_t sp;
    uint32_t stackCap;

    Value *globals;
    uint64_t globalAmount; // how many globals are used
    
    Value *consts;
    int constCount;

    Call *callStack;
    int callAmt;
    int callCap;
} VM;

VM vmStd = {0};
VM *vm;

static void checkCallStack(void) {
    if (vm->callAmt >= vm->callCap-1) {
        vm->callCap *= 2;
        vm->callStack = realloc(
            vm->callStack,
            vm->callCap * sizeof(Call)
        );
    }
}

static inline void push(Value v) {
    checkCallStack();
    vm->stack[vm->sp++] = v;
}

static inline Value pop(void) {
    if (vm->sp <= 0) {lraise(WF_VM, ERR_VM_UNDERFLOW, vm->ip,0, vm->filename);};
    return vm->stack[--vm->sp];
}

static inline Value peek(void) {
    if (vm->sp <= 0) {lraise(WF_VM, ERR_VM_UNDERFLOW, vm->ip,0, vm->filename);};
    return vm->stack[vm->sp - 1];
}

static inline Value prev(void) {
    if (vm->sp <= 1) {lraise(WF_VM, ERR_VM_UNDERFLOW, vm->ip,0, vm->filename);};
    return vm->stack[vm->sp - 2];
}

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

static void printValue(Value v) {
    switch (v.flag) {
        case VAL_INT:
            printf("%d", v.as.i);
            break;

        case VAL_FLOAT:
            printf("%g", v.as.f);
            break;

        case VAL_CHAR:
            printf("%c", v.as.c);
            break;

        case VAL_STR:
            printf("%s", v.as.s);
            break;

        default:
            break;
    }
}

static void dumpState(uint8_t op) {
    char buf[512];
    char rBuf[64];
    if (vm->sp <= 0) {
        snprintf(buf, sizeof(buf),
            "OP=0x%02X | stackPointer=%d stackCap=%d stackTop=None | ip=%d | callAmt=%d",
            op,
            vm->sp,
            vm->stackCap,
            vm->ip,
            vm->callAmt
        );
    } else {
        snprintf(buf, sizeof(buf),
            "OP=0x%02X | stackPointer=%d stackCap=%d stackTop=%s | ip=%d | callAmt=%d",
            op,
            vm->sp,
            vm->stackCap,
            valueToString(peek(), rBuf, sizeof(rBuf)),
            vm->ip,
            vm->callAmt
        );
    }

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
    Value rhs = pop();
    Value lhs = pop();

    switch (lhs.flag) {
        case VAL_CHAR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_STR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_INT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.i = lhs.as.i + rhs.as.i,
                        .flag = VAL_INT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.i + rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        case VAL_FLOAT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.f = lhs.as.f + rhs.as.i,
                        .flag = VAL_FLOAT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.f + rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        default:
            return;
    }
}

static void subtraction(void) {
    Value rhs = pop();
    Value lhs = pop();

    switch (lhs.flag) {
        case VAL_CHAR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_STR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_INT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.i = lhs.as.i - rhs.as.i,
                        .flag = VAL_INT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.i - rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        case VAL_FLOAT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.f = lhs.as.f - rhs.as.i,
                        .flag = VAL_FLOAT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.f - rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        default:
            return;
    }
}

static void multiplication(void) {
    Value rhs = pop();
    Value lhs = pop();

    switch (lhs.flag) {
        case VAL_CHAR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_STR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_INT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.i = lhs.as.i * rhs.as.i,
                        .flag = VAL_INT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.i * rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        case VAL_FLOAT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.f = lhs.as.f * rhs.as.i,
                        .flag = VAL_FLOAT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.f * rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        default:
            return;
    }
}

static void division(void) {
    Value rhs = pop();
    Value lhs = pop();

    switch (lhs.flag) {
        case VAL_CHAR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_STR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_INT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.i = lhs.as.i / rhs.as.i,
                        .flag = VAL_INT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.i / rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        case VAL_FLOAT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.f = lhs.as.f / rhs.as.i,
                        .flag = VAL_FLOAT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = lhs.as.f / rhs.as.f,
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        default:
            return;
    }
}

static void power(void) {
    Value rhs = pop();
    Value lhs = pop();

    switch (lhs.flag) {
        case VAL_CHAR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_STR:
            lraise(WF_VM, ERR_VM_TEXT_CALC_UNSUPPORTED, vm->ip,0, vm->filename);
            return;

        case VAL_INT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.i = pow(lhs.as.i, rhs.as.i),
                        .flag = VAL_INT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = pow(lhs.as.i, rhs.as.f),
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        case VAL_FLOAT:
            switch (rhs.flag) {
                case VAL_INT:
                    push((Value){
                        .as.f = pow(lhs.as.f, rhs.as.i),
                        .flag = VAL_FLOAT
                    });
                    return;

                case VAL_FLOAT:
                    push((Value){
                        .as.f = pow(lhs.as.f, rhs.as.f),
                        .flag = VAL_FLOAT
                    });
                    return;

                default:
                    lraise(WF_VM, ERR_VM_TEXT_NUM_TYPE_CALC, vm->ip,0, vm->filename);
                    return;
            }

        default:
            return;
    }
}

static void freeAll(void) {
    freeConstPool(vmStd.consts, vmStd.constCount);
    free(vmStd.stack);
    free(vmStd.globals);
    return;
}

int runVM(ByteCodeResult bc, bool verbose, const char filename[512]) {
    logRuntime("Starting VM execution");
    vmStd.code = bc.data;
    vmStd.ip = 0;
    if (filename) {
        snprintf(vmStd.filename, 511, "%s", filename);
    }
    vmStd.consts = NULL;
    vmStd.constCount = 0;
    vmStd.callAmt = 0;
    vmStd.callCap = 64;
    vmStd.callStack = malloc(vmStd.callCap * sizeof(Call));
    vmStd.sp = 0;
    vmStd.stackCap = 64;
    vmStd.stack = malloc(vmStd.stackCap * sizeof(Value));
    vmStd.globalAmount = bc.globalAmount;
    vmStd.globals = malloc(vmStd.globalAmount * sizeof(Value));
    vm = &vmStd;

    if (bc.cb.length > 0 && bc.cb.data != NULL) {
        vmStd.consts = decodeConstPool(bc.cb.data, bc.cb.length, &vmStd.constCount);

        if (!vmStd.consts) {
            logRuntime("Failed to decode constant pool");
            freeAll();
            lraise(WF_VM, ERR_VM_CANNOT_DECODE_CONST_POOL, vm->ip,0, vm->filename);
            return 1;
        }

        logRuntime("Constant pool loaded into VM");
    }

    //void fcp() {freeConstPool()};
    //atexit(freeConstPool);

    while (1) {
        uint8_t op = readByte();

        if (verbose) {
            dumpState(op);
        }

        char buf[64];
        snprintf(buf, 64, "%s", opcode_name(op));
        logRuntime(buf);

        advanceByte();

        switch (op) {
            case OP_PUSH: {
                push((Value){.as.i = read16(), .flag = VAL_INT});
                break;
            }

            case OP_POP: {
                pop();
                break;
            }

            case OP_SWAP: {
                if (vm->sp < 2) {
                    lraise(WF_VM, ERR_VM_UNDERFLOW, vm->ip,0, vm->filename);
                }

                Value b = pop();
                Value a = pop();

                push(b);
                push(a);
                break;
            }

            case OP_DUP: {
                push(peek());
                break;
            }

            case OP_OVER: {
                push(prev());
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
                uint16_t slot = read16();
                if (slot >= vm->globalAmount) {
                    freeAll();
                    lraise(WF_VM, ERR_VM_GLOBAL_SLOT_OUT_OF_RANGE, vm->ip,0, vm->filename);
                    return 1;
                }
                vm->globals[slot] = pop();
                break;
            }

            case OP_LOAD: {
                uint16_t slot = read16();
                if (slot >= vm->globalAmount) {
                    freeAll();
                    lraise(WF_VM, ERR_VM_GLOBAL_SLOT_OUT_OF_RANGE, vm->ip,0, vm->filename);
                    return 1;
                }
                push(vm->globals[slot]);
                break;
            }

            case OP_CONST_LOAD: {
                uint16_t constIndex = read16();

                if (constIndex >= (uint16_t)vm->constCount) {
                    freeAll();
                    lraise(WF_VM, ERR_VM_CONST_OUT_OF_RANGE, vm->ip,0, vm->filename);
                    return 1;
                }

                push(vm->consts[constIndex]);
                break;
            }
                
            case OP_CALL_NATIVE: {
                NativeCommand nc = readByte();
                advanceByte();
                switch (nc) {
                    case NAT_DUMP:
                        dumpState(op);
                        break;
                    case NAT_PRINT:
                        printValue(pop());  
                        break;
                    case NAT_EXIT:
                        return pop().as.i;
                        break;
                    default:
                        lraise(WF_VM, ERR_VM_UNKOWN_NATIVE_COMMAND, vm->ip,0, vm->filename);
                        break;
                }
                break;
            }

            case OP_JUMP: {
                int32_t offset = (int32_t)read32();
                if (offset < 0 && vm->ip < (uint32_t)(-offset)) {
                    lraise(WF_VM, ERR_VM_INVALID_JUMP, vm->ip,0, vm->filename);
                }
                if (verbose) {
                    char buf[64];
                    snprintf(buf, 64, "%d", offset);
                    logRuntime(buf);
                }
                vm->ip += offset;
                break;
            }

            case OP_CALL: {
                checkCallStack();
                vm->callStack[vm->callAmt++] = (Call){.rtnAdr = vm->ip+4};
                int32_t offset = (int32_t)read32();
                if (offset < 0 && vm->ip < (uint32_t)(-offset)) {
                    lraise(WF_VM, ERR_VM_INVALID_JUMP, vm->ip,0, vm->filename);
                }
                if (verbose) {
                    char buf[64];
                    snprintf(buf, 64, "%d", offset);
                    logRuntime(buf);
                }
                vm->ip += offset;
                break;
            }

            case OP_RETURN: {
                --vm->callAmt;
                //vm->sp = vm->callStack[vm->callAmt].stackPos;
                vm->ip = vm->callStack[vm->callAmt].rtnAdr;
                break;
            }

            case OP_FINISH: {
                freeAll();
                return pop().as.i;
            }

            default: {
                logRuntime("Unknown opcode encountered");
                printf("Unknown opcode: 0x%02X at %d\n", op, vm->ip - 1);
                freeAll();
                exit(1);
            }
        }
    }

    logRuntime("VM exited unexpectedly");
    freeAll();
    lraise(WF_VM, ERR_VM_UNEXPECTED_EXIT, vm->ip,0, vm->filename);
    return 1;
}
