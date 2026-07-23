/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: MIT
 */

#include "../include/args.h"
#include "../include/errors.h"
#include "../include/codes.h"

#include <stdio.h>
#include <string.h>

#define AP_MAX_ARGS 256

/// @brief Custom ArgParser setup script.
/// @param parser The argparser struct. Owned by user.
/// @param argc The number of command-line arguments.
/// @param argv The command-line arguments.
void argParseSetup(ArgParser *parser, char *argv[], int argc) {
    static char *flags[AP_MAX_ARGS];
    static char *positionals[AP_MAX_ARGS];
    static char *optionKeys[AP_MAX_ARGS];
    static char *optionValues[AP_MAX_ARGS];

    parser->command = NULL;
    parser->noCommand = true;

    parser->flags = flags;
    parser->positionals = positionals;
    parser->optionKeys = optionKeys;
    parser->optionValues = optionValues;

    parser->flagAmount = 0;
    parser->positionalAmount = 0;
    parser->optionAmount = 0;

    parser->bin = NULL;

    if (!argv || !argc) {
        logController("Arg parser initialised with no arguments");
        return;
    }

    parser->bin = argv[0];

    if (argc <= 1) {
        return;
    }

    int i = 1;

    if (argv[1][0] != '-') {
        parser->command = argv[1];
        parser->noCommand = false;
        i = 2;
    }

    while (i < argc) {
        char *arg = argv[i];

        if (arg[0] == '-') {

            /* option: --threads 8 */
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                parser->optionKeys[parser->optionAmount] = arg;
                parser->optionValues[parser->optionAmount] = argv[i + 1];
                parser->optionAmount++;

                i += 2;
                continue;
            }

            /* flag: -v */
            parser->flags[parser->flagAmount++] = arg;
        } else {
            /* positional argument */
            parser->positionals[parser->positionalAmount++] = arg;
        }

        i++;
    }

    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Arg parser ready: command=%s flags=%d options=%d positionals=%d",
                 parser->command ? parser->command : "(none)",
                 parser->flagAmount,
                 parser->optionAmount,
                 parser->positionalAmount);
        logController(buffer);
    }
}

/// @brief Checks whether specified flag is present.
/// @param parser The argparser struct. Owned by user.
/// @param flag The flag to check.
/// @return true if flag is existent, else false
bool isFlag(ArgParser *parser, const char *flag) {
    for (int i = 0; i < parser->flagAmount; i++) {
        if (strcmp(parser->flags[i], flag) == 0) {
            return true;
        }
    }

    return false;
}

/// @brief Checks whether specified command is present.
/// @param parser The argparser struct. Owned by user.
/// @param cmd The command to check.
/// @return true if command is existent, else false
bool isCommand(ArgParser *parser, const char *cmd) {
    if (parser->noCommand || parser->command == NULL) {
        return false;
    }

    return strcmp(parser->command, cmd) == 0;
}

/// @brief Get the option provided.
/// @param parser The argparser struct. Owned by user.
/// @param option The option flag to get (eg. "-o").
/// @return The option, or NULL if none.
char *getOption(ArgParser *parser, const char *option) {
    for (int i = 0; i < parser->optionAmount; i++) {
        if (strcmp(parser->optionKeys[i], option) == 0) {
            return parser->optionValues[i];
        }
    }

    return NULL;
}

/// @brief Get the positional argument at specified position.
/// @param parser The argparser struct. Owned by user.
/// @param index The index on which positional to fetch.
/// @return The positional, or NULL if none.
char *getPositional(ArgParser *parser, int index) {
    if (index < 0 || index >= parser->positionalAmount) {
        return NULL;
    }

    return parser->positionals[index];
}

/// @brief Gets name of binary that run leyo.
/// @param parser The argparser struct. Owned by user.
/// @return The parser binary name, or NULL if none.
char *getBin(ArgParser *parser) {
    if (parser->bin) {
        return parser->bin;
    }

    return NULL;
}
