#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stdio.h>

void disassemble(const uint8_t* code, size_t size);

#endif