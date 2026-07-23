/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>

typedef struct {
    char *command;
    bool noCommand;

    char **flags;
    int flagAmount;

    char **positionals;
    int positionalAmount;

    char **optionKeys;
    char **optionValues;
    int optionAmount;

    char *bin;
} ArgParser;

/// @brief Custom ArgParser setup script.
/// @param parser The argparser struct. Owned by user.
/// @param argc The number of command-line arguments.
/// @param argv The command-line arguments.
void argParseSetup(ArgParser *parser, char *argv[], int argc);

/// @brief Checks whether specified command is present.
/// @param parser The argparser struct. Owned by user.
/// @param cmd The command to check.
/// @return true if command is existent, else false
bool isCommand(ArgParser *parser, const char *cmd);

/// @brief Checks whether specified flag is present.
/// @param parser The argparser struct. Owned by user.
/// @param flag The flag to check.
/// @return true if flag is existent, else false
bool isFlag(ArgParser *parser, const char *flag);

/// @brief Get the option provided.
/// @param parser The argparser struct. Owned by user.
/// @param option The option flag to get (eg. "-o").
/// @return The option, or NULL if none.
char *getOption(ArgParser *parser, const char *option);

/// @brief Get the positional argument at specified position.
/// @param parser The argparser struct. Owned by user.
/// @param index The index on which positional to fetch.
/// @return The positional, or NULL if none.
char *getPositional(ArgParser *parser, int index);

/// @brief Gets name of binary that run leyo.
/// @param parser The argparser struct. Owned by user.
/// @return The parser binary name, or NULL if none.
char *getBin(ArgParser *parser);

#endif
