#include <string.h>
#include <stdbool.h>

char *flags[];
int flagAmount;
char *command;
bool noCommand;

int argParseSetup(char* argv[], int argc) { // returns 1 if command is existent
    // argv[0] `leyo`
    if (argc <= 1) {
        noCommand = true;
        return true;
    }
    // argc > 1
    // argv[1] cmd `build`
    if (argv[1][0] == '-') {
        noCommand = true;
        return true;
    }
}

bool isFlag(char *flag) {
    for (int i; i < flagAmount; i++) {
        if (strcmp(flags[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

bool isCommand(char *cmd) {
    if (noCommand) {
        return false;
    }
    if (strcmp(command, cmd) == 0) {
        return true;
    }
    return false;
}