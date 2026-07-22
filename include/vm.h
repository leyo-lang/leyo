/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef VM_H
#define VM_H

#include "../include/parser.h"

int runVM(ByteCodeResult bc, bool verbose, char filename[512]); 

#endif
