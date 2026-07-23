/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "../include/lyst.h"
#include "../include/errors.h"
#include "../include/codes.h"

static void trimInput(char *text) {
    size_t len = strlen(text);

    while (len > 0) {
        unsigned char c = (unsigned char)text[len - 1];
        if (c != '\n' && c != '\r' && !isspace(c)) {
            break;
        }
        text[--len] = '\0';
    }

    size_t start = 0;
    while (text[start] && isspace((unsigned char)text[start])) {
        start++;
    }

    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

static void promptText(const char *label, const char *fallback, char *out, size_t outSize) {
    char buffer[512];

    printf("%s [%s]: ", label, fallback);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        snprintf(out, outSize, "%s", fallback);
        return;
    }

    trimInput(buffer);

    if (buffer[0] == '\0') {
        snprintf(out, outSize, "%s", fallback);
        return;
    }

    snprintf(out, outSize, "%s", buffer);
}

static bool promptBool(const char *label, bool fallback) {
    char buffer[32];

    printf("%s [%s]: ", label, fallback ? "Y/n" : "y/N");
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return fallback;
    }

    trimInput(buffer);

    if (buffer[0] == '\0') {
        return fallback;
    }

    if (strcmp(buffer, "y") == 0 || strcmp(buffer, "Y") == 0 || strcmp(buffer, "yes") == 0 || strcmp(buffer, "YES") == 0 || strcmp(buffer, "true") == 0) {
        return true;
    }

    if (strcmp(buffer, "n") == 0 || strcmp(buffer, "N") == 0 || strcmp(buffer, "no") == 0 || strcmp(buffer, "NO") == 0 || strcmp(buffer, "false") == 0) {
        return false;
    }

    return fallback;
}

static int promptInt(const char *label, int fallback, int minValue) {
    char buffer[64];

    printf("%s [%d]: ", label, fallback);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return fallback;
    }

    trimInput(buffer);

    if (buffer[0] == '\0') {
        return fallback;
    }

    char *end = NULL;
    long value = strtol(buffer, &end, 10);

    if (end == buffer || *end != '\0' || value < minValue) {
        return fallback;
    }

    return (int)value;
}

static bool writeInitLyst(
    bool loggingEnabled,
    bool loggingRotate,
    bool loggingBuild,
    bool loggingRuntime,
    bool loggingController,
    bool loggingErrors,
    int retentionDays,
    const char *retentionAction,
    const char *logPath,
    const char *archivePath,
    const char *buildIn,
    const char *buildOut
) {
    FILE *fp = fopen(".lyst", "w");

    if (!fp) {
        return false;
    }

    fputs("; Leyo settings\n\n", fp);
    fputs("[logging]\n", fp);
    fprintf(fp, "enabled = %s\n", loggingEnabled ? "true" : "false");
    fprintf(fp, "rotate = %s\n", loggingRotate ? "true" : "false");
    fprintf(fp, "build = %s\n", loggingBuild ? "true" : "false");
    fprintf(fp, "runtime = %s\n", loggingRuntime ? "true" : "false");
    fprintf(fp, "controller = %s\n", loggingController ? "true" : "false");
    fprintf(fp, "errors = %s\n", loggingErrors ? "true" : "false");
    fprintf(fp, "retention_days = %d\n", retentionDays);
    fprintf(fp, "retention_action = %s\n", retentionAction);
    fprintf(fp, "path = %s\n", logPath);
    fprintf(fp, "archive_path = %s\n\n", archivePath);
    fputs("[build]\n", fp);
    fprintf(fp, "in = %s\n", buildIn);
    fprintf(fp, "out = %s\n", buildOut);

    fclose(fp);
    return true;
}

bool runInitWizard(void) {
    char logPath[260];
    char archivePath[260];
    char buildIn[260];
    char buildOut[260];

    puts("Leyo setup");
    puts("===========");
    puts("Press Enter to accept a default value.");
    puts("");

    FILE *existing = fopen(".lyst", "r");
    if (existing) {
        fclose(existing);
        if (!promptBool("Overwrite existing .lyst?", false)) {
            puts("Setup cancelled.");
            return false;
        }
    }

    bool loggingEnabled = promptBool("Enable logging?", true);
    bool loggingRotate = promptBool("Rotate the active log on startup?", true);
    bool loggingBuild = promptBool("Enable build logs?", true);
    bool loggingRuntime = promptBool("Enable runtime logs?", true);
    bool loggingController = promptBool("Enable controller logs?", true);
    bool loggingErrors = promptBool("Enable error logs?", true);
    int retentionDays = promptInt("Delete/archive logs older than how many days? 0 disables cleanup", 30, 0);
    const char *retentionAction = "archive";

    if (retentionDays > 0) {
        retentionAction = promptBool("Archive old logs instead of deleting them?", true) ? "archive" : "delete";
    }

    promptText("Active log path", "logs/latest.lylog", logPath, sizeof(logPath));
    promptText("Archive folder for old logs", "logs/archive/", archivePath, sizeof(archivePath));
    promptText("Default build input file", "test.leyo", buildIn, sizeof(buildIn));
    promptText("Default build output file", "test.lybc", buildOut, sizeof(buildOut));

    if (!writeInitLyst(
            loggingEnabled,
            loggingRotate,
            loggingBuild,
            loggingRuntime,
            loggingController,
            loggingErrors,
            retentionDays,
            retentionAction,
            logPath,
            archivePath,
            buildIn,
            buildOut)) {
        lraise(WF_GENERAL, ERR_FILE_WRITE_ERROR, 0,0, NULL);
        return false;
    }

    printf("Wrote .lyst with logging path %s\n", logPath);
    return true;
}

bool writeDefaultLyst(void) {
    return writeInitLyst(
        true,
        true,
        true,
        true,
        true,
        true,
        30,
        "archive",
        "logs/latest.lylog",
        "logs/archive/",
        "test.leyo",
        "test.lybc"
    );
}

static int parseRetentionAction(const char *value) {
    if (!value) {
        return 1;
    }

    if (strcmp(value, "delete") == 0) {
        return 0;
    }

    return 1;
}

LogConfig readLogConfig(void) {
    LogConfig config = {0};

    config.enabled = lystGetBool("logging/enabled", true);
    config.rotate = lystGetBool("logging/rotate", true);
    config.build = lystGetBool("logging/build", true);
    config.runtime = lystGetBool("logging/runtime", true);
    config.controller = lystGetBool("logging/controller", true);
    config.errors = lystGetBool("logging/errors", true);
    config.retentionDays = lystGetInt("logging/retention_days", 30);
    config.retentionAction = parseRetentionAction(lystGet("logging/retention_action"));

    const char *path = lystGet("logging/path");
    const char *archive = lystGet("logging/archive_path");

    if (!path || !*path) {
        path = "logs/latest.lylog";
    }

    if (!archive || !*archive) {
        archive = "logs/archive/";
    }

    snprintf(config.path, sizeof(config.path), "%s", path);
    snprintf(config.archivePath, sizeof(config.archivePath), "%s", archive);

    return config;
}

