#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "../include/type.h"

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

ByteCodeResult parse(TokenStream ts);

#endif