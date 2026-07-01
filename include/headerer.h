#ifndef HEADERER_H
#define HEADERER_H

#include "../include/parser.h"


typedef struct {
    char magic[4];
    char version[64];
    uint16_t flags;
    uint32_t code_size;
    uint32_t const_size;
} LeyoHeader;





ByteCodeResult headThis(ByteCodeResult);
ByteCodeResult unheadThis(ByteCodeResult);

ByteCodeResult headThisLYPK(ByteCodeResult);
ByteCodeResult unheadThisLYPK(ByteCodeResult);

#endif
