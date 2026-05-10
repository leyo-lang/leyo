#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/errors.h"

#define STRICT_MODE 1

static Error errors[100];
static int error_count = 0;

bool isErr = false;


static FILE *logFile = NULL;

void logBuildLexer(const char *msg) {
    if (!logFile) return;

    fprintf(logFile, "[BUILD][LEXER] %s\n", msg);
    fflush(logFile); // ensures it is written immediately
}

void logBuildParser(const char *msg) {
    if (!logFile) return;

    fprintf(logFile, "[BUILD][PARSER] %s\n", msg);
    fflush(logFile); // ensures it is written immediately
}

void logRuntime(const char *msg) {
    if (!logFile) return;

    fprintf(logFile, "[RUNTIME] %s\n", msg);
    fflush(logFile); // ensures it is written immediately
}

void logController(const char *msg) {
    if (!logFile) return;

    fprintf(logFile, "[CONTROLLER] %s\n", msg);
    fflush(logFile); // ensures it is written immediately
}

void _logError(const char *msg, int line, int collumn) {
    if (!logFile) return;

    fprintf(logFile, "[ERROR] Line:%d Collumn:%d %s\n", line, collumn, msg);
    fflush(logFile); // ensures it is written immediately
}

void raise(const char *msg, int line, int col) {
    _logError(msg, line, col);
    isErr = true;

    if (error_count >= 100) return;

    strncpy(errors[error_count].message,
            msg,
            sizeof(errors[error_count].message) - 1);

    errors[error_count].message[sizeof(errors[error_count].message) - 1] = '\0';

    errors[error_count].line = line;
    errors[error_count].column = col;

    error_count++;

    if (STRICT_MODE) {
        callAllErr();
    }
}

void callAllErr() {
    for (int i = 0; i < error_count; i++) {
        Error currErr = errors[i];
        printf("Error at position %d:%d: %s\n\n",
               currErr.line,
               currErr.column,
               currErr.message);
    }
    quick_exit(1);
}


void initLog(const char *filename) {
    logFile = fopen(filename, "w"); // overwrite each run
    if (!logFile) {
        raise("Failed to open log file", 0,0);
        callAllErr();
    }
}