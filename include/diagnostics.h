/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

/// @brief Prints the diagnostic values.
/// @param L_VERSION The version leyo is running.
/// @param VERSION The compiler version.
/// @param GIT_IS_DIRTY A Yes/No of if the current workspace is dirty.
/// @param GIT_WHAT_COMMIT The commit tag at build time.
void diagnostics(char *L_VERSION, char *VERSION, char *GIT_IS_DIRTY, char* GIT_WHAT_COMMIT);

#endif
