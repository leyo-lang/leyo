#include "../include/type.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/bytecode.h"
#include "../include/native.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

ConstBuffer constBuf = {0};

ByteCoder bytecoder = {0};
ByteCoder *b;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_CHAR,
    TYPE_UNKNOWN
} VarType;


// HOISTS

static void parseStatement(void);

static void checkByteBuff(void) {
    logBuildParser("Checking ByteBuff Size");
    if (b->byteIndex >= b->byteCap - 1) {
        logBuildParser("Doubling ByteBuff Capacity");
        b->byteCap = b->byteCap * 2;
        b->bytebuff = realloc(b->bytebuff, b->byteCap * sizeof(uint8_t));
    }
}

static Token current(void) {
    return b->tokens[b->pos];
}

/*
static Token previous(void) {
    if (b->count-1==b->pos) {
        logBuildParser("Too far - previoused into eos");
        raise("Internal Parser Error: Too far - previoused into sos/eos", current().line, current().collumn);
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
    checkByteBuff();
    if (b->byteIndex >= b->byteCap) {
        logBuildParser("Byte buffer overflow detected");
        raise("Byte buffer overflow", current().line, current().collumn);
    }

    b->bytebuff[b->byteIndex++] = value;
}

static void emit16(uint16_t value) {
    emit((uint8_t)(value & 0xFF));
    emit((uint8_t)((value >> 8) & 0xFF));
}

static void emit32(uint32_t value) {
    emit((uint8_t)(value & 0xFF));
    emit((uint8_t)((value >> 8) & 0xFF));
    emit((uint8_t)((value >> 16) & 0xFF));
    emit((uint8_t)((value >> 24) & 0xFF));
}

static uint32_t reserve32(void) {
    uint32_t pos = b->byteIndex;
    emit32(0);
    return pos;
}

static void patch32(uint32_t loc, uint32_t value) {
    if (loc + 3 >= b->byteIndex) {
        logBuildParser("Invalid patch location");
        raise("Byte patch out of bounds", current().line, current().collumn);
        return;
    }

    b->bytebuff[loc+0] = (uint8_t)(value & 0xFF);
    b->bytebuff[loc+1] = (uint8_t)((value >> 8) & 0xFF);
    b->bytebuff[loc+2] = (uint8_t)((value >> 16) & 0xFF);
    b->bytebuff[loc+3] = (uint8_t)((value >> 24) & 0xFF);
}

static void constEmit(uint8_t v) {
    constBuf.data[constBuf.length++] = v;
}

static void addModule(const char *name) {
    if (b->moduleAmt == b->moduleCap) {
        b->moduleCap *= 2;
        b->modulesLoaded = realloc(
            b->modulesLoaded,
            sizeof(char *) * b->moduleCap
        );
    }

    b->modulesLoaded[b->moduleAmt++] = strdup(name);
}

static bool isModuleLoaded(const char *name) {
    for (int i = 0; i < b->moduleAmt; i++) {
        if (strcmp(b->modulesLoaded[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static TokenType getTypeVar(void) {
    if (strcmp(current().value, "int") == 0) {return NUMBER;}
    if (strcmp(current().value, "str") == 0) {return STRING;}
    if (strcmp(current().value, "flt") == 0) {return FLT;}
    if (strcmp(current().value, "chr") == 0) {return CHR;}
    return UNKNOWN;
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

static uint16_t emitConst(void) {
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

static bool isKeyword(char *tc) {
    char *keywords[] = {
        "int", "chr", "str", "flt", "arr",
        "rtn", "fnc", "whl", "rpt", "brk", "cnt"
    };

    int keywordAmt = sizeof(keywords) / sizeof(keywords[0]);

    for (int i = 0; i < keywordAmt; i++) {
        if (strcmp(keywords[i], tc) == 0) {
            raise("Cannot Overwrite Keyword", current().line, current().collumn);
            return true;
        }
    }

    return false;
}

static int define(char *name, TokenType type) {
    isKeyword(name);
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            raise("Previously Defined", current().line, current().collumn);
            return b->globals[i].slot;
        }
    }
    for (int i = 0; i < b->funcAmt; i++) {
        if (strcmp(b->funcs[i].name, name) == 0) {
            raise("Previously Defined", current().line, current().collumn);
            return b->funcs[i].address;
        }
    }
    


    int slot = b->globalCount;

    b->globals[b->globalCount].name = strdup(name);
    b->globals[b->globalCount].slot = slot;
    b->globals[b->globalCount].type = type;
    b->globalCount++;

    return slot;
}

static uint32_t definef(char *name, TokenType ret) {
    isKeyword(name);
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            raise("Previously Defined", current().line, current().collumn);
            return b->globals[i].slot;
        }
    }
    for (int i = 0; i < b->funcAmt; i++) {
        if (strcmp(b->funcs[i].name, name) == 0) {
            raise("Previously Defined", current().line, current().collumn);
            return b->funcs[i].address;
        }
    }

    b->funcs[b->funcAmt].name = strdup(name);
    b->funcs[b->funcAmt].address = b->byteIndex;
    b->funcs[b->funcAmt].retType = ret;
    b->funcAmt++;

    return b->byteIndex;
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

static uint32_t resolvef(char *name) {
    for (int i = 0; i < b->funcAmt; i++) {
        if (strcmp(b->funcs[i].name, name) == 0) {
            return b->funcs[i].address;
        }
    }

    if (strcmp(name, "main") == 0) {
        raise("Undefined func (HINT: 'main' entry point required)", current().line, current().collumn);
        callAllErr();
        return -1;
    }

    raise("Undefined func", current().line, current().collumn);
    callAllErr();
    return -1;
}

static TokenType resolveType(char *name) {
    for (int i = 0; i < b->globalCount; i++) {
        if (strcmp(b->globals[i].name, name) == 0) {
            return b->globals[i].type;
        }
    }

    raise("Undefined variable", current().line, current().collumn);

    callAllErr();
    return UNKNOWN;
}

static TokenType resolveTypef(char *name) {
    for (int i = 0; i < b->funcAmt; i++) {
        if (strcmp(b->funcs[i].name, name) == 0) {
            return b->funcs[i].retType;
        }
    }

    raise("Undefined function", current().line, current().collumn);
    callAllErr();
    return UNKNOWN;
}

static TokenType functionCall(void) {
    char *name = current().value;  
    logBuildParser("atom is ident func");
    emit(OP_CALL);
    emit32((int32_t)(resolvef(name) - (b->byteIndex + 4)));
    expectAndPass(OPENBRAC, "Function must be opened '('");
    while (current().type != CLOSEBRAC) {
        advance();
    }
    TokenType type = resolveTypef(name);
    advance();

    return type;
}

static void consumeStatementTerminator(const char *ctx) {
    if (current().type != SEMICOLON) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s: expected ';'", ctx ? ctx : "Statement");
        raise(buffer, current().line, current().collumn);
        callAllErr();
        return;
    }

    advance();
}

static TokenType parseAtom(void) { // small singular unit of expression (number, identifier, string, etc)
    logBuildParser("Parsing atom");

    switch (current().type) {
        case NUMBER: {
            logBuildParser("atom is number");
            emit(OP_CONST_LOAD);
            emit16((uint16_t)emitConst());
    
            break;
        }

        case CHR: {
            logBuildParser("atom is char");
            emit(OP_CONST_LOAD);
            emit16((uint16_t)emitConst());
    
            break;
        }

        case STRING: {
            logBuildParser("atom is string");
            emit(OP_CONST_LOAD);
            emit16((uint16_t)emitConst());
    
            break;
        }

        case FLT: {
            logBuildParser("atom is float");
            emit(OP_CONST_LOAD);
            emit16((uint16_t)emitConst());
    
            break;
        }

        case IDENTIFIER: {
            if (peek().type == OPENBRAC) {
                return functionCall();
            }
            logBuildParser("atom is ident");
            uint16_t slot = resolve(current().value);
            
            emit(OP_LOAD); // takes from slot given and return into top
            emit16((uint16_t)slot); // put slot into mem

            TokenType type = resolveType(current().value);
            advance();
            return type;

            break;
        }

        default: {
            raise("Unknown atom", current().line, current().collumn);
            callAllErr();
            break;
        }
    }
    TokenType type = current().type;
    advance();
    return type;
}

static TokenType parseExpression(void) {
    TokenType ltype = parseAtom(); // result -> top

    while (current().type == OPERATION) {
        char op = current().value[0];

        advance();

        TokenType rtype = parseAtom(); // right operand -> top | left -> 2nd

        if (ltype != rtype) {
            raise("Types in expression are conflicting", current().line, current().collumn);
            
            return UNKNOWN;
        }

        switch (op) {
            case '+': emit(OP_OPERATE_ADD); break;
            case '-': emit(OP_OPERATE_SUB); break;
            case '*': emit(OP_OPERATE_MUL); break;
            case '/': emit(OP_OPERATE_DIV); break;
            case '^': emit(OP_OPERATE_EXP); break;

            default:
                raise("Unknown operator",
                      current().line,
                      current().collumn);
                return UNKNOWN;
        }
    }
    
    return ltype;
}

static void parseExpressionTC(TokenType aim) {
    TokenType type = parseExpression();
    if (aim != type) {
        raise("Type is conflicting", current().line, current().collumn);
        return;
    }
    return;
}

static void parseAssign(void) {
    logBuildParser("Parsing assignment");

    char *name = current().value;
    uint16_t slot = resolve(name);

    expectAndPass(EQUALS, "Expected '='");

    parseExpressionTC(resolveType(name));

    emit(OP_STORE);
    emit16(slot);

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseVarDecl(void) {
    logBuildParser("Parsing variable declaration");

    if (current().type != IDENTIFIER) {
        raise("Expected type", current().line, current().collumn);
    };

    char *type = current().value;
    TokenType aim = getTypeVar();

    if (!(strcmp(type, "int") == 0 ||
          strcmp(type, "str") == 0 ||
          strcmp(type, "flt") == 0 ||
          strcmp(type, "chr") == 0)) {
        raise("Invalid type", current().line, current().collumn);
    }

    expect(IDENTIFIER, "Expected variable name");

    char *name = current().value;
    uint16_t slot = define(name, aim);

    expectAndPass(EQUALS, "Expected '='");

    parseExpressionTC(aim);

    emit(OP_STORE);
    emit16(slot);    

    expectCurrent(SEMICOLON, "Expected ';'");
}

static void parseNative(void) {
    logBuildParser("Parsing Native Call");
    advance(); //past @
    NativeCommand nc;

    if (strcmp(current().value, "log") == 0) {nc = NAT_LOG;} else
    if (strcmp(current().value, "dump") == 0) {nc = NAT_DUMP;} else
    if (strcmp(current().value, "trace") == 0) {nc = NAT_TRACE;} else
    {raise("Native Function Unkown", current().line, current().collumn); callAllErr(); return;}

    emit(OP_CALL_NATIVE);
    emit((uint8_t)nc);
    expectAndPass(SEMICOLON, "No semicolon after statement");    
}


static void parseFuncBody(TokenType retType) {
    while (current().type != CLOSEBRACE &&
           current().type != ENDOFSTREAM) {

        uint32_t before = b->pos;

        // RETURN HANDLING (no hacks, no eat)
        if (strcmp(current().value, "rtn") == 0) {
            advance();
            parseExpressionTC(retType);

            emit(OP_RETURN);

            expectCurrent(SEMICOLON, "Expected ';' after return");

            // IMPORTANT: return ends statement cleanly
            continue;
        }

        parseStatement();

        if (b->pos == before) {
            raise("Parser stalled inside function", current().line, current().collumn);
            break;
        }
    }
}

static void parseFunction(void) {
    logBuildParser("[FN] Enter parseFunction()");

    advance(); // past fnc
    logBuildParser("[FN] Consumed 'fnc'");

    bool runNow = false;

    if (strcmp(current().value, "!") == 0) {
        runNow = true;
        logBuildParser("[FN] Run-now flag detected");
        advance();
    }

    logBuildParser("[FN] Parsing function name");

    char *name = current().value;
    expectCurrent(IDENTIFIER, "Function must be named");

    logBuildParser("[FN] Function name captured");

    isKeyword(name);
    logBuildParser("[FN] Keyword check passed");

    expectCurrent(OPENBRAC, "[FN] Expect '(' for params");
    logBuildParser("[FN] Enter parameter list");

    while (current().type != CLOSEBRAC) {
        logBuildParser("[FN] Parsing param token");
        advance();
    }

    logBuildParser("[FN] End parameter list");

    advance(); // onto '<'
    logBuildParser("[FN] Parsing return type start");

    if (strcmp(current().value, "<") != 0) {
        logBuildParser("[FN] ERROR: missing '<'");
        raise("Requires opening type bracket '<'", current().line, current().collumn);
        callAllErr();
        return;
    }

    advance();
    logBuildParser("[FN] Reading return type");

    TokenType retType = getTypeVar();

    advance(); // onto '>'
    logBuildParser("[FN] Expecting '>'");

    if (strcmp(current().value, ">") != 0) {
        logBuildParser("[FN] ERROR: missing '>'");
        raise("Requires closing type bracket '>'", current().line, current().collumn);
        callAllErr();
        return;
    }

    logBuildParser("[FN] Return type parsed successfully");

    expectAndPass(OPENBRACE, "[FN] Expect function body '{'");
    logBuildParser("[FN] Enter function body");

    uint32_t reservedLoc = 0;
    uint32_t origin = 0;

    if (!runNow) {
        emit(OP_JUMP);
        reservedLoc = reserve32();
        origin = b->byteIndex;
        logBuildParser("[FN] Reserved jump patch slot");
    }

    definef(name, retType);
    logBuildParser("[FN] Function registered in symbol table");

    parseFuncBody(retType);

    logBuildParser("[FN] Function body closed");

    if (!runNow) {
        logBuildParser("[FN] Patching jump address");
        patch32(reservedLoc, b->byteIndex-origin);
    }

    advance();
    if (current().type == SEMICOLON) {
        advance();
    }

    logBuildParser("[FN] Function fully parsed");

    return;
}

static void parseModule(void) {
    char *name = current().value;
    if (isModuleLoaded(name)) {
        expectAndPass(SEMICOLON, "No Semicolon After Statement");
        return;
    }

    addModule(name);

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "pkg/%s.leyo", name);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        raise("Cannot open module", current().line, current().collumn);
        callAllErr();
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *src = malloc(size + 1);
    fread(src, 1, size, fp);
    src[size] = '\0';
    fclose(fp);

    TokenStream ts = tokenise(src);
    
    Token *oldTokens = b->tokens;
    uint32_t oldCount = b->count;
    uint32_t oldPos = b->pos;

    b->tokens = ts.stream;
    b->count = ts.count;
    b->pos = 0;

    while (current().type != ENDOFSTREAM) {
        logBuildParser("[MODULE] Entering parseStatement()");
        parseStatement();
        logBuildParser("[MODULE] Returned from parseStatement()");
    }

    // No OP_FINISH as more code to come

    free(ts.stream);

    b->tokens = oldTokens;
    b->count = oldCount;
    b->pos = oldPos;

    expectAndPass(SEMICOLON, "No Semicolon After Statement");
}

static void parseStatement(void) {
    logBuildParser("Parsing statement");

    switch (current().type) {
        case SEMICOLON:
            logBuildParser("[WARN] Lone SemiColon");
            advance();
            break;

        case NATIVE:
            if (strcmp(current().value, "@") == 0) {
                parseNative();
                break;
            }
            raise("Internal Parser Error: Lexer Failure Caught", current().line, current().collumn);
            callAllErr();
            break;

        case IDENTIFIER: {

            // type keywords OR variable name
            if (strcmp(current().value, "int") == 0 ||
                strcmp(current().value, "str") == 0 ||
                strcmp(current().value, "flt") == 0 ||
                strcmp(current().value, "chr") == 0) {

                parseVarDecl();
            } else if (strcmp(current().value, "fnc") == 0) {
                parseFunction();
            } else if (strcmp(current().value, "use") == 0) {
                advance();
                parseModule();
            } else if (peek().type == EQUALS) {
                parseAssign();
            } else if (peek().type == OPENBRAC) {
                functionCall();
                consumeStatementTerminator("Function call");
            } else {
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
    b->byteCap = 64;
    b->bytebuff = malloc(sizeof(uint8_t) * b->byteCap);
    b->byteIndex = 0;
    b->globalCount = 0;
    b->funcs = malloc(sizeof(Func) * 1024);
    b->funcAmt = 0;
    b->constAmt = 0;
    b->consts = malloc(sizeof(Value) * 1024);
    b->moduleCap = 8;
    b->moduleAmt = 0;
    b->modulesLoaded = malloc(sizeof(char *) * b->moduleCap);

    if (!b->funcs) {
        raise("Failed to allocate function table", 0, 0);
        callAllErr();
    }

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

    // Entry Point
    emit(OP_CALL);
    emit32((int32_t)(resolvef("main") - (b->byteIndex + 4)));

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

    free(b->bytebuff);
    for (int i = 0; i < b->moduleAmt; i++) {
        free(b->modulesLoaded[i]);
    }
    free(b->modulesLoaded);
    
    return res;
}
