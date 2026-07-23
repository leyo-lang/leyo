/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *usage;
    const char *description;
} HelpEntry;

static const HelpEntry helpEntries[] = {
    {"help", "help [query]", "Show all CLI commands or filter by a search term"},
    {"init", "init [--defaults]", "Create or rewrite .lyst configuration"},
    {"build", "build <source> [output]", "Compile Leyo source into .lybc output"},
    {"run", "run <file.lybc> [-V, --verbose]", "Execute compiled bytecode in the VM"},
    {"repl", "repl", "Start the interactive evaluator"},
    {"test", "test", "Run the built-in smoke test"},
    {"disassemble", "disassemble <file.lybc> [--hex] [--head]", "Inspect bytecode in readable or hex form"},
    {"dis", "dis <file.lybc> [--hex] [--head]", "Short alias for disassemble"},
    {"do", "do", "Build using paths from .lyst"},
    {"github", "github", "Open Leyo github"}
};

static const size_t helpEntryCount = sizeof(helpEntries) / sizeof(helpEntries[0]);

static bool containsIgnoreCase(const char *haystack, const char *needle) {
    if (!needle || !*needle) {
        return true;
    }

    size_t needleLen = strlen(needle);
    size_t hayLen = strlen(haystack);

    if (needleLen > hayLen) {
        return false;
    }

    for (size_t i = 0; i + needleLen <= hayLen; i++) {
        size_t j = 0;
        while (j < needleLen) {
            unsigned char a = (unsigned char)haystack[i + j];
            unsigned char b = (unsigned char)needle[j];

            if (tolower(a) != tolower(b)) {
                break;
            }

            j++;
        }

        if (j == needleLen) {
            return true;
        }
    }

    return false;
}

static bool helpEntryMatches(const HelpEntry *entry, const char *query) {
    if (!query || !*query) {
        return true;
    }

    return containsIgnoreCase(entry->name, query) ||
           containsIgnoreCase(entry->usage, query) ||
           containsIgnoreCase(entry->description, query);
}

void printHelp(const char *query) {
    printf("Leyo CLI Help%s%s\n",
           query && *query ? " - " : "",
           query && *query ? query : "");
    puts("================================");
    puts("");

    printf("Usage: leyo <command> [args]\n");
    puts("");

    puts("Commands:");
    for (size_t i = 0; i < helpEntryCount; i++) {
        const HelpEntry *entry = &helpEntries[i];
        if (!helpEntryMatches(entry, query)) {
            continue;
        }

        printf("  %-28s %s\n", entry->usage, entry->description);
    }

    puts("");
    puts("Global Flags:");
    puts("  --version, -v        Print the current version");
    puts("  --diagnostics, -D    Print build and runtime diagnostics");
    puts("");
    puts("Notes:");
    puts("  -s and --speed can be used anywhere and disables logging for performance");
    puts("  - help uses the first positional argument as a search term");
    puts("  - init --defaults writes the default .lyst without prompting");
}
