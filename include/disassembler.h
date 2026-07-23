/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stdio.h>

/// @brief Gets the string name of an operand.
/// @param op The current operand. 
/// @return The name of the operand, as a string.
const char* opcode_name(uint8_t op);

void disassemble(const uint8_t* code, size_t size);
void disassembleHex(const uint8_t* code, size_t size);

int dis(char *filename, bool flag_justHex, bool flag_head);

#endif
