#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stdio.h>

void disassemble(const uint8_t* code, size_t size);
void disassembleHex(const uint8_t* code, size_t size);

int dis(char *filename, bool flag_justHex, bool flag_head);

#endif
