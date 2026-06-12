#include "../include/type.h"
#include "../include/parser.h"
#include "../include/errors.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

ConstBuffer constBuf = {0};

ByteCoder bytecoder = {0};
ByteCoder *b;



static Token current(void) {
    return b->tokens[b->pos];
}

/*
static Token previous(void) {
    if (b->count-1==b->pos) {
        logBuildParser("Too far - previoused into eos");
        raise("Internal Parser Error: Too far - previoused into eos", current().line, current().collumn);
        callAllErr();
    }
    return b->tokens[b->pos-1];
}
*/

static Token peek(void) {
    if (b->count-1==b->pos) {
        logBuildParser("Too far - peeked into eos");
        raise("Internal Parser Error: Too far - peeked into eos", current().line, current().collumn);
        callAllErr();
    }
    return b->tokens[b->pos+1];
}

static void advance(void) {
    if (b->count+1==b->pos) {
        logBuildParser("Too far - advanced into eos");
        raise("Internal Parser Error: Too far - advanced into eos", current().line, current().collumn);
        callAllErr();
    }
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

static void emit(uint8_t value) {
    if ((size_t)b->byteIndex >= sizeof(b->bytebuff)) {
        logBuildParser("Byte buffer overflow detected");
        raise("Byte buffer overflow", current().line, current().collumn);
    }

    b->bytebuff[b->byteIndex++] = value;
}

static void emit16(uint16_t value) {
    emit((uint8_t)(value & 0xFF));
    emit((uint8_t)((value >> 8) & 0xFF));
}

static void constEmit(uint8_t v) {
    constBuf.data[constBuf.length++] = v;
}

static void serializeValue(Value *v) {
    constEmit((uint8_t)v->flag);

    switch (v->flag) {
        case VAL_INT: {
            int x = v->as.i;
            memcpy(&constBuf.data[constBuf.length], &x, sizeof(x));
            constBuf.length += sizeof(x);
            break;
        }

        case VAL_FLOAT: {
            double x = v->as.f;
            memcpy(&constBuf.data[constBuf.length], &x, sizeof(x));
            constBuf.length += sizeof(x);
            break;
        }

        case VAL_CHAR:
            constEmit((uint8_t)v->as.c);
            break;

        case VAL_STR: {
            uint16_t len = strlen(v->as.s);

            memcpy(&constBuf.data[constBuf.length], &len, sizeof(len));
            constBuf.length += sizeof(len);

            memcpy(&constBuf.data[constBuf.length], v->as.s, len);
            constBuf.length += len;
            break;
        }
    }
}

static uint16_t emitConst() {
    Value v = {0};
    switch (current().type) {
        case CHR:
            v.as.c = current().value[0];
            v.flag = VAL_CHAR;
            break;

        case NUMBER:
            v.as.i = atoi(current().value);
            v.flag = VAL_INT;
            break;

        case FLT:
            v.as.f = atof(current().value);
            v.flag = VAL_FLOAT;
            break;

        case STRING:
            v.as.s = current().value;
            v.flag = VAL_STR;
            break;

        default:
            break;
    }
    serializeValue(&v);
    b->consts[b->constAmt++] = v;
    return b->constAmt-1;
}

static int define(char *name) {
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

static int resolve(char *name) {
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            return b->globals[i].slot;
        }
    }

    raise("Undefined variable", current().line, current().collumn);
    callAllErr();
    return -1;
}

static void parseAtom(void) { // small singular unit of expression (number, identifier, string, etc)
    logBuildParser("Parsing atom");

    switch (current().type) {
        case NUMBER: {
            logBuildParser("atom is number");
            emit(OP_CONST_LOAD);
            emit16((uint16_t)emitConst());
            emit(OP_PUT_A_R);
    
            advance();
            break;
        }

        case IDENTIFIER: {
            logBuildParser("atom is ident");
            uint16_t slot = resolve(current().value);
            emit(OP_PUT_A);
            emit16((uint16_t)slot); // put slot into A
            emit(OP_LOAD); // takes from slot def in A and return into r
            emit(OP_PUT_A_R);
            //emit(OP_SS_PUSH_A);

            advance();
            break;
        }

        default: {
            raise("Unknown atom", current().line, current().collumn);
            callAllErr();
            break;
        }
    }
}
static void parseExpression(void) {
    parseAtom(); // result -> A

    while (current().type == OPERATION) {
        char op = current().value[0];

        emit(OP_PUT_S); // save left operand into B

        advance();

        parseAtom(); // right operand -> A

        emit(OP_PUT_S); // swap:
                         // B = right
                         // A = left

        switch (op)
        {
            case '+': emit(OP_OPERATE_ADD); break;
            case '-': emit(OP_OPERATE_SUB); break;
            case '*': emit(OP_OPERATE_MUL); break;
            case '/': emit(OP_OPERATE_DIV); break;
            case '^': emit(OP_OPERATE_EXP); break;

            default:
                raise("Unknown operator",
                      current().line,
                      current().collumn);
                return;
        }

        emit(OP_PUT_A_R); // result becomes new left side
    }
}

