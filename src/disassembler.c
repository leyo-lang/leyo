#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/bytecode.h"
#include "../include/errors.h"

static uint16_t read16(const uint8_t *code, size_t *ip) {
    uint16_t value =
        (uint16_t)code[*ip] |
        ((uint16_t)code[*ip + 1] << 8);

    *ip += 2;

    return value;
}

const char* opcode_name(uint8_t op) {
    switch (op) {
        case OP_PUT_A:       return "PUT_A";
        case OP_PUT_B:       return "PUT_B";
        case OP_PUT_S:       return "PUT_S";

        case OP_PUT_A_R:     return "PUT_A_R";
        case OP_PUT_B_R:     return "PUT_B_R";

        case OP_OPERATE_MUL: return "MUL";
        case OP_OPERATE_DIV: return "DIV";
        case OP_OPERATE_ADD: return "ADD";
        case OP_OPERATE_SUB: return "SUB";
        case OP_OPERATE_EXP: return "EXP";

        case OP_STORE:       return "STORE";
        case OP_LOAD:        return "LOAD";

        case OP_SS_PUSH_A:   return "SS_PUSH_A";
        case OP_SS_PUSH_B:   return "SS_PUSH_B";
        case OP_SS_POP:      return "SS_POP";

        case OP_FINISH:      return "FINISH";

        case OP_CONST_LOAD:  return "CONST_LOAD";

        case OP_CALL_NATIVE: return "CALL_NATIVE";

        default:             return "UNKNOWN";
    }
}

int opcode_has_operand(uint8_t op) {
    switch (op)
    {
        case OP_PUT_A:
        case OP_PUT_B:
            return 2;

        case OP_CALL_NATIVE:
            return 1;

        default:
            return 0;
    }
}

void disassemble(const uint8_t* code, size_t size) {
    logController("Disassembler entered human-readable mode");
    size_t ip = 0;

    while (ip < size)
    {
        uint8_t op = code[ip++];

        if (opcode_has_operand(op))
        {
            if (ip >= size)
            {
                printf("%s <missing operand>\n", opcode_name(op));
                break;
            }

            printf("%s %u\n",
                   opcode_name(op),
                   read16(code,&ip));

            ip++;
        }
        else
        {
            printf("%s\n", opcode_name(op));

            if (op == OP_FINISH)
                break;
        }
    }

    logController("Disassembler finished human-readable mode");
}


void disassembleHex(const uint8_t* code, size_t size) {
    logController("Disassembler entered hex mode");
    size_t ip = 0;

    while (ip < size)
    {
        uint8_t op = code[ip++];

        printf("%02x ", op);

        if (op == OP_FINISH)
            break;
    }

    logController("Disassembler finished hex mode");
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