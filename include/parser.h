#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "../include/type.h"

typedef struct {
    char *name;
    uint16_t slot;
} Global;

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
    Token *tokens;
    int pos;
    int count;

    uint8_t bytebuff[2048];
    int byteIndex;

    Global globals[65535];
    int globalCount;

    Value *consts;
    int constAmt;
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
