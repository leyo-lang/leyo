#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

typedef struct {
    char message[256];
    int line;
    int column;
} Error;

void raise(const char *msg, int line, int col);
void callAllErr(void);

extern bool isErr;

void initLog(const char *filename);

void logBuildLexer(const char *msg);
void logBuildParser(const char *msg);
void logRuntime(const char *msg);
void logController(const char *msg);

#endif