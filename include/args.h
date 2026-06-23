#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

typedef struct {
    char *command;
    bool noCommand;

    char **flags;
    int flagAmount;

    char **positionals;
    int positionalAmount;

    char **optionKeys;
    char **optionValues;
    int optionAmount;

    char *bin;
} ArgParser;

void argParseSetup(ArgParser *parser, char *argv[], int argc);

bool isCommand(ArgParser *parser, const char *cmd);
bool isFlag(ArgParser *parser, const char *flag);

char *getOption(ArgParser *parser, const char *option);
char *getPositional(ArgParser *parser, int index);
char *getBin(ArgParser *parser);

#endif