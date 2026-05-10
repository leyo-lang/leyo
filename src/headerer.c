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
    LeyoHeader header = {
        .magic = {'L', 'Y', 'B', 'C'},
        .version = getVersion(),
        .flags = 0,
        .code_size = bcr.length
    };

    int header_size = sizeof(LeyoHeader);
    int total_size = header_size + bcr.length;

    uint8_t* new_data = malloc(total_size);

    // Copy header first
    memcpy(new_data, &header, header_size);

    // Copy bytecode after header
    memcpy(new_data + header_size,
           bcr.data,
           bcr.length);

    ByteCodeResult result = {
        .length = total_size,
        .data = new_data
    };

    return result;
}