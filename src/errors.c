#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include "../include/errors.h"
#include "../include/codes.h"
#include "../include/codes.h"

#define STRICT_MODE 0

static RaisedError errors[100];
static int error_count = 0;

bool isErr = false;
bool isInert = false;

static FILE *logFile = NULL;
static LogConfig logConfig = {
    .enabled = true,
    .rotate = true,
    .build = true,
    .runtime = true,
    .controller = true,
    .errors = true,
    .retentionDays = 0,
    .retentionAction = 1,
    .path = "logs/latest.lylog",
    .archivePath = "logs/archive/"
};

typedef struct {
    int archived;
    int deleted;
} RetentionStats;

static bool isSeparator(char c) {
    return c == '/' || c == '\\';
}

static bool pathExists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static bool endsWith(const char *value, const char *suffix) {
    size_t valueLen = strlen(value);
    size_t suffixLen = strlen(suffix);

    if (valueLen < suffixLen) {
        return false;
    }

    return strcmp(value + valueLen - suffixLen, suffix) == 0;
}

static void copyPath(char *dest, size_t destSize, const char *src) {
    if (destSize == 0) {
        return;
    }

    snprintf(dest, destSize, "%s", src ? src : "");
}

static void appendPath(char *dest, size_t destSize, const char *src) {
    size_t used = strlen(dest);

    if (used >= destSize - 1) {
        return;
    }

    strncat(dest, src, destSize - used - 1);
}

static void ensureTrailingSeparator(char *path, size_t size) {
    size_t len = strlen(path);

    if (len == 0) {
        return;
    }

    if (isSeparator(path[len - 1])) {
        return;
    }

    if (len + 1 >= size) {
        return;
    }

    path[len] = '/';
    path[len + 1] = '\0';
}

static void ensureParentDirectories(const char *path) {
    char partial[LOG_PATH_MAX];
    size_t len = 0;

    if (!path || !*path) {
        return;
    }

    memset(partial, 0, sizeof(partial));

    for (size_t i = 0; path[i] != '\0' && len < sizeof(partial) - 1; i++) {
        char c = path[i];

        if (isSeparator(c)) {
            partial[len] = '\0';

            if (len > 0) {
                mkdir(partial, 0777);
            }

            partial[len++] = c;
            continue;
        }

        partial[len++] = c;
    }
}

static void getDirectoryFromPath(const char *path, char *out, size_t outSize) {
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *sep = slash;

    if (!sep || (backslash && backslash > sep)) {
        sep = backslash;
    }

    if (!sep) {
        snprintf(out, outSize, ".");
        return;
    }

    size_t dirLen = (size_t)(sep - path) + 1;
    if (dirLen >= outSize) {
        dirLen = outSize - 1;
    }

    memcpy(out, path, dirLen);
    out[dirLen] = '\0';
}

static const char *getBaseName(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');

    if (!slash && !backslash) {
        return path;
    }

    if (!slash) {
        return backslash + 1;
    }

    if (!backslash) {
        return slash + 1;
    }

    return slash > backslash ? slash + 1 : backslash + 1;
}

static void buildArchiveTarget(const char *archiveDir, const char *sourceName, char *out, size_t outSize) {
    copyPath(out, outSize, archiveDir);
    appendPath(out, outSize, sourceName);

    if (!pathExists(out)) {
        return;
    }

    time_t now = time(NULL);
    struct tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif

    char stamp[64];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &tmNow);

    for (int suffix = 1; pathExists(out) && suffix < 1000; suffix++) {
        copyPath(out, outSize, archiveDir);
        appendPath(out, outSize, stamp);
        {
            char suffixPart[32];
            snprintf(suffixPart, sizeof(suffixPart), "-%d-", suffix);
            appendPath(out, outSize, suffixPart);
        }
        appendPath(out, outSize, sourceName);
    }
}

static void moveOrDeleteOldLog(const char *fullPath, const char *archiveDir, RetentionStats *stats) {
    if (logConfig.retentionAction == 0) {
        if (remove(fullPath) == 0) {
            stats->deleted++;
        }
        return;
    }

    char target[LOG_PATH_MAX];
    buildArchiveTarget(archiveDir, getBaseName(fullPath), target, sizeof(target));
    ensureParentDirectories(target);

    if (rename(fullPath, target) == 0) {
        stats->archived++;
    }
}

