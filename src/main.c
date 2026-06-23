#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/errors.h"
#include "../include/vm.h"
#include "../include/version.h"
#include "../include/headerer.h"
#include "../include/disassembler.h"
#include "../include/repl.h"
#include "../include/lyst.h"
#include "../include/args.h"
#include "../include/tests.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

#ifndef GIT_DIRTY
#define GIT_DIRTY "unknown"
#endif

const char *tokenTypeName(TokenType t) {
    switch (t) {
        case STRING: return "STRING";
        case NUMBER: return "NUMBER";
        case IDENTIFIER: return "IDENTIFIER";
        case OPENBRAC: return "OPENBRAC";
        case CLOSEBRAC: return "CLOSEBRAC";
        case OPENBRACE: return "OPENBRACE";
        case CLOSEBRACE: return "CLOSEBRACE";
        case OPENSQUARE: return "OPENSQUARE";
        case CLOSESQUARE: return "CLOSESQUARE";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case OPERATION: return "OPERATION";
        case CONDITION: return "CONDITION";
        case EQUALS: return "EQUALS";
        case UNKNOWN: return "UNKNOWN";
        case ENDOFSTREAM: return "ENDOFSTREAM";
        case CHR: return "CHAR";
        case FLT: return "FLOAT";
        case NATIVE: return "NATIVE";
        default: return "???";
    }
}

void printTokenStream(TokenStream ts) {
    logController("Printing token stream");

    for (int i = 0; i < ts.count; i++) {
        Token t = ts.stream[i];

        printf("Token %d:\n", i);
        printf("  Type   : %s\n", tokenTypeName(t.type));
        printf("  Value  : %s\n", t.value);
        printf("  Line   : %d\n", t.line);
        printf("  Column : %d\n", t.collumn);
        printf("\n");
    }

    logController("Finished printing token stream");
}

void printByteCode(ByteCodeResult* bc) {
    logController("Printing bytecode output");

    for (int i = 0; i < bc->length; i++) {
        printf("%02x ", bc->data[i]);
    }
    printf("\n");

    logController("Finished printing bytecode output");
}

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

typedef struct {
    const char *name;
    const char *usage;
    const char *description;
} HelpEntry;

static const HelpEntry helpEntries[] = {
    {"help", "help [query]", "Show all CLI commands or filter by a search term"},
    {"init", "init [--defaults]", "Create or rewrite .lyst configuration"},
    {"build", "build <source> [output]", "Compile Leyo source into .lybc output"},
    {"run", "run <file.lybc>", "Execute compiled bytecode in the VM"},
    {"repl", "repl", "Start the interactive evaluator"},
    {"test", "test", "Run the built-in smoke test"},
    {"disassemble", "disassemble <file.lybc> [--hex] [--head]", "Inspect bytecode in readable or hex form"},
    {"dis", "dis <file.lybc> [--hex] [--head]", "Short alias for disassemble"},
    {"do", "do", "Build using paths from .lyst"},
};

static const size_t helpEntryCount = sizeof(helpEntries) / sizeof(helpEntries[0]);

static bool containsIgnoreCase(const char *haystack, const char *needle) {
    if (!needle || !*needle) {
        return true;
    }

    size_t needleLen = strlen(needle);
    size_t hayLen = strlen(haystack);

    if (needleLen > hayLen) {
        return false;
    }

    for (size_t i = 0; i + needleLen <= hayLen; i++) {
        size_t j = 0;
        while (j < needleLen) {
            unsigned char a = (unsigned char)haystack[i + j];
            unsigned char b = (unsigned char)needle[j];

            if (tolower(a) != tolower(b)) {
                break;
            }

            j++;
        }

        if (j == needleLen) {
            return true;
        }
    }

    return false;
}

static bool helpEntryMatches(const HelpEntry *entry, const char *query) {
    if (!query || !*query) {
        return true;
    }

    return containsIgnoreCase(entry->name, query) ||
           containsIgnoreCase(entry->usage, query) ||
           containsIgnoreCase(entry->description, query);
}

