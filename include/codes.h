#ifndef ERR_CODES_H
#define ERR_CODES_H

#include <stdbool.h>

typedef enum {
    ERR_VAR_NOT_DEFINED,
    ERR_AMOUNT
} ErrorCode;

typedef struct {
    ErrorCode ec; // like ERR_ZERO_DIV
    char *name; // like V001
    char *msg; // like Cannot divide by zero
    bool fatal; // if fatal or recoverable
} Error;

typedef struct {
    ErrorCode ec; // like ERR_ZERO_DIV
    int line;
    int column;
} RaisedError;

const Error errorTable[] = {
    {ERR_VAR_NOT_DEFINED,           "S002", "Variable not defined"},
};

#endif