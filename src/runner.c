#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/parser.h"
#include "../include/vm.h"
#include "../include/headerer.h"
#include "../include/errors.h"
#include "../include/codes.h"
#include "../include/version.h"

int run(char *filename, bool verbose) {
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Run target=%s", filename);
        logController(buffer);
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        lraise(ERR_FILE_OPEN_ERROR, 0, 0);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Read header
    LeyoHeader header;
    if (fread(&header, sizeof(LeyoHeader), 1, file) != 1) {
        lraise(ERR_INVALID_BYTECODE_HEADER, 0, 0);
        fclose(file);
        return -1;
    }

    long payloadSize = fileSize - sizeof(LeyoHeader);
    if (payloadSize < 0 || payloadSize < (long)header.code_size) {
        lraise(ERR_INVALID_BYTECODE, 0, 0);
        fclose(file);
        return -1;
    }

    // Validate magic
    if (memcmp(header.magic, "LYBC", 4) != 0) {
        lraise(ERR_INVALID_BYTECODE_HEADER, 0, 0);
        fclose(file);
        return -1;
    }

    if (strcmp(header.version, LEYO_VERSION) != 0) {
        lraise(ERR_WARN_DIFFERERENT_VERSIONS, 0,0);
    }

    // Allocate bytecode buffer
    uint8_t *code = malloc(header.code_size);
    if (!code) {
        lraise(ERR_VM_CANNOT_ALLOCATE, 0, 0);
        fclose(file);
        return -1;
    }

    // Read bytecode
    if (fread(code, 1, header.code_size, file) != header.code_size) {
        lraise(ERR_CANNOT_READ_BYTECODE, 0, 0);
        free(code);
        fclose(file);
        return -1;
    }

    long constSize = payloadSize - (long)header.code_size;
    uint8_t *constData = NULL;
    if (constSize > 0) {
        constData = malloc((size_t)constSize);
        if (!constData) {
            lraise(ERR_VM_CANNOT_ALLOCATE, 0, 0);
            free(code);
            fclose(file);
            return -1;
        }

        if (fread(constData, 1, (size_t)constSize, file) != (size_t)constSize) {
            lraise(ERR_VM_INVALID_CONST_POOL, 0, 0);
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

    int result = runVM(bcr, verbose);

    // IMPORTANT: only free original pointer
    free(code);
    free(constData);

    return result;
}