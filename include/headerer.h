#ifndef HEADERER_H
#define HEADERER_H

#include "../include/parser.h"


typedef struct {
    char magic[4];
    char *version;
    uint16_t flags;
    uint32_t code_size;
} LeyoHeader;


ByteCodeResult headThis(ByteCodeResult);

#endif