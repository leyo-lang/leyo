#include <stdio.h>
#include "../include/errors.h"

char *getVersion(void) {
    logController("Fetching Version...");
    FILE *vfile = fopen("VERSION.txt", "r");
    if (!vfile) {
        logController("Failed to open input file");
        raise("File open error", 0, 0);
        callAllErr();
    }
    logController("Version Fetched.");
    static char buff[64];
    fgets(buff, 64, vfile);
    return buff;
}