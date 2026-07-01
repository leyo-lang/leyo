#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/vm.h"
#include "../include/headerer.h"
#include "../include/errors.h"
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

    // Validate magic
    if (memcmp(header.magic, "LYBC", 4) != 0) {
        raise("Invalid bytecode magic", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    // Validate file size matches header expectations
    long expectedSize = (long)sizeof(LeyoHeader)
                      + (long)header.code_size
                      + (long)header.const_size;

    if (fileSize != expectedSize) {
        raise("Bytecode file is corrupt or truncated", 0, 0);
        callAllErr();
        fclose(file);
        return -1;
    }

    // Allocate bytecode buffer
    uint8_t *code = NULL;
    if (header.code_size > 0) {
        code = malloc(header.code_size);
        if (!code) {
            raise("Memory allocation failed (code)", 0, 0);
            callAllErr();
            fclose(file);
            return -1;
        }

        if (fread(code, 1, header.code_size, file) != header.code_size) {
            raise("Failed to read bytecode", 0, 0);
            callAllErr();
            free(code);
            fclose(file);
            return -1;
        }
    }

    // Allocate constant pool buffer
    uint8_t *constData = NULL;
    if (header.const_size > 0) {
        constData = malloc(header.const_size);
        if (!constData) {
            raise("Memory allocation failed (const pool)", 0, 0);
            callAllErr();
            free(code);
            fclose(file);
            return -1;
        }

        if (fread(constData, 1, header.const_size, file) != header.const_size) {
            raise("Failed to read constant pool", 0, 0);
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
    bcr.cb.length = header.const_size;

    logRuntime("VM START");

    int result = runVM(bcr);

    free(code);
    free(constData);

    return result;
}
