/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "../include/type.h"

typedef struct {
    char *name;
    uint16_t slot;
    TokenType type;
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
    char *name;
    uint32_t address;
    TokenType retType;
    // todo add args 
} Func;

typedef struct {
    Token *tokens;
    uint32_t pos;
    uint32_t count;

    uint8_t *bytebuff;
    uint32_t byteIndex;
    uint32_t byteCap;

    Global globals[65535];
    int globalCount;

    Value *consts;
    uint64_t constAmt;

    Func *funcs;
    int funcAmt;

    char funcPrefix[256];

    char **modulesLoaded;
    int moduleAmt;
    int moduleCap;

    char currentFileName[512];
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

ByteCodeResult parse(TokenStream *ts, char *currentFileName);

#endif
