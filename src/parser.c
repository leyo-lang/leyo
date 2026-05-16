#include "../include/type.h"
#include "../include/errors.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    int length;
} ByteCodeResult;

typedef struct {
    char *name;
    uint16_t slot;
} Symbol;

typedef struct {
    Token *tokens;
    int pos;
    int count;

    uint8_t bytebuff[2048];
    int byteIndex;

    int hasHalfByte;
    uint8_t halfByte;

    // NEW: variable system
    Symbol globals[256];
    int globalCount;
} ByteCoder;

ByteCoder bytecoder = {0};
ByteCoder *b;

static Token current() {
    return b->tokens[b->pos];
}

static Token previous() {
    if (b->count-1==b->pos) {
        logBuildParser("Too far - previoused into eos");
        raise("Internal Parser Error: Too far - previoused into eos", current().line, current().collumn);
        callAllErr();
    }
    return b->tokens[b->pos-1];
}

static Token peek() {
    if (b->count-1==b->pos) {
        logBuildParser("Too far - peeked into eos");
        raise("Internal Parser Error: Too far - peeked into eos", current().line, current().collumn);
        callAllErr();
    }
    return b->tokens[b->pos+1];
}

static void advance() {
    b->pos++;
}

static void expectCurrent(TokenType type, char *errorStr) {
    if (type != current().type) {
        logBuildParser("Expect failed (current mismatch)");
        raise(errorStr, current().line, current().collumn);
    }
    advance();
}

static void expect(TokenType type, char *errorStr) {
    if (type != peek().type) {
        logBuildParser("Expect failed (peek mismatch)");
        raise(errorStr, peek().line, peek().collumn);
    }
    advance();
}

static void expectAndPass(TokenType type, char *errorStr) {
    advance();
    if (type != current().type) {
        logBuildParser("ExpectAndPass failed");
        raise(errorStr, current().line, current().collumn);
    }
    advance();
}

static void writeByte(uint8_t value) {
    if ((size_t)b->byteIndex >= sizeof(b->bytebuff)) {
        logBuildParser("Byte buffer overflow detected");
        raise("Byte buffer overflow", current().line, current().collumn);
    }

    b->bytebuff[b->byteIndex++] = value;
}


static int defineGlobal(char *name) {
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            return b->globals[i].slot;
        }
    }

    int slot = b->globalCount;

    b->globals[b->globalCount].name = strdup(name);
    b->globals[b->globalCount].slot = slot;
    b->globalCount++;

    return slot;
}

static int resolveGlobal(char *name) {
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            return b->globals[i].slot;
        }
    }

    raise("Undefined variable", current().line, current().collumn);
    callAllErr();
    return -1;
}

static void parsePrimary() {

    // number literal
    if (current().type == NUMBER) {

        int value = atoi(current().value);

        writeByte(OP_PUSH_CONST);
        writeByte((uint8_t)value);

        advance();
        return;
    }

    // variable
    if (current().type == IDENTIFIER) {

        uint16_t slot = resolveGlobal(current().value);

        writeByte(OP_LOAD_GLOBAL);
        writeByte((uint8_t)slot);

        advance();
        return;
    }

    raise("Invalid expression", current().line, current().collumn);
}

static void parseExpression() {

    parsePrimary();

    while (current().type == OPERATION) {

        char *op = current().value;
        advance();

        parsePrimary();

        if (strcmp(op, "+") == 0) {
            writeByte(OP_ADD);
        }
        else if (strcmp(op, "-") == 0) {
            writeByte(OP_SUB);
        }
        else if (strcmp(op, "*") == 0) {
            writeByte(OP_MUL);
        }
        else if (strcmp(op, "/") == 0) {
            writeByte(OP_DIV);
        }
        else {
            raise("Unknown operator", current().line, current().collumn);
        }
    }
}

static void parseAssign() {
    logBuildParser("Parsing assignment");

    char *name = current().value;
    uint16_t slot = resolveGlobal(name);

    expectAndPass(EQUALS, "Expected '='");

    parseExpression();

    writeByte(OP_STORE_GLOBAL);
    writeByte((uint8_t)slot);

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseVarDecl() {
    logBuildParser("Parsing variable declaration");

    if (current().type != IDENTIFIER) {
        raise("Expected type", current().line, current().collumn);
    };

    char *type = current().value;

    if (!(strcmp(type, "int") == 0 ||
          strcmp(type, "str") == 0 ||
          strcmp(type, "flt") == 0 ||
          strcmp(type, "chr") == 0)) {
        raise("Invalid type", current().line, current().collumn);
    }

    expect(IDENTIFIER, "Expected variable name");

    char *name = current().value;

    uint16_t slot = defineGlobal(name);

    expectAndPass(EQUALS, "Expected '='");

    // IMPORTANT: expression compiler must leave value on stack
    parseExpression();

    writeByte(OP_STORE_GLOBAL);
    writeByte((uint8_t)slot);

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseStatement() {
    logBuildParser("Parsing statement");

    switch (current().type) {

        case IDENTIFIER: {

            // type keywords OR variable name
            if (strcmp(current().value, "int") == 0 ||
                strcmp(current().value, "str") == 0 ||
                strcmp(current().value, "flt") == 0 ||
                strcmp(current().value, "chr") == 0) {

                parseVarDecl();
            }
            else if (peek().type == EQUALS) {
                parseAssign();
            }
            else {
                raise("Unknown identifier statement", current().line, current().collumn);
            }
            break;
        }

        default:
            raise("Unknown statement", current().line, current().collumn);
            break;
    }
}

ByteCodeResult parse(TokenStream ts) {
    logBuildParser("Parser started");

    b = &bytecoder;

    b->tokens = ts.stream;
    b->pos = 0;
    b->count = ts.count;

    b->byteIndex = 0;
    b->globalCount = 0;
    b->hasHalfByte = 0;

    while (current().type != ENDOFSTREAM) {
        parseStatement();
    }
    

    writeByte(OP_HALT); // end mark

    ByteCodeResult res = {0};
    res.length = b->byteIndex;
    res.data = malloc(res.length);
    memcpy(res.data, b->bytebuff, res.length);
    return res;
}