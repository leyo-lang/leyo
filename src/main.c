#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/errors.h"
#include "../include/vm.h"
#include "../include/version.h"
#include "../include/headerer.h"

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



int build(char *filename, char *bcrfilename) {
    logController("Build started");

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

    ByteCodeResult bcr = headThis(parse(ts));

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
    FILE *file = fopen(filename, "rb");
    if (!file) {
        raise("Failed to open .lybc file", 0, 0);
        callAllErr();
        return -1;
    }

    // Read header
    LeyoHeader header;
    if (fread(&header, sizeof(LeyoHeader), 1, file) != 1) {
        raise("Failed to read header", 0, 0);
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

    fclose(file);

    // Build VM input (strip 0xFF before execution if you want)
    ByteCodeResult bcr;
    bcr.length = header.code_size;
    bcr.data = code;

    logRuntime("VM START");

    int result = runVM(bcr);

    // IMPORTANT: only free original pointer
    free(code);

    return result;
}

int main(int argc, char *argv[]) {
    initLog("logs/latest.lylog");
    logController("Logger initialized");
    
    const char *version = getVersion();

    logController("Running Leyo");
    logController("Version:");
    logController(version);


    if (argc == 1) { //no cl-arg
        printf("Leyo version v%s\nAuthored by Josh Ruddick", version);
        return 0;
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc == 3) {
            argv[argc++] = "a.lybc";
        } else if (argc != 4) {
            logController("Too little command line args");
            raise("Too little command line args", 0,0);
            callAllErr();
        }
        return build(argv[2], argv[3]);
    } else if (strcmp(argv[1], "run") == 0) {
        if (argc != 3) {
            logController("Too little command line args");
            raise("Too little command line args", 0,0);
            callAllErr();
        }
        return run(argv[2]);
    } else if (strcmp(argv[1], "repl") == 0) {
        ;
    } else if (strcmp(argv[1], "test") == 0) {
        ;
    } else {
        logController("Unkown command line argument");
        raise("Unkown command line argument", 0,0);
        callAllErr();
    }

    return 0;
}
