#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static char *charToStr(char c) {
    static char tmp[2];
    tmp[0] = c;
    tmp[1] = '\0';
    return tmp;
}

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
    char *char_arr;
    char **string_arr;
    int *int_arr;
    float *float_arr;
} Value;

typedef struct {
    ValueType type;
    Value value;
    int length;
    char *name;
} Variable;

typedef struct {
    int amount;
    Variable value[2048];
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

void printVar(Variable var) {
    switch (var.type) {

        case TYPE_CHAR:
            fprintf(stderr, "%c", var.value.chr);
            break;

        case TYPE_STRING:
            fprintf(stderr, "%s", var.value.str);
            break;

        case TYPE_INT:
            fprintf(stderr, "%d", var.value.num);
            break;

        case TYPE_FLOAT:
            fprintf(stderr, "%f", var.value.flt);
            break;

        case TYPE_CHAR_ARR:
            if (var.value.char_arr != NULL) {
                for (int i = 0; i < var.length; i++) {
                    fprintf(stderr, "%c", var.value.char_arr[i]);
                }
            }
            break;

        case TYPE_STRING_ARR:
            if (var.value.string_arr != NULL) {
                fprintf(stderr, "[");

                for (int i = 0; i < var.length; i++) {
                    fprintf(stderr, "\"%s\"", var.value.string_arr[i]);

                    if (i < var.length - 1) {
                        fprintf(stderr, ", ");
                    }
                }

                fprintf(stderr, "]");
            }
            break;

        case TYPE_INT_ARR:
            if (var.value.int_arr != NULL) {
                fprintf(stderr, "[");

                for (int i = 0; i < var.length; i++) {
                    fprintf(stderr, "%d", var.value.int_arr[i]);

                    if (i < var.length - 1) {
                        fprintf(stderr, ", ");
                    }
                }

                fprintf(stderr, "]");
            }
            break;

        case TYPE_FLOAT_ARR:
            if (var.value.float_arr != NULL) {
                fprintf(stderr, "[");

                for (int i = 0; i < var.length; i++) {
                    fprintf(stderr, "%f", var.value.float_arr[i]);

                    if (i < var.length - 1) {
                        fprintf(stderr, ", ");
                    }
                }

                fprintf(stderr, "]");
            }
            break;

        default:
            fprintf(stderr, "<unknown>");
            break;
    }
}

static Variable getVar(char *name, int *pos) {
    logRuntime("Getting Var: ");
    logRuntime(name);
    for (int i = 0; i < vars->amount; i++) {
        if (strcmp(name, vars->value[i].name) == 0) {
            logRuntime("Found var");
            *pos = i;
            return vars->value[i];
        }
    }

    raise("No var under name", v->stmt,0);
    callAllErr();
}

ValueType detectType(char *expr) {

    int len = strlen(expr);

    // arrays
    if (expr[0] == '[' && expr[len - 1] == ']') {

        // string array
        if (strstr(expr, "\"") != NULL) {
            return TYPE_STRING_ARR;
        }

        // char array
        if (strstr(expr, "'") != NULL) {
            return TYPE_CHAR_ARR;
        }

        // float array
        if (strchr(expr, '.') != NULL) {
            return TYPE_FLOAT_ARR;
        }

        // int array
        return TYPE_INT_ARR;
    }

    // string
    if (expr[0] == '"' && expr[len - 1] == '"') {
        return TYPE_STRING;
    }

    // char
    if (expr[0] == '\'' && expr[2] == '\'') {
        return TYPE_CHAR;
    }

    // float
    if (strchr(expr, '.') != NULL) {
        return TYPE_FLOAT;
    }

    // int
    return TYPE_INT;
}

Value parseValue(char *expr, ValueType type) {

    Value v;

    switch (type) {

        case TYPE_INT:
            v.num = atoi(expr);
            break;

        case TYPE_FLOAT:
            v.flt = atof(expr);
            break;

        case TYPE_STRING:
            expr[strlen(expr) - 1] = '\0';
            v.str = strdup(expr + 1);
            break;

        case TYPE_CHAR:
            v.chr = expr[0];
            break;
    }

    return v;
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

void runAssignment() {
    logRuntime("runAssignment() entered");
    advanceByte();
    int pos;
    advanceByte();
    Variable var = getVar(readExpr(), &pos);
    advanceByte();
    char *value = readExpr();
    if (detectType(value) != var.type) {
        raise("Type Error: Type Mismatch", v->stmt, 0);
        callAllErr();
    }
    var.value = parseValue(value, detectType(value));
    vars->value[pos] = var;
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

    vars->value[vars->amount++] = var;

    logRuntime("runVarDecl() finished");
}

void printVars() {
    for (int i = 0; i < vars->amount; i++) {
        printVar(vars->value[i]);
    }
}

void dumpVM() {
    fprintf(stderr, "VM TRACE CALL\npos->%d\nstmt->%d\nnibble->", v->pos, v->stmt, v->nibblePos);
    printVars();
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
        } else if (currentByte() == BC_ASSIGN) {
            logRuntime("Dispatching runAssignment()");
            runAssignment();
        } else if (currentByte() == BC_TRACE) {
            logRuntime("Dumping VM data");
            fprintf(stderr, "Pos: %d\n", v->pos);
            for (int i=0; i < vars->amount; i++) {
                fprintf(stderr, "Variable %s: %c\n", vars->value[i].name, vars->value[i].value.chr);
            }
            advanceByte();
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

    for (int i=0; i < vars->amount; i++) {
        logRuntime("Variable:");
        logRuntime(vars->value[i].name);
        logRuntime(charToStr(vars->value[i].value.chr));
    }

    logRuntime("========== VM END ==========");

    return 0;
}