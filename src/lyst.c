/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../include/lyst.h"
#include "../include/errors.h"
#include "../include/codes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *lystTrim(char *s) {
    while (isspace((unsigned char)*s))
        s++;

    if (*s == '\0')
        return s;

    char *end = s + strlen(s) - 1;

    while (end > s && isspace((unsigned char)*end))
        *end-- = '\0';

    return s;
}

LystConfig cfgRoot = {0};
LystConfig *cfg;

int lystLoad(const char *filename) {
    cfg = &cfgRoot;
    cfg->count = 0;

    logController("Loading LYST configuration");

    FILE *fp = fopen(filename, "r");

    if (!fp) {
        logController("LYST configuration not found");
        return 0;
    }

    char line[512];
    char currentSection[64] = "";

    while (fgets(line, sizeof(line), fp)) {
        char *p = lystTrim(line);

        if (*p == '\0')
            continue;

        if (*p == '#' || *p == ';')
            continue;

        if (*p == '[') {
            char *end = strchr(p, ']');

            if (!end)
                continue;

            *end = '\0';

            strncpy(
                currentSection,
                p + 1,
                sizeof(currentSection) - 1
            );

            currentSection[sizeof(currentSection) - 1] = '\0';

            continue;
        }

        char *eq = strchr(p, '=');

        if (!eq)
            continue;

        *eq = '\0';

        char *key = lystTrim(p);
        char *value = lystTrim(eq + 1);

        if (*value == '"') {
            value++;

            char *endQuote = strrchr(value, '"');

            if (endQuote)
                *endQuote = '\0';
        }

        if (cfg->count >= LYST_MAX_ENTRIES)
            break;

        LystEntry *entry = &cfg->entries[cfg->count++];

        strncpy(
            entry->section,
            currentSection,
            sizeof(entry->section) - 1
        );

        entry->section[sizeof(entry->section) - 1] = '\0';

        strncpy(
            entry->key,
            key,
            sizeof(entry->key) - 1
        );

        entry->key[sizeof(entry->key) - 1] = '\0';

        strncpy(
            entry->value,
            value,
            sizeof(entry->value) - 1
        );

        entry->value[sizeof(entry->value) - 1] = '\0';
    }

    fclose(fp);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "LYST loaded with %d entries", cfg->count);
    logController(buffer);

    return 1;
}

const char *lystGet(const char *path) {
    char section[64];
    char key[64];

    const char *slash = strchr(path, '/');

    if (!slash)
        return NULL;

    size_t sectionLen = slash - path;

    if (sectionLen >= sizeof(section))
        return NULL;

    memcpy(section, path, sectionLen);
    section[sectionLen] = '\0';

    strcpy(key, slash + 1);

    for (int i = 0; i < cfg->count; i++) {
        LystEntry *entry = &cfg->entries[i];

        if (
            strcmp(entry->section, section) == 0 &&
            strcmp(entry->key, key) == 0
        ) {
            return entry->value;
        }
    }

    return NULL;
}

static bool lystEqualsIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

bool lystGetBool(const char *path, bool fallback) {
    const char *value = lystGet(path);

    if (!value) {
        return fallback;
    }

    if (lystEqualsIgnoreCase(value, "true") ||
        lystEqualsIgnoreCase(value, "yes") ||
        lystEqualsIgnoreCase(value, "on") ||
        strcmp(value, "1") == 0) {
        return true;
    }

    if (lystEqualsIgnoreCase(value, "false") ||
        lystEqualsIgnoreCase(value, "no") ||
        lystEqualsIgnoreCase(value, "off") ||
        strcmp(value, "0") == 0) {
        return false;
    }

    return fallback;
}

int lystGetInt(const char *path, int fallback) {
    const char *value = lystGet(path);

    if (!value || !*value) {
        return fallback;
    }

    char *end = NULL;
    long parsed = strtol(value, &end, 10);

    if (end == value || *end != '\0') {
        return fallback;
    }

    return (int)parsed;
}
