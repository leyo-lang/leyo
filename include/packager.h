#ifndef PACK_H
#define PACK_H

#include <stdint.h>
#include <stdio.h>

#include "../include/parser.h"


typedef struct {
    char magic[4];        // "LYPK"

    uint32_t funcs_size;
    uint32_t code_size;
    uint32_t const_size;
} LeyoPKHeader;

typedef struct {
    ByteCodeResult bc;
    FuncTable ft;
} LYPKG;

LYPKG read_pkg(FILE *);

#endif