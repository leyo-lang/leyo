/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/headerer.h"
#include "../include/errors.h"
#include "../include/codes.h"

/// @brief Gets the string representation of a token type.
/// @param type The token type.
/// @return The name of the specified token type, or NULL if the token type is unknown.
static const char *tokenTypeName(TokenType t) {
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
        case COLON: return "COLON";
        default: return "???";
    }
}

/// @brief Prints the provided token stream.
/// @param ts The token stream object.
static void printTokenStream(TokenStream ts) {
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

/// @brief Prints the provided bytecode.
/// @param bc The byte code result object.
static void printByteCode(ByteCodeResult* bc) {
    logController("Printing bytecode output");

    for (int i = 0; i < bc->length; i++) {
        printf("%02x ", bc->data[i]);
    }
    printf("\n");

    logController("Finished printing bytecode output");
}

int build(char *filename, char *bcrfilename, bool isFlnameScript, bool dump) {
    logController("Build started");
    char *buffer = NULL;

    if (isFlnameScript) {
        logController("Build Is A Script");
        buffer = filename;
        goto tokenising;
    }

    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Input=%s Output=%s", filename, bcrfilename);
        logController(buffer);
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        logController("Failed to open input file");
        lraise(WF_GENERAL, ERR_FILE_OPEN_ERROR, 0, 0, NULL);
        return 1;
    }

    logController("Input file opened successfully");

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    
    buffer = malloc(size + 1);
    if (!buffer) {
        logController("Memory allocation failed for file buffer");
        fclose(file);
        return 1;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    logController("File loaded into memory");

tokenising:
    ;
    TokenStream ts = tokenise(buffer);
    logController("Tokenisation completed");

    if (dump) {
       printTokenStream(ts); 
    }

    /*
    if (isErr) {
        logController("Errors detected after tokenisation");
        callAllErr();
    }
    */

    ByteCodeResult bcr = headThis(parse(&ts, filename));

    logController("Parsing to bytecode completed");

    if (!(bcr.data == NULL || bcr.length == 0)) {
        logController("Bytecode successfully generated");
    } else {
        logController("Bytecode generation failed or empty");
    }

    if (dump) {
        printByteCode(&bcr);
    }

    if (isErr) {
        logController("Errors detected after building. Exiting.");
        return 1;
    }
    
    logController("Program built successfully");

    FILE* filebcr = fopen(bcrfilename, "wb");

    if (!filebcr) {
        logController("Fail to open bcr file");
        lraise(WF_GENERAL, ERR_FILE_OPEN_ERROR, 0,0, NULL);
    }

    fwrite(bcr.data, 1, bcr.length, filebcr);

    fclose(filebcr);

    return 0;
}