#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "../include/codes.h"

enum { LOG_PATH_MAX = 512 };

typedef struct {
    bool enabled;
    bool rotate;
    bool build;
    bool runtime;
    bool controller;
    bool errors;
    int retentionDays;
    int retentionAction;
    char path[LOG_PATH_MAX];
    char archivePath[LOG_PATH_MAX];
} LogConfig;

void lraise(WhereFrom wf, ErrorCode code, int line, int col, char filename[512]);
void callAllErr(void);

extern bool isErr;

void setLogConfig(LogConfig config);
const LogConfig *getLogConfig(void);
void closeLog(void);
void initLog(const char *filename);

void logBuildLexer(const char *msg);
void logBuildParser(const char *msg);
void logRuntime(const char *msg);
void logController(const char *msg);

void inertLogs(void);

#endif