static void sweepCandidate(const char *fullPath, const char *archiveDir, time_t now, RetentionStats *stats) {
    struct stat st;

    if (stat(fullPath, &st) != 0) {
        return;
    }

    if (st.st_mtime <= 0) {
        return;
    }

    double ageSeconds = difftime(now, st.st_mtime);
    double threshold = (double)logConfig.retentionDays * 86400.0;

    if (ageSeconds < threshold) {
        return;
    }

    moveOrDeleteOldLog(fullPath, archiveDir, stats);
}

static void sweepLogsInDirectory(const char *logDir, const char *archiveDir, const char *activeBase, RetentionStats *stats) {
    time_t now = time(NULL);

#ifdef _WIN32
    char pattern[LOG_PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s*.lylog", logDir);

    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA(pattern, &data);

    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        if (strcmp(data.cFileName, activeBase) == 0) {
            continue;
        }

    char fullPath[LOG_PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s%s", logDir, data.cFileName);
        sweepCandidate(fullPath, archiveDir, now, stats);
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
#else
    DIR *dir = opendir(logDir);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' && entry->d_name[1] == '\0') {
            continue;
        }

        if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0') {
            continue;
        }

        if (!endsWith(entry->d_name, ".lylog")) {
            continue;
        }

        if (strcmp(entry->d_name, activeBase) == 0) {
            continue;
        }

        char fullPath[LOG_PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s%s", logDir, entry->d_name);
        sweepCandidate(fullPath, archiveDir, now, stats);
    }

    closedir(dir);
#endif
}

static void maintainLogs(void) {
    if (!logConfig.enabled || logConfig.retentionDays <= 0) {
        return;
    }

    char logDir[LOG_PATH_MAX];
    getDirectoryFromPath(logConfig.path, logDir, sizeof(logDir));

    char archiveDir[LOG_PATH_MAX];
    if (logConfig.archivePath[0] != '\0') {
        copyPath(archiveDir, sizeof(archiveDir), logConfig.archivePath);
    } else {
        copyPath(archiveDir, sizeof(archiveDir), logDir);
        appendPath(archiveDir, sizeof(archiveDir), "archive/");
    }

    char archiveProbe[LOG_PATH_MAX];
    copyPath(archiveProbe, sizeof(archiveProbe), archiveDir);
    appendPath(archiveProbe, sizeof(archiveProbe), "keep.lylog");
    ensureParentDirectories(archiveProbe);

    char activeBase[LOG_PATH_MAX];
    snprintf(activeBase, sizeof(activeBase), "%s", getBaseName(logConfig.path));

    RetentionStats stats = {0};
    sweepLogsInDirectory(logDir, archiveDir, activeBase, &stats);

    if (stats.archived > 0 || stats.deleted > 0) {
        char buffer[160];
        snprintf(buffer, sizeof(buffer),
                 "Log retention complete: archived=%d deleted=%d",
                 stats.archived,
                 stats.deleted);
        logController(buffer);
    }
}

static void rotateLogIfNeeded(const char *path) {
    if (!logConfig.rotate || !pathExists(path)) {
        return;
    }

    char directory[LOG_PATH_MAX];
    getDirectoryFromPath(path, directory, sizeof(directory));

    time_t now = time(NULL);
    struct tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif

    char stamp[64];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &tmNow);

    char candidate[LOG_PATH_MAX];
    copyPath(candidate, sizeof(candidate), directory);
    appendPath(candidate, sizeof(candidate), stamp);
    if (endsWith(path, ".lylog")) {
        copyPath(candidate, sizeof(candidate), directory);
        appendPath(candidate, sizeof(candidate), stamp);
        appendPath(candidate, sizeof(candidate), ".lylog");
    }

    for (int suffix = 1; pathExists(candidate) && suffix < 1000; suffix++) {
        copyPath(candidate, sizeof(candidate), directory);
        appendPath(candidate, sizeof(candidate), stamp);
        {
            char suffixPart[32];
            snprintf(suffixPart, sizeof(suffixPart), "-%d.lylog", suffix);
            appendPath(candidate, sizeof(candidate), suffixPart);
        }
    }

    if (rename(path, candidate) != 0) {
        lraise(WF_GENERAL, ERR_FILE_CANNOT_DEL, 0,0, NULL);
    }
}

static void writeTagged(const char *tag, const char *msg) {
    if (!logConfig.enabled || !logFile) {
        return;
    }

    fprintf(logFile, "%s %s\n", tag, msg ? msg : "");
    fflush(logFile);
}

