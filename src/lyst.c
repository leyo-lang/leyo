#include "../include/lyst.h"

#include <stdio.h>
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

    FILE *fp = fopen(filename, "r");

    if (!fp)
        return 0;

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
