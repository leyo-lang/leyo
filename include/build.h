/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUILD_H
#define BUILD_H

#include <stdbool.h>

/// @brief Builds a leyo file into a lybc file and saves it.
/// @param filename The filename of the leyo input file.
/// @param bcrfilename The output filename of where to store the resulting bytecode.
/// @param isFlnameScript A boolean that defines the @p filename as a leyo script.
/// @param dump A boolean to toggle having the parser and tokeniser dump the tokenstream and bytecode.
/// @retval 0 Building was successful.
/// @retval 1 Building failed.
int build(char *filename, char *bcrfilename, bool isFlnameScript, bool dump);

#endif