void setLogConfig(LogConfig config) {
    if (config.path[0] == '\0') {
        snprintf(config.path, sizeof(config.path), "%s", "logs/latest.lylog");
    }

    if (config.archivePath[0] == '\0') {
        char dir[LOG_PATH_MAX];
        getDirectoryFromPath(config.path, dir, sizeof(dir));
        copyPath(config.archivePath, sizeof(config.archivePath), dir);
        appendPath(config.archivePath, sizeof(config.archivePath), "archive/");
    }

    ensureTrailingSeparator(config.archivePath, sizeof(config.archivePath));

    logConfig = config;
}

const LogConfig *getLogConfig(void) {
    return &logConfig;
}

void closeLog(void) {
    if (!logFile) {
        return;
    }

    fflush(logFile);
    fclose(logFile);
    logFile = NULL;
}

void inertLogs(void) {
    isInert = true;
}

void logBuildLexer(const char *msg) {
    if (isInert) {return;}
    if (!logConfig.enabled || !logConfig.build) return;
    writeTagged("[BUILD][LEXER]", msg);
}

void logBuildParser(const char *msg) {
    if (isInert) {return;}
    if (!logConfig.enabled || !logConfig.build) return;
    writeTagged("[BUILD][PARSER]", msg);
}

void logRuntime(const char *msg) {
    if (isInert) {return;}
    if (!logConfig.enabled || !logConfig.runtime) return;
    writeTagged("[RUNTIME]", msg);
}

void logController(const char *msg) {
    if (isInert) {return;}
    if (!logConfig.enabled || !logConfig.controller) return;
    writeTagged("[CONTROLLER]", msg);
}

void _logError(const char *msg, int line, int collumn) {
    if (!logConfig.enabled || !logConfig.errors) return;

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "Line:%d Column:%d %s", line, collumn, msg);
    writeTagged("[ERROR]", buffer);
}

const Error *lookupError(ErrorCode code) {
    for (unsigned int i = 0; i < errorTableAmt; i++) {
        if (errorTable[i].ec == code) {
            return &errorTable[i];
        }
    }

    return NULL;
}

void lraise(WhereFrom wf, ErrorCode code, int line, int col, char filename[512]) {
    const Error *err = lookupError(code);

    if (err == NULL) {
        _logError("Unknown error", line, col);
        return;
    }

    _logError(err->msg, line, col);
    isErr = true;

    if (error_count >= 100)
        return;

    errors[error_count].ec = code;
    errors[error_count].line = line;
    errors[error_count].column = col;
    errors[error_count].wf = wf;
    if (filename) {
        snprintf(errors[error_count].filename, 511, "%s", filename);
    }
    error_count++;

    if (err->fatal || STRICT_MODE) {
        exit(1);
    }
}

static void printErr(RaisedError *err, const Error *related) {
    if (err->wf == WF_GENERAL) {
        fprintf(stderr,
            "[%s] %s\n",
            related->name,
            related->msg);
    } else if (err->wf == WF_VM) {
        fprintf(stderr,
            "[%s] %s (ip: %d)\n",
            related->name,
            related->msg,
            err->line);
    } else if (err->wf == WF_PARSER) {
        fprintf(stderr,
            "[%s] %s (%s:%d:%d)\n",
            related->name,
            related->msg,
            err->filename,
            err->line,
            err->column);
    }
}

void callAllErr(void) {
    for (int i = 0; i < error_count; i++) {
        const Error *err = lookupError(errors[i].ec);

        printErr(&errors[i], err);
    }
}

void initLog(const char *filename) {
    if (isInert) {
        return;
    }
    const char *path = filename && filename[0] ? filename : logConfig.path;

    if (!logConfig.enabled) {
        closeLog();
        return;
    }

    if (!path || !*path) {
        path = "logs/latest.lylog";
    }

    snprintf(logConfig.path, sizeof(logConfig.path), "%s", path);
    ensureParentDirectories(logConfig.path);
    rotateLogIfNeeded(logConfig.path);
    closeLog();

    logFile = fopen(logConfig.path, "w");

    if (!logFile) {
        lraise(WF_GENERAL, ERR_FILE_OPEN_WARN, 0,0, NULL),
        logConfig.enabled = false;
        return;
    }

    maintainLogs();
}