static void printHelp(const char *query) {
    printf("Leyo CLI Help%s%s\n",
           query && *query ? " - " : "",
           query && *query ? query : "");
    puts("================================");
    puts("");

    printf("Usage: leyo <command> [args]\n");
    puts("");

    puts("Commands:");
    for (size_t i = 0; i < helpEntryCount; i++) {
        const HelpEntry *entry = &helpEntries[i];
        if (!helpEntryMatches(entry, query)) {
            continue;
        }

        printf("  %-28s %s\n", entry->usage, entry->description);
    }

    puts("");
    puts("Global Flags:");
    puts("  --version, -v        Print the current version");
    puts("  --diagnostics, -D    Print build and runtime diagnostics");
    puts("");
    puts("Notes:");
    puts("  - help uses the first positional argument as a search term");
    puts("  - init --defaults writes the default .lyst without prompting");
}

static void diagnostics(void) {
    puts("Leyo Diagnostics");
    puts("================");

    printf("Version      : %s\n", getVersion());
    printf("Build Date   : %s %s\n", __DATE__, __TIME__);
    printf("Compiler     : %s\n", __VERSION__);
    printf("Platform     : %s\n", platformName());
    printf("Architecture : %s\n", archName());

    printf("Dirty        : %s\n", GIT_DIRTY);
    printf("Git Commit   : %s\n", GIT_COMMIT);

    printf("Value Size   : %zu bytes\n", sizeof(Value));
//    printf("Stack Size   : %d\n", STACK_MAX);    TODO

    puts("================");
}

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

static bool runInitWizard(void) {
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
        fprintf(stderr, "Failed to write .lyst\n");
        return false;
    }

    printf("Wrote .lyst with logging path %s\n", logPath);
    return true;
}

static bool writeDefaultLyst(void) {
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

static LogConfig readLogConfig(void) {
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

int dis(char *filename, bool flag_justHex, bool flag_head) {
    logController("Disassembly started");
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "File=%s Hex=%s Head=%s",
                 filename,
                 flag_justHex ? "true" : "false",
                 flag_head ? "true" : "false");
        logController(buffer);
    }

    FILE* fp = fopen(filename, "rb");

    if (!fp) {
        logController("Failed to open disassembly target");
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    uint8_t* data = malloc(size);

    if (!data) {
        fclose(fp);
        return 1;
    }

    fread(data, 1, size, fp);
    fclose(fp);

    const size_t ENTRY_POINT = 0x4C;

    if ((size_t)size <= ENTRY_POINT) {
        logController("Disassembly target too small");
        fprintf(stderr, "File too small\n");
        free(data);
        return 1;
    } 

    if (flag_head) {
        if (flag_justHex) {
            disassembleHex(data + ENTRY_POINT, size - ENTRY_POINT);
        } else {
            disassemble(
                data + ENTRY_POINT,
                size - ENTRY_POINT
            );
        }
    } else {
        if (flag_justHex) {
            disassembleHex(data, size);
        } else {
            disassemble(
                data,
                size
            );
        }
    } 

    free(data);

    return 0;
}

int build(char *filename, char *bcrfilename) {
    logController("Build started");
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Input=%s Output=%s", filename, bcrfilename);
        logController(buffer);
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        logController("Failed to open input file");
        raise("File open error", 0, 0);
        return 1;
    }

    logController("Input file opened successfully");

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        logController("Memory allocation failed for file buffer");
        fclose(file);
        return 1;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    logController("File loaded into memory");

    TokenStream ts = tokenise(buffer);
    logController("Tokenisation completed");

    printTokenStream(ts);

    if (isErr) {
        logController("Errors detected after tokenisation");
        callAllErr();
    }

    ByteCodeResult bcr = headThis(parse(&ts));

    logController("Parsing to bytecode completed");

    if (!(bcr.data == NULL || bcr.length == 0)) {
        logController("Bytecode successfully generated");
    } else {
        logController("Bytecode generation failed or empty");
    }

    printByteCode(&bcr);

    if (isErr) {
        logController("Errors detected after parsing stage");
        callAllErr();
    }

    logController("Program built successfully");

    FILE* filebcr = fopen(bcrfilename, "wb");

    if (!filebcr) {
        logController("Fail to open bcr file");
        raise("Failed to open file", 0,0);
        callAllErr();
    }

    fwrite(bcr.data, 1, bcr.length, filebcr);

    fclose(filebcr);

    return 0;
}

