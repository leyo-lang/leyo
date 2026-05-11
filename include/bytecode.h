#ifndef BYTECODE_H
#define BYTECODE_H

#define BC_START_END 0xFF
#define BC_END_OF_LINE 0xF0
#define BC_IDENT_DELIM 0xFE
#define BC_EXPR_DELIM 0xFD
#define BC_VAR_DECL_COMMON 0x1
#define BC_VAR_DECL_INT 0x1
#define BC_VAR_DECL_FLT 0x3
#define BC_VAR_DECL_CHR 0x5
#define BC_VAR_DECL_STR 0x7
#define BC_ASSIGN 0x21
#define BC_TRACE 0x80
#define BC_PRINT 0x81
#define BC_ALLOW 0x82

#endif