static void parseAssign(void) {
    logBuildParser("Parsing assignment");

    char *name = current().value;
    uint16_t slot = resolve(name);

    expectAndPass(EQUALS, "Expected '='");

    parseExpression();

    emit(OP_PUT_B);
    emit16(slot);
    emit(OP_STORE);

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseVarDecl(void) {
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

    uint16_t slot = define(name);

    expectAndPass(EQUALS, "Expected '='");

    parseExpression();

    emit(OP_PUT_B);
    emit16(slot);
    emit(OP_STORE);
    

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseStatement(void) {
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

ByteCodeResult parse(TokenStream *ts) {
    logBuildParser("Parser started");

    logBuildParser("Assigning parser state");

    b = &bytecoder;

    b->tokens = ts->stream;
    b->count = ts->count;
    b->pos = 0;
    b->byteIndex = 0;
    b->globalCount = 0;
    b->constAmt = 0;
    b->consts = malloc(sizeof(Value) * 1024);

    if (!b->consts) {
        raise("Failed to allocate const table", 0, 0);
        callAllErr();
    }

    constBuf.length = 0;
    constBuf.data = malloc(65535);

    if (!constBuf.data) {
        raise("Failed to allocate const buffer", 0, 0);
        callAllErr();
    }

    logBuildParser("State assigned");

    char buf[256];

    sprintf(buf, "tokens=%p", (void*)b->tokens);
    logBuildParser(buf);

    sprintf(buf, "count=%d", b->count);
    logBuildParser(buf);

    sprintf(buf, "pos=%d", b->pos);
    logBuildParser(buf);

    sprintf(buf, "count=%d", b->count);
    logBuildParser(buf);

    if (b->count > 0) {
        sprintf(buf, "token0.type=%d", b->tokens[0].type);
        logBuildParser(buf);
        sprintf(buf, "token0.value=%s", b->tokens[0].value);
        logBuildParser(buf);
    }

    logBuildParser("About to enter while loop");

    while (current().type != ENDOFSTREAM) {
        logBuildParser("Entering parseStatement()");
        parseStatement();
        logBuildParser("Returned from parseStatement()");
    }

    logBuildParser("Reached EOF");

    emit(OP_FINISH); // end mark

    ByteCodeResult res = {0};
    res.length = b->byteIndex;
    res.data = malloc(res.length);
    memcpy(res.data, b->bytebuff, res.length);

    size_t cap = 65535;
    uint8_t *consts = malloc(cap);
    
    size_t amount = 0;
    Value c;
    int i = 0;
    while (i < b->constAmt) {
        c = b->consts[i++];
        switch (c.flag) {
            case VAL_INT:
                consts[amount++] = 0x01;
                int64_t val_i = c.as.i;
                memcpy(&consts[amount], &val_i, sizeof(val_i));
                amount += sizeof(val_i);
                break;

            case VAL_FLOAT:
                consts[amount++] = 0x02;

                double val_f = c.as.f;
                memcpy(&consts[amount], &val_f, sizeof(val_f));
                amount += sizeof(val_f);
                break;

            case VAL_CHAR:
                consts[amount++] = 0x03;
                consts[amount++] = (uint8_t)c.as.c;
                break;

            case VAL_STR: {
                const char *s = c.as.s;
                uint32_t len = strlen(s);

                consts[amount++] = 0x04;

                memcpy(&consts[amount], &len, sizeof(len));
                amount += sizeof(len);

                memcpy(&consts[amount], s, len);
                amount += len;
                break;
            }
            
            default:
                break;
        }
        
    }

    const uint8_t *constsFinal = consts;

    ConstBuffer cb = {0};
    cb.data = malloc(amount);
    cb.length = amount;
    memcpy(cb.data, constsFinal, cb.length);
    res.cb = cb;
    
    return res;
}