int run(char *filename) {
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Run target=%s", filename);
        logController(buffer);
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        raise("Failed to open .lybc file", 0, 0);
        callAllErr();
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Read header
    LeyoHeader header;
    if (fread(&header, sizeof(LeyoHeader), 1, file) != 1) {
        raise("Failed to read header", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    long payloadSize = fileSize - (long)sizeof(LeyoHeader);
    if (payloadSize < 0 || payloadSize < (long)header.code_size) {
        raise("Bytecode file is truncated", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    // Validate magic
    if (memcmp(header.magic, "LYBC", 4) != 0) {
        raise("Invalid bytecode magic", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    // Allocate bytecode buffer
    uint8_t *code = malloc(header.code_size);
    if (!code) {
        raise("Memory allocation failed", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    // Read bytecode
    if (fread(code, 1, header.code_size, file) != header.code_size) {
        raise("Failed to read bytecode", 0, 0);
        callAllErr();
        free(code);
        fclose(file);
        return -1;
    }

    long constSize = payloadSize - (long)header.code_size;
    uint8_t *constData = NULL;
    if (constSize > 0) {
        constData = malloc((size_t)constSize);
        if (!constData) {
            raise("Memory allocation failed", 0, 0);
            callAllErr();
            free(code);
            fclose(file);
            return -1;
        }

        if (fread(constData, 1, (size_t)constSize, file) != (size_t)constSize) {
            raise("Failed to read const pool", 0, 0);
            callAllErr();
            free(constData);
            free(code);
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    ByteCodeResult bcr = {0};
    bcr.length = header.code_size;
    bcr.data = code;
    bcr.cb.data = constData;
    bcr.cb.length = (int)constSize;

    logRuntime("VM START");

    int result = runVM(bcr);

    // IMPORTANT: only free original pointer
    free(code);
    free(constData);

    return result;
}

int main(int argc, char *argv[]) {
    ArgParser parser;
    argParseSetup(&parser, argv, argc);

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
        raise("Initialisation Failed", 0,0);
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
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Arg parser ready: command=%s flags=%d options=%d positionals=%d",
                 parser.command ? parser.command : "(none)",
                 parser.flagAmount,
                 parser.optionAmount,
                 parser.positionalAmount);
        logController(buffer);
    }

    const char *version = getVersion();

    logController("Running Leyo");
    logController(version);

    logController("Initialising ArgParser");
    logController("Initialised");

    if (parser.noCommand) {
        if (parser.flagAmount == 0) {
            printf("Leyo version v%s\nAuthored by Josh Ruddick\nhttps://github.com/JoshRuds/leyo", version);
            return 0;
        }

        if (isFlag(&parser, "--version") || isFlag(&parser, "-v")) {
            printf("%s\n", version);
            return 0;
        } else if (isFlag(&parser, "--diagnostics") || isFlag(&parser, "-D")) {
            diagnostics();
            return 0;
        }

        logController("Unknown global flag");
        raise("Unknown global flag", 0, 0);
        callAllErr();
    }

    if (isCommand(&parser, "build")) {

        char *source = getPositional(&parser, 0);

        if (!source) {
            logController("Missing source file");
            raise("Missing source file", 0, 0);
            callAllErr();
        }

        char *dest = getOption(&parser, "-o");

        if (!dest) {
            dest = "a.lybc";
        }

        return build(source, dest);

    } else if (isCommand(&parser, "run")) {

        char *source = getPositional(&parser, 0);

        if (!source) {
            logController("Missing source file");
            raise("Missing source file", 0, 0);
            callAllErr();
        }

        return run(source);

    } else if (isCommand(&parser, "repl")) {

        return repl();

    } else if (isCommand(&parser, "test")) {

        return testLeyo(getBin(&parser));

    } else if (isCommand(&parser, "disassemble") || isCommand(&parser, "dis")) {

        char *file = getPositional(&parser, 0);

        if (!file) {
            logController("Missing input file");
            raise("Missing input file", 0, 0);
            callAllErr();
        }

        bool hex = isFlag(&parser, "--hex");
        bool head = isFlag(&parser, "--head");

        return dis(file, hex, head);

    } else if (isCommand(&parser, "do")) {
        logController("Running configured build command");

        char *in = (char*)lystGet("build/in");
        char *out = (char*)lystGet("build/out");
        return build(in, out);

    } else {

        logController("Unknown command line argument");
        raise("Unknown command line argument", 0, 0);
        callAllErr();
    }

    return -1;
}
