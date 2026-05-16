#include "../include/type.h"
#include "../include/errors.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void logBuildParser(const char *msg);

typedef struct {
    Token *tokens;
    int pos;
    int count;
    uint8_t bytebuff[2048];
    int byteIndex;
    int hasHalfByte;
    uint8_t halfByte;
} ByteCoder;

typedef struct {
    uint8_t *data;
    int length;
} ByteCodeResult;

static char *VARDEFS[] = {
    "str",
    "int",
    "flt",
    "chr"
};
static int VARDEFCOUNT = 4;

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

static void writeHalfByte(uint8_t value) {
    if (!b->hasHalfByte) {
        b->halfByte = value & 0xF;
        b->hasHalfByte = 1;
        logBuildParser("Stored first half-byte");
    } else {
        uint8_t full = (b->halfByte << 4) | (value & 0xF);
        writeByte(full);
        b->hasHalfByte = 0;
        logBuildParser("Flushed half-byte pair into byte");
    }
}

static void writeRawExpr(char *expr, uint8_t opcode) {
    logBuildParser("Writing raw expression");

    uint8_t len = (uint8_t)strlen(expr);

    writeByte(opcode);
    writeByte(len);

    for (uint8_t i = 0; i < len; i++) {
        writeByte((uint8_t)expr[i]);
    }

    // delim - writeByte(BC_END_DELIM);
}

static void writeToken(Token tok) {
    switch (tok.type) {
        case CHR:
            writeRawExpr(tok.value, )
            break;
        
        default:
            break;
    }
}

static bool strInVarDef(char *str) {
    for (int i = 0; i < VARDEFCOUNT; i++) {
        if (strcmp(VARDEFS[i], str) == 0) {
            return true;
        }
    }
    return false;
}

static void parseAssign() {
    logBuildParser("Parsing assignments");

    writeByte(BC_ASSIGN);
    writeRawExpr(current().value, BC_IDENT_DELIM);
    expectAndPass(EQUALS, "Internal parser error");
    
    logBuildParser("Parsing expression for variable assignment");

    writeRawExpr(current().value, BC_EXPR_DELIM);
    /*if (previous().type != SEMICOLON) {
        raise("No semicolon after statement", previous().line, previous().collumn);
        callAllErr()
    }*/
    expectCurrent(SEMICOLON, "No semicolon after statement");
    logBuildParser("Finished assignment");
}

static void parseVarDecl() {
    logBuildParser("Parsing variable declaration");

    writeHalfByte(BC_VAR_DECL_COMMON);

    int arrOff = 0;

    if (strcmp(peek().value, "arr") == 0) {
        arrOff = 1;
        logBuildParser("Array declaration detected");
    }

    if (strcmp(current().value, "int") == 0) {
        writeHalfByte(BC_VAR_DECL_INT + arrOff);
    } else if (strcmp(current().value, "str") == 0) {
        writeHalfByte(BC_VAR_DECL_STR + arrOff);
    } else if (strcmp(current().value, "flt") == 0) {
        writeHalfByte(BC_VAR_DECL_FLT + arrOff);
    } else if (strcmp(current().value, "chr") == 0) {
        writeHalfByte(BC_VAR_DECL_CHR + arrOff);
    } else {
        logBuildParser("Invalid variable declaration type");
        raise("Internal Parser Error - Dead Var Decl",
            current().line,
            current().collumn);
        callAllErr();
    }

    if (arrOff) {
        advance();
    }

    expect(IDENTIFIER, "No identifier after var decl");

    logBuildParser("Parsing identifier for variable declaration");
    writeRawExpr(current().value, BC_IDENT_DELIM);

    expectAndPass(EQUALS, "No equals after var decl identifier");

    logBuildParser("Parsing expression for variable assignment");

    char expr[1024];
    int exprLoc = 0;

    while (current().type != SEMICOLON) {
        if (current().type == ENDOFSTREAM) {
            logBuildParser("No semicolom before EndOfStream");
            raise("No semicolon before EndOfStream", previous().line, previous().collumn);
            callAllErr();
        }
        char *val = current().value;
        for (int j = 0; val[j] != '\0'; j++) {
            expr[exprLoc++] = val[j];
        }
        advance();
    }

    expr[exprLoc] = '\0';

    writeRawExpr(expr, BC_EXPR_DELIM);

    expectCurrent(SEMICOLON, "No semicolon after statement");
    //advance(); // past semicolon

    logBuildParser("Finished variable declaration");
}

static void parseStatement() {
    logBuildParser("Parsing statement");

    switch (current().type) {
        case IDENTIFIER:
            if (strInVarDef(current().value)) {
                parseVarDecl();
            } else if (peek().type == EQUALS) { // assignments
                parseAssign();
            } else if (strcmp(current().value, "trace") == 0) {
                writeByte(BC_TRACE);
                expectAndPass(SEMICOLON, "No semicolon after statement");
            } else if (strcmp(current().value, "allow") == 0) {
                writeByte(BC_ALLOW);
                expect(IDENTIFIER, "NO ALLOWED ITEMS");
                writeRawExpr(current().value, BC_IDENT_DELIM);
                expectAndPass(SEMICOLON, "No semicolon after statement");
            } else {
                logBuildParser("Unknown identifier in body");
                raise("Unknown identifier in body", current().line, current().collumn);
            }
            break;

        default:
            logBuildParser("Unknown token in statement parser");
            raise("Unknown Token Type", current().line, current().collumn);
            break;
    }

    if (previous().type != SEMICOLON) {
        logBuildParser("No semicolom after statement (statement controller catch)");
        raise("No semicolon after statement", previous().line, previous().collumn);
        callAllErr();
    }
    writeByte(BC_END_OF_LINE);
    logBuildParser("End of statement");
}

ByteCodeResult parseToByteCode(TokenStream ts) {
    logBuildParser("Parser started");

    b = &bytecoder;
    b->tokens = ts.stream;
    b->pos = 0;
    b->byteIndex = 0;
    b->hasHalfByte = 0;

    writeByte(BC_START_END);

    while (current().type != ENDOFSTREAM) {
        parseStatement();
    }

    writeByte(BC_START_END);

    if (b->hasHalfByte) {
        logBuildParser("Unpaired half-byte detected at end of parsing");
        raise("Unpaired half-byte", current().line, current().collumn);
        callAllErr();
    }

    logBuildParser("Parser finished successfully");

    ByteCodeResult res = {0};
    res.data = b->bytebuff;
    res.length = b->byteIndex;

    return res;
}