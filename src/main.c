/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/build.h"
#include "../include/runner.h"
#include "../include/errors.h"
#include "../include/codes.h"
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
#include "../include/external.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

#ifndef GIT_DIRTY
#define GIT_DIRTY "unknown"
#endif

/// @brief The main entry point of the Leyo compiler.
/// @param argc The number of command-line arguments.
/// @param argv The command-line arguments.
/// @retval -1 A controller error occurred.
/// @retval 0 The program completed successfully.
/// @retval 1 A sub-program returned an error.
int main(int argc, char *argv[]) {
    // set atexits's
    atexit(callAllErr);
    atexit(closeLog);

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
        lraise(WF_GENERAL, ERR_LYST_FAILED_LOAD, 0,0, NULL);
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
        lraise(WF_GENERAL, ERR_UNKOWN_GLOBAL_FLAG, 0, 0, NULL);
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
                lraise(WF_GENERAL, ERR_MISSING_SOURCE_FILE, 0, 0, NULL);
            }
        }

        char *dest = getOption(&parser, "-o");

        if (!dest) {
            dest = "a.lybc";
        }

        return build(source, dest, isScript, isFlag(&parser, "-d"));

    } else if (isCommand(&parser, "run")) {

        char *source = getPositional(&parser, 0);

        if (!source) {
            logController("Missing source file");
            lraise(WF_GENERAL, ERR_MISSING_SOURCE_FILE, 0, 0, NULL);
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
            lraise(WF_GENERAL, ERR_MISSING_INPUT_FILE, 0, 0, NULL);
        }

        bool hex = isFlag(&parser, "--hex");
        bool head = !isFlag(&parser, "--head");

        return dis(file, hex, head);

    } else if (isCommand(&parser, "do")) {
        logController("Running configured build command");

        char *in = (char*)lystGet("build/in");
        char *out = (char*)lystGet("build/out");
        return build(in, out, false, false); // TODO fix up and add lyst setting

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
            lraise(WF_GENERAL, ERR_RM_COMMAND_UNKOWN, 0,0, NULL);
            return -1;
        }
        return 0;
    } else {
        logController("Unknown command line argument");
        lraise(WF_GENERAL, ERR_UNKNOWN_CLI_ARG, 0, 0, NULL);
    }

    return -1;
}
