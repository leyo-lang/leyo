#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR_ARR,
    TYPE_STRING_ARR,
    TYPE_INT_ARR,
    TYPE_FLOAT_ARR,
} ValueType;

typedef union {
    char chr;
    char *str;
    int num;
    float flt;
} Value;

typedef struct {
    ValueType type;
    Value value;
    char *name;
} Variable;

typedef struct {
    int amount;
    Variable *value;
} VariableList;

typedef struct {
    uint8_t *data;
    int dataCount;
    int pos;
    int nibblePos;
    int stmt;
} VM;

VM vm;
VM *v;

VariableList variables = {0};
VariableList *vars;

static uint8_t currentByte() {
    if (v->pos >= v->dataCount) {
        logRuntime("currentByte() hit EOF");
        return 0;
    }

    return v->data[v->pos];
}

static uint8_t previousByte() {
    if (v->pos - 1 >= v->dataCount) {
        logRuntime("previousByte() invalid access");
        return 0;
    }

    return v->data[v->pos - 1];
}

static uint8_t peekByte() {
    if (v->pos + 1 >= v->dataCount) {
        logRuntime("peekByte() hit EOF");
        return 0;
    }

    return v->data[v->pos + 1];
}

static void advanceByte() {
    if (!(v->pos >= v->dataCount)) {
        v->pos++;

        char buf[128];
        sprintf(buf, "Advanced VM byte cursor -> %d", v->pos);
        logRuntime(buf);
    } else {
        logRuntime("advanceByte() prevented overflow");
    }
}

static uint8_t highNibble(uint8_t byte) {
    return (byte >> 4) & 0xF;
}

static uint8_t lowNibble(uint8_t byte) {
    return byte & 0xF;
}

static void expectCurrent(uint8_t type, char *errorStr) {
    char buf[128];

    sprintf(buf,
        "expectCurrent() expecting 0x%02X got 0x%02X",
        type,
        currentByte());

    logRuntime(buf);

    if (type != currentByte()) {
        logRuntime("Expect failed (current mismatch)");
        raise(errorStr, v->stmt, 0);
        return;
    }

    advanceByte();
}

static void expect(uint8_t type, char *errorStr) {
    char buf[128];

    sprintf(buf,
        "expect() expecting 0x%02X got 0x%02X",
        type,
        peekByte());

    logRuntime(buf);

    if (type != peekByte()) {
        logRuntime("Expect failed (peek mismatch)");
        raise(errorStr, v->stmt, 0);
        return;
    }

    advanceByte();
}

static void expectAndPass(uint8_t type, char *errorStr) {
    char buf[128];

    sprintf(buf,
        "expectAndPass() expecting 0x%02X got 0x%02X",
        type,
        peekByte());

    logRuntime(buf);

    if (type != peekByte()) {
        logRuntime("ExpectAndPass failed");
        raise(errorStr, v->stmt, 0);
        return;
    }

    advanceByte();
    advanceByte();
}

void evaluateExpr(Variable *var, char *expr) {
    switch (var->type) {

        case TYPE_INT:
            var->value.num = atoi(expr);
            break;

        case TYPE_FLOAT:
            var->value.flt = atof(expr);
            break;

        case TYPE_STRING:
            var->value.str = strdup(expr);
            break;

        case TYPE_CHAR:
            var->value.chr = expr[0];
            break;

        default:
            raise("Unsupported expression type", 0,0);
            callAllErr();
    }
}

static char* readExpr() {
    uint8_t len = currentByte();

    char buf[128];
    sprintf(buf, "readExpr() length = %d", len);
    logRuntime(buf);

    advanceByte();

    char *str = malloc(len + 1);

    if (!str) {
        logRuntime("Malloc fail mid readExpr()");
        raise("Malloc Error", v->stmt, 0);
        callAllErr();
        return NULL;
    }

    for (uint8_t i = 0; i < len; i++) {
        str[i] = (char)currentByte();

        sprintf(buf,
            "readExpr() char[%d] = '%c' (0x%02X)",
            i,
            str[i],
            currentByte());

        logRuntime(buf);

        advanceByte();
    }

    str[len] = '\0';

    sprintf(buf, "readExpr() result = \"%s\"", str);
    logRuntime(buf);

    return str;
}

void runVarDecl() {
    logRuntime("runVarDecl() entered");

    Variable var = {0};
    ValueType type;

    uint8_t opcode = currentByte();

    char buf[128];

    sprintf(buf,
        "runVarDecl() opcode = 0x%02X",
        opcode);

    logRuntime(buf);

    switch (lowNibble(opcode)) {
        case 0x1:
            type = TYPE_INT;
            logRuntime("Variable type -> INT");
            break;

        case 0x2:
            type = TYPE_INT_ARR;
            logRuntime("Variable type -> INT ARRAY");
            break;

        case 0x3:
            type = TYPE_FLOAT;
            logRuntime("Variable type -> FLOAT");
            break;

        case 0x4:
            type = TYPE_FLOAT_ARR;
            logRuntime("Variable type -> FLOAT ARRAY");
            break;

        case 0x5:
            type = TYPE_CHAR;
            logRuntime("Variable type -> CHAR");
            break;

        case 0x6:
            type = TYPE_CHAR_ARR;
            logRuntime("Variable type -> CHAR ARRAY");
            break;

        case 0x7:
            type = TYPE_STRING;
            logRuntime("Variable type -> STRING");
            break;

        case 0x8:
            type = TYPE_STRING_ARR;
            logRuntime("Variable type -> STRING ARRAY");
            break;

        default:
            logRuntime("Bad bytecode opcode in runVarDecl()");
            raise("Bad bytecode", v->stmt, 0);
            callAllErr();
            return;
    }

    var.type = type;

    logRuntime("Checking identifier delimiter");
    expect(BC_IDENT_DELIM, "No identifier after var decl");
    advanceByte();

    logRuntime("Reading variable identifier");
    var.name = readExpr();

    expectCurrent(BC_EXPR_DELIM, "Missing expr delimiter");
    logRuntime("Evaluating expr");
    char *expr = readExpr();

    evaluateExpr(&var, expr);

    sprintf(buf, "Variable declared -> %s", var.name);
    logRuntime(buf);

    logRuntime("runVarDecl() finished");
}

int runByteCode(ByteCodeResult bcr) {
    vars = &variables;

    vm.data = bcr.data;
    vm.dataCount = bcr.length;
    vm.pos = 0;
    vm.nibblePos = 0;
    vm.stmt = 0;

    v = &vm;

    logRuntime("========== VM START ==========");
    
    char buf[128];

    sprintf(buf, "Bytecode length = %d", vm.dataCount);
    logRuntime(buf);

    sprintf(buf, "Initial byte = 0x%02X", currentByte());
    logRuntime(buf);

    if (currentByte() != BC_START_END) {
        logRuntime("Missing bytecode start marker");
        raise("Missing 0xFF start marker", 0, 0);
        callAllErr();
        return -1;
    }

    logRuntime("Found BC_START_END");
    advanceByte();

    while (v->pos < v->dataCount) {

        uint8_t byte = currentByte();

        sprintf(buf,
            "VM LOOP | pos=%d stmt=%d byte=0x%02X",
            v->pos,
            v->stmt,
            byte);

        logRuntime(buf);

        if (byte == BC_START_END) {
            logRuntime("Reached BC_START_END");
            break;
        }

        if (highNibble(byte) == 1) {
            logRuntime("Dispatching runVarDecl()");
            runVarDecl();
        } else {
            logRuntime("Unknown opcode encountered");
            raise("Unknown opcode", v->stmt, 0);
            callAllErr();
            return -1;
        }

        if (currentByte() == BC_START_END) {
            logRuntime("Program ended after instruction");
            break;
        }

        logRuntime("Expecting BC_END_OF_LINE");
        expectCurrent(BC_END_OF_LINE, "No EndOfLine");

        v->stmt++;

        sprintf(buf,
            "Statement complete -> %d",
            v->stmt);

        logRuntime(buf);
    }

    logRuntime("========== VM END ==========");

    return 0;
}