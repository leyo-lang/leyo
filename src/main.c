#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/build.h"
#include "../include/runner.h"
#include "../include/errors.h"
#include "../include/vm.h"
#include "../include/version.h"
#include "../include/headerer.h"
#include "../include/disassembler.h"
#include "../include/repl.h"
#include "../include/lyst.h"
#include "../include/args.h"
#include "../include/tests.h"
#include "../include/diagnostics.h"
#include "../include/helper.h"
#include "../include/wizard.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

#ifndef GIT_DIRTY
#define GIT_DIRTY "unknown"
#endif

void openGithub(void) {
#ifdef _WIN32
    system("start https://github.com/JoshRuds/Leyo");
#elif __APPLE__
    system("open https://github.com/JoshRuds/Leyo");
#else
    system("xdg-open https://github.com/JoshRuds/Leyo");
#endif
}

int main(int argc, char *argv[]) {
    ArgParser parser;
    argParseSetup(&parser, argv, argc);

    if (isFlag(&parser, "-s") || isFlag(&parser, "--speed")) {
        inertLogs();
    }

    if (isCommand(&parser, "help")) {
        printHelp(getPositional(&parser, 0));
        return 0;
    }

    if (isCommand(&parser, "init")) {
        if (isFlag(&parser, "--defaults") || isFlag(&parser, "-y")) {
            return writeDefaultLyst() ? 0 : -1;
        }

        return runInitWizard() ? 0 : -1;
    }

    if (!lystLoad(".lyst")) {
        fprintf(stderr, "Failed to load .lyst configuration\n");
        lraise("Initialisation Failed", 0,0);
        callAllErr();
        return -1;
    }

    LogConfig logConfig = readLogConfig();
    setLogConfig(logConfig);
    initLog(logConfig.path);
    atexit(closeLog);

    logController("Logger initialized");
    logController("LYST configuration loaded");
    logController(logConfig.path);

    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "Arg parser ready: command=%s flags=%d options=%d positionals=%d",
             parser.command ? parser.command : "(none)",
             parser.flagAmount,
             parser.optionAmount,
             parser.positionalAmount);
    logController(buffer);

    logController("Fetching Version...");
    const char *version = LEYO_VERSION;
    logController("Version Fetched.");

    logController("Running Leyo");
    logController(version);

    logController("Initialising ArgParser");
    logController("Initialised");

    if (parser.noCommand) {
        if (parser.flagAmount == 0) {
            printf("Leyo version v%s\nAuthored by Josh Ruddick\nRun `leyo github` for github link\n", version);
            return 0;
        }

        if (isFlag(&parser, "--version") || isFlag(&parser, "-v")) {
            printf("%s\n", version);
            return 0;
        } else if (isFlag(&parser, "--diagnostics") || isFlag(&parser, "-D")) {
            diagnostics(LEYO_VERSION, __VERSION__, GIT_DIRTY, GIT_COMMIT);
            return 0;
        }

        logController("Unknown global flag");
        lraise("Unknown global flag", 0, 0);
        callAllErr();
    }

    

    if (isCommand(&parser, "build")) {
        bool isScript = false;

        char *source = getPositional(&parser, 0);

        if (!source) {
            char *script = getOption(&parser, "-S");
            if (script) {
                isScript = true;
                source = script;
            } else {
                logController("Missing source file");
                lraise("Missing source file", 0, 0);
                callAllErr();
            }
        }

        char *dest = getOption(&parser, "-o");

        if (!dest) {
            dest = "a.lybc";
        }

        return build(source, dest, isScript);

    } else if (isCommand(&parser, "run")) {

        char *source = getPositional(&parser, 0);

        if (!source) {
            logController("Missing source file");
            lraise("Missing source file", 0, 0);
            callAllErr();
        }

        return run(source, isFlag(&parser, "-V") || isFlag(&parser, "--verbose"));

    } else if (isCommand(&parser, "repl")) {

        return repl();

    } else if (isCommand(&parser, "test")) {

        return testLeyo(getBin(&parser));

    } else if (isCommand(&parser, "disassemble") || isCommand(&parser, "dis")) {

        char *file = getPositional(&parser, 0);

        if (!file) {
            logController("Missing input file");
            lraise("Missing input file", 0, 0);
            callAllErr();
        }

        bool hex = isFlag(&parser, "--hex");
        bool head = !isFlag(&parser, "--head");

        return dis(file, hex, head);

    } else if (isCommand(&parser, "do")) {
        logController("Running configured build command");

        char *in = (char*)lystGet("build/in");
        char *out = (char*)lystGet("build/out");
        return build(in, out, false);

    } else if (isCommand(&parser, "github")) {
        logController("Opening Github");

        openGithub();
        return 0;

    } else if (isCommand(&parser, "update")) {
        logController("Leyo is checking for updates");

        //();
        return 0;

    } else if (isCommand(&parser, "rm")) {
        logController("Removing items");
        char conf = 0;
        printf("Confirm deletion (y/N): ");
        scanf(" %c", &conf);
        if (!(conf == 'y' || conf == 'Y')) {
            logController("No Removal Confimation, Exiting");
            printf("Cancelled.\n");
            return 0;
        }
        if (strcmp(getPositional(&parser, 0), "log") == 0) {
            #ifdef _WIN32
                system("del /Q logs\\*");
                system("del /Q logs\\archive\\*");
            #else
                system("rm -f logs/* 2>/dev/null");
                system("rm -f logs/archive/*");
            #endif
        } else {
            lraise("Unkown RM command", 0,0);
            callAllErr();
            return -1;
        }
        return 0;
    } else {

        logController("Unknown command line argument");
        lraise("Unknown command line argument", 0, 0);
        callAllErr();
    }

    return -1;
}
