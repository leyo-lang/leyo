#ifndef LYST_H
#define LYST_H

#include <stdbool.h>

#define LYST_MAX_ENTRIES 256

typedef struct {
    char section[64];
    char key[64];
    char value[256];
} LystEntry;

typedef struct {
    LystEntry entries[LYST_MAX_ENTRIES];
    int count;
} LystConfig;

int lystLoad(const char *filename);

const char *lystGet(const char *path);
bool lystGetBool(const char *path, bool fallback);
int lystGetInt(const char *path, int fallback);

#endif
