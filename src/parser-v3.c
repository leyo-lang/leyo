#include "../include/type.h"
#include "../include/errors.h"
#include "../include/bytecode.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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