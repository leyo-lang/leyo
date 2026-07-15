#ifndef ERR_CODES_H
#define ERR_CODES_H

#include <stdbool.h>

typedef enum {
    ERR_FILE_OPEN_ERROR,
    ERR_LEX_UNTERMINATED_STR_LIT,
    ERR_LEX_INVALID_CHAR,
    ERR_LEX_OUT_OF_MEMORY,
    ERR_LEX_INVALID_NUM,
    ERR_PARSER_VAR_NOT_DEFINED,
    ERR_PARSER_VAR_PERVIOUSLY_DEFINED,
    ERR_PARSER_FUNC_NOT_DEFINED,
    ERR_PARSER_FUNC_PERVIOUSLY_DEFINED,
    ERR_LYST_FAILED_LOAD,
    ERR_UNKOWN_GLOBAL_FLAG,
    ERR_MISSING_SOURCE_FILE,
    ERR_MISSING_INPUT_FILE,
    ERR_RM_COMMAND_UNKOWN,
    ERR_UNKNOWN_CLI_ARG,
    ERR_NOT_IMPLEMENTED_YET,
    ERR_NOT_IMPLEMENTED,
    ERR_PARSER_INTO_ENDOFSTREAM,
    ERR_PARSER_INTO_STARTOFSTREAM,
    ERR_PARSER_CANNOT_SHADOW_KW,
    ERR_VM_UNDERFLOW,
    ERR_VM_INVALID_BINARY_OP,
    ERR_PARSER_BYTE_OVERFLOW,
    ERR_PARSER_OUT_OF_BOUNDS,
    ERR_PARSER_NO_ENTRY_POINT,
    ERR_PARSER_TYPE_CONFLICT_EXPR,
    ERR_PARSER_UNKOWN_OPERATOR,
    ERR_PARSER_ATOM_UNKOWN,
    ERR_VM_UNEXPECTED_EXIT,
    ERR_VM_CANNOT_DECODE_CONST_POOL,
    //
    ERR_PARSER_EXPECTED_IDENT,
    ERR_PARSER_UNEXPECTED_STATE,
    ERR_PARSER_UNKOWN_NATIVE,
    ERR_PARSER_STALLED,
    ERR_PARSER_NO_OPEN_TYPE_BRACKET, // <
    ERR_PARSER_NO_CLOSE_TYPE_BRACKET, // >
    ERR_PARSER_MODULE_PREVIOUSLY_LOADED,
    ERR_PARSER_MODULE_CANNOT_OPEN,
    ERR_PARSER_LEXER_FAILURE_CAUGHT,
    ERR_PARSER_UNKOWN_IDENT_STMT,
    ERR_PARSER_UNKOWN_STMT,
    ERR_PARSER_CANNOT_ALLOCATE,
    //
    ERR_AMOUNT
} ErrorCode;

typedef struct {
    ErrorCode ec; // like ERR_ZERO_DIV
    char *name; // like V001
    char *msg; // like Cannot divide by zero
    bool fatal; // if fatal or recoverable
} Error;

typedef struct {
    ErrorCode ec; // like ERR_ZERO_DIV
    int line;
    int column;
} RaisedError;

const Error errorTable[] = {
    {ERR_FILE_OPEN_ERROR,                "F001", "Failed to open file", .fatal=true},
    {ERR_LEX_UNTERMINATED_STR_LIT,       "L001", "Unterminated string literal", .fatal=false},
    {ERR_LEX_INVALID_CHAR,               "L002", "Invalid character", .fatal=false},
    {ERR_LEX_OUT_OF_MEMORY,              "L003", "Lexer out of memory", .fatal=true},
    {ERR_LEX_INVALID_NUM,                "L004", "Invalid number literal", .fatal=false},
    {ERR_PARSER_VAR_NOT_DEFINED,         "P001", "Variable not defined", .fatal=false},
    {ERR_PARSER_VAR_PERVIOUSLY_DEFINED,  "P002", "Variable already defined", .fatal=false},
    {ERR_PARSER_FUNC_NOT_DEFINED,        "P003", "Function not defined", .fatal=false},
    {ERR_PARSER_FUNC_PERVIOUSLY_DEFINED, "P004", "Function already defined", .fatal=false},
    {ERR_LYST_FAILED_LOAD,               "I001", "Failed to load language system table", .fatal=true},
    {ERR_UNKOWN_GLOBAL_FLAG,             "I002", "Unknown global flag", .fatal=true},
    {ERR_MISSING_SOURCE_FILE,            "C001", "Missing source file", .fatal=true},
    {ERR_MISSING_INPUT_FILE,             "C002", "Missing input file", .fatal=true},
    {ERR_RM_COMMAND_UNKOWN,              "C003", "Unknown command", .fatal=true},
    {ERR_UNKNOWN_CLI_ARG,                "C004", "Unknown command-line argument", .fatal=true},
    {ERR_NOT_IMPLEMENTED_YET,            "I003", "Feature not implemented yet", .fatal=true},
    {ERR_NOT_IMPLEMENTED,                "I004", "Operation not implemented", .fatal=true},
    {ERR_PARSER_INTO_ENDOFSTREAM,        "P005", "Unexpected end of token stream", .fatal=true},
    {ERR_PARSER_INTO_STARTOFSTREAM,      "P006", "Unexpected beginning of token stream", .fatal=true},
    {ERR_PARSER_CANNOT_SHADOW_KW,        "P007", "Cannot shadow a keyword", .fatal=false},
    {ERR_VM_UNDERFLOW,                   "V001", "Stack underflow", .fatal=true},
    {ERR_VM_INVALID_BINARY_OP,           "V002", "Invalid binary operation", .fatal=true},
    {ERR_PARSER_BYTE_OVERFLOW,           "P008", "Byte value overflow", .fatal=true},
    {ERR_PARSER_OUT_OF_BOUNDS,           "P009", "Parser index out of bounds", .fatal=true},
    {ERR_PARSER_NO_ENTRY_POINT,          "P010", "No entry point found", .fatal=true},
    {ERR_PARSER_TYPE_CONFLICT_EXPR,      "P011", "Type mismatch in expression", .fatal=false},
    {ERR_PARSER_UNKOWN_OPERATOR,         "P012", "Unknown operator", .fatal=false},
    {ERR_PARSER_ATOM_UNKOWN,             "P013", "Unknown expression", .fatal=false},
    {ERR_VM_UNEXPECTED_EXIT,             "V003", "Program exited unexpectedly", .fatal=true},
    {ERR_VM_CANNOT_DECODE_CONST_POOL,    "V004", "Failed to decode constant pool", .fatal=true},
};

#endif