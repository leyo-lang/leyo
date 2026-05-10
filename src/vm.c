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
    int nibblePos; // 0 = high nibble, 1 = low nibble
    int stmt;
} VM;

VM vm;
VM *v;

VariableList variables = {0};
VariableList *vars;

static uint8_t currentByte() {
    if (v->pos >= v->dataCount) return 0;
    return v->data[v->pos];
}

static uint8_t previousByte() {
    if (v->pos-1 >= v->dataCount) return 0;
    return v->data[v->pos-1];
}

static uint8_t peekByte() {
    if (v->pos + 1 >= v->dataCount) return 0;
    return v->data[v->pos + 1];
}

static void advanceByte() {
    if (!(v->pos >= v->dataCount)) {
        v->pos++;
    }
}

static uint8_t highNibble(uint8_t byte) {
    return (byte >> 4) & 0xF;
}

static uint8_t lowNibble(uint8_t byte) {
    return byte & 0xF;
}

static void expectCurrent(uint8_t type, char *errorStr) {
    if (type != currentByte()) {
        logRuntime("Expect failed (current mismatch)");
        raise(errorStr, v->stmt,0);
    }
    advanceByte();
}

static void expect(uint8_t type, char *errorStr) {
    if (type != peekByte()) {
        logRuntime("Expect failed (peek mismatch)");
        raise(errorStr, v->stmt,0);
    }
    advanceByte();
}

static void expectAndPass(uint8_t type, char *errorStr) {
    if (type != peekByte()) {
        logRuntime("ExpectAndPass failed");
        raise(errorStr, v->stmt,0);
    }
    advanceByte();
    advanceByte();
}

static char* readExpr() {
    uint8_t len = currentByte();

    char *str = malloc(len + 1); // +1 for null terminator
    if (!str) {
        logRuntime("Malloc fail mid read-expr");
        raise("Malloc Error", v->stmt,0);
        callAllErr();
    }

    for (uint8_t i=0; i<len; i++) {
        str[i] = (char)currentByte();
    }

    str[len] = '\0';
    return str;
}

void runVarDecl() {
    Variable var = {0};
    ValueType type;
    switch (lowNibble(currentByte())) {
        case 0x1: type = TYPE_INT; break;
        case 0x2: type = TYPE_INT_ARR; break;
        case 0x3: type = TYPE_FLOAT; break;
        case 0x4: type = TYPE_FLOAT_ARR; break;
        case 0x5: type = TYPE_CHAR; break;
        case 0x6: type = TYPE_CHAR_ARR; break;
        case 0x7: type = TYPE_STRING; break;
        case 0x8: type = TYPE_STRING_ARR; break;
        default: logRuntime("Bad bytecode"); raise("Bad bytecode", v->stmt,0); callAllErr(); return;
    };
    var.type = type;
    expect(BC_IDENT_DELIM, "No identifier after var decl");
    var.name = readExpr();
    
}

int runByteCode(ByteCodeResult bcr) {
    vars = &variables;

    vm.data = bcr.data;
    vm.dataCount = bcr.length;
    vm.pos = 0;
    vm.nibblePos = 0;
    vm.stmt = 0;
    v = &vm;

    logRuntime("VM STARTED");

    // validate start marker ONCE
    if (currentByte() != 0xFF) {
        raise("Missing 0xFF start marker", 0, 0);
        callAllErr();
        return -1;
    }

    advanceByte(); // consume 0xFF
    logRuntime("Found BC start");

    while (vm.pos < vm.dataCount) {
        uint8_t byte = currentByte();

        if (byte == 0xF0) {
            advanceByte(); // consume EOL
            continue;
        }

        if (highNibble(byte) == 1) {
            runVarDecl();
        } else {
            raise("Unknown opcode", vm.stmt, 0);
            callAllErr();
            return -1;
        }

        vm.stmt++;
    }

    return 0;
}