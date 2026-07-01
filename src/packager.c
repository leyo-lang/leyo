#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/packager.h"
#include "../include/parser.h"

LYPKG read_pkg(FILE *fp) {
    LYPKG pkg = {0};
    LeyoPKHeader hdr;

    if (fread(&hdr, sizeof(hdr), 1, fp) != 1)
        return pkg;

    if (memcmp(hdr.magic, "LYPK", 4) != 0) {
        fprintf(stderr, "Invalid package file.\n");
        return pkg;
    }

    /* ---------- Function Table ---------- */

    if (fread(&pkg.ft.funcAmt, sizeof(pkg.ft.funcAmt), 1, fp) != 1)
        return pkg;

    pkg.ft.funcs = malloc(pkg.ft.funcAmt * sizeof(Func));

    for (int i = 0; i < pkg.ft.funcAmt; i++) {
        uint32_t len;

        fread(&len, sizeof(len), 1, fp);

        pkg.ft.funcs[i].name = malloc(len + 1);
        fread(pkg.ft.funcs[i].name, 1, len, fp);
        pkg.ft.funcs[i].name[len] = '\0';

        fread(&pkg.ft.funcs[i].address,
              sizeof(pkg.ft.funcs[i].address), 1, fp);

        fread(&pkg.ft.funcs[i].retType,
              sizeof(pkg.ft.funcs[i].retType), 1, fp);
    }

    /* ---------- Bytecode ---------- */

    pkg.bc.length = hdr.code_size;
    pkg.bc.data = malloc(pkg.bc.length);

    fread(pkg.bc.data, 1, pkg.bc.length, fp);

    /* ---------- Constant Buffer ---------- */

    pkg.bc.cb.length = hdr.const_size;
    pkg.bc.cb.data = malloc(pkg.bc.cb.length);

    fread(pkg.bc.cb.data, 1, pkg.bc.cb.length, fp);

    return pkg;
}