#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>

#define AP_MAX_FLAGS       128
#define AP_MAX_OPTIONS     128
#define AP_MAX_POSITIONALS 128

typedef struct {
    char *name;
    char *value;
} AP_Option;

typedef struct {
    char *command;

    char *flags[AP_MAX_FLAGS];
    int flag_count;

    AP_Option options[AP_MAX_OPTIONS];
    int option_count;

    char *positionals[AP_MAX_POSITIONALS];
    int positional_count;

    bool has_command;
} ArgParser;

#endif