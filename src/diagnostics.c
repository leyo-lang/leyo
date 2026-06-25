#include <stdio.h>
#include "../include/parser.h"

static const char *platformName(void) {
#ifdef _WIN32
    return "Windows";
#elif __linux__
    return "Linux";
#elif __APPLE__
    return "macOS";
#else
    return "Unknown";
#endif
}

static const char *archName(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__)
    return "ARM64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "Unknown";
#endif
}

void diagnostics(char *L_VERSION, char *VERSION, char *GIT_IS_DIRTY, char* GIT_WHAT_COMMIT) {
    puts("Leyo Diagnostics");
    puts("================");

    printf("Version      : %s\n", L_VERSION);
    printf("Build Date   : %s %s\n", __DATE__, __TIME__);
    printf("Compiler     : %s\n", VERSION);
    printf("Platform     : %s\n", platformName());
    printf("Architecture : %s\n", archName());

    printf("Dirty        : %s\n", GIT_IS_DIRTY);
    printf("Git Commit   : %s\n", GIT_WHAT_COMMIT);

    printf("Value Size   : %zu bytes\n", sizeof(Value));
//    printf("Stack Size   : %d\n", STACK_MAX);    TODO

    puts("================");
}