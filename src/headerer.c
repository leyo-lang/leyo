#include "../include/errors.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include "../include/version.h"
#include "../include/headerer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



ByteCodeResult headThis(ByteCodeResult bcr) {
    logController("Wrapping bytecode with Leyo header");
    
    LeyoHeader header = {
        .magic = {'L', 'Y', 'B', 'C'},
        .flags = 0,
        .code_size = bcr.length,
        .const_size = bcr.cb.length
    };

    snprintf(header.version, sizeof(header.version), "%s", LEYO_VERSION);

    int header_size = sizeof(LeyoHeader);
    int total_size = header_size + bcr.length + bcr.cb.length;

    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "Header size=%d payload=%d consts=%d total=%d",
                 header_size,
                 bcr.length,
                 bcr.cb.length,
                 total_size);
        logController(buffer);
    }

    uint8_t* new_data = malloc(total_size);

    // Copy header first
    memcpy(new_data, &header, header_size);

    // Copy bytecode after header
    memcpy(new_data + header_size,
           bcr.data,
           bcr.length);

    if (bcr.cb.length > 0 && bcr.cb.data != NULL) {
        memcpy(new_data + header_size + bcr.length,
               bcr.cb.data,
               bcr.cb.length);
    }

    ByteCodeResult result = {
        .length = total_size,
        .data = new_data,
        .cb = bcr.cb
    };

    logController("Header wrapping finished");

    return result;
}

ByteCodeResult unheadThis(ByteCodeResult bcr) {
    logController("Removing Leyo header");

    if (bcr.length < (int)sizeof(LeyoHeader)) {
        raise("Bytecode too small to contain a valid header", 0, 0);
        callAllErr();
        return (ByteCodeResult){0};
    }

    LeyoHeader header;
    memcpy(&header, bcr.data, sizeof(LeyoHeader));

    if (memcmp(header.magic, "LYBC", 4) != 0) {
        raise("Invalid Leyo bytecode magic", 0, 0);
        callAllErr();
        return (ByteCodeResult){0};
    }

    if (header.code_size + header.const_size + sizeof(LeyoHeader) > (uint32_t)bcr.length) {
        raise("Corrupt Leyo bytecode file", 0, 0);
        callAllErr();
        return (ByteCodeResult){0};
    }

    ByteCodeResult result = {
        .length = header.code_size,
        .data = bcr.data + sizeof(LeyoHeader),
        .cb = {
            .length = header.const_size,
            .data = bcr.data + sizeof(LeyoHeader) + header.code_size
        }
    };

    {
        char buffer[128];
        snprintf(buffer,
                 sizeof(buffer),
                 "Read header: code=%u consts=%u version=%s",
                 header.code_size,
                 header.const_size,
                 header.version);
        logController(buffer);
    }

    logController("Header removed");

    return result;
}



ByteCodeResult headThisLYPK(ByteCodeResult bcr) {
    logController("Wrapping bytecode into LYPK package");

    LeyoPKHeader header = {
        .magic = {'L', 'Y', 'P', 'K'},
        .flags = 0,
        .code_size = bcr.length,
        .const_size = bcr.cb.length
    };

    int header_size = sizeof(LeyoPKHeader);

    int total_size =
        header_size +
        bcr.length +
        bcr.cb.length;

    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "LYPK header=%d code=%d const=%d total=%d",
                 header_size,
                 bcr.length,
                 bcr.cb.length,
                 total_size);
        logController(buffer);
    }

    uint8_t *new_data = malloc(total_size);
    if (!new_data) {
        raise("Failed to allocate LYPK package", 0, 0);
        callAllErr();
        return (ByteCodeResult){0};
    }

    // Copy header
    memcpy(new_data, &header, header_size);

    // Copy bytecode
    memcpy(new_data + header_size,
           bcr.data,
           bcr.length);

    // Copy constants
    if (bcr.cb.length > 0 && bcr.cb.data != NULL) {
        memcpy(new_data + header_size + bcr.length,
               bcr.cb.data,
               bcr.cb.length);
    }

    ByteCodeResult result = {
        .length = total_size,
        .data = new_data,
        .cb = bcr.cb
    };

    logController("LYPK packaging complete");

    return result;
}

ByteCodeResult unheadThisLYPK(ByteCodeResult pkg) {
    logController("Unwrapping LYPK package");

    ByteCodeResult result = {0};

    if (pkg.length < (int)sizeof(LeyoPKHeader)) {
        raise("LYPK package too small", 0, 0);
        callAllErr();
        return result;
    }

    LeyoPKHeader *header = (LeyoPKHeader *)pkg.data;

    if (memcmp(header->magic, "LYPK", 4) != 0) {
        raise("Invalid LYPK magic", 0, 0);
        callAllErr();
        return result;
    }

    int expected =
        sizeof(LeyoPKHeader) +
        header->code_size +
        header->const_size;

    if (pkg.length < expected) {
        raise("LYPK package truncated", 0, 0);
        callAllErr();
        return result;
    }

    result.length = header->code_size;
    result.data = malloc(header->code_size);

    if (!result.data) {
        raise("Memory allocation failed", 0, 0);
        callAllErr();
        return (ByteCodeResult){0};
    }

    memcpy(
        result.data,
        pkg.data + sizeof(LeyoPKHeader),
        header->code_size
    );

    result.cb.length = header->const_size;

    if (header->const_size > 0) {
        result.cb.data = malloc(header->const_size);

        if (!result.cb.data) {
            free(result.data);
            raise("Memory allocation failed", 0, 0);
            callAllErr();
            return (ByteCodeResult){0};
        }

        memcpy(
            result.cb.data,
            pkg.data + sizeof(LeyoPKHeader) + header->code_size,
            header->const_size
        );
    }

    logController("LYPK unwrapped successfully");

    return result;
}