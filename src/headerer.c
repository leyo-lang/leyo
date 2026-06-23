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
        .code_size = bcr.length
    };

    snprintf(header.version, sizeof(header.version), "%s", getVersion());

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
