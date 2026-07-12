#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "../include/type.h"

typedef struct {
    char *name;
    uint16_t slot;
    TokenType type;
    bool isEmpty;
    int id;
} Slot;

typedef enum {
    VAL_FLOAT,
    VAL_INT,
    VAL_STR,
    VAL_CHAR,
} ValueFlag;

typedef struct {
    ValueFlag flag;
    union {
        int i;
        double f;
        char c;
        char *s; // sterilise later
    } as;
} Value;

typedef struct {
    char *name;
    TokenType type;
} Arg;

typedef struct {
    char *name;
    uint32_t address;
    TokenType retType;
    Arg *args;
    uint16_t argCount;
} Func;

typedef struct {
    Token *tokens;
    uint32_t pos;
    uint32_t count;

    uint8_t *bytebuff;
    uint32_t byteIndex;
    uint32_t byteCap;

    Slot slots[65535];
    int slotCount;

    Value *consts;
    int constAmt;

    Func *funcs;
    int funcAmt;

    char funcPrefix[256];

    char **modulesLoaded;
    int moduleAmt;
    int moduleCap;
} ByteCoder;

typedef struct {
    uint8_t *data;
    int length;
} ConstBuffer;

typedef struct {
    uint8_t *data;
    int length;
    ConstBuffer cb;
} ByteCodeResult;

ByteCodeResult parse(TokenStream *ts);

#endif
