#include <stdbool.h>
#include "../include/codes.h"


const Error errorTable[] = {
    // File
    {ERR_FILE_OPEN_ERROR,                "F001", "Failed to open file", .fatal=true},
    {ERR_FILE_TOO_SMALL,                 "F002", "File is too small to be valid", .fatal=true},
    {ERR_FILE_CANNOT_DEL,                "F003", "File could not be archived or deleted", .fatal=false},
    {ERR_FILE_OPEN_WARN,                 "F004", "Failed to open file", .fatal=false},    
    {ERR_FILE_WRITE_ERROR,               "F005", "Failed to write to file", .fatal=false},
    {ERR_FILE_TEMP_NOT_AVAILABLE,        "F006", "Could not create tempory files/folders", .fatal=false},
    
    // CLI
    {ERR_MISSING_SOURCE_FILE,            "C001", "Missing source file", .fatal=true},
    {ERR_MISSING_INPUT_FILE,             "C002", "Missing input file", .fatal=true},
    {ERR_RM_COMMAND_UNKOWN,              "C003", "Unknown command", .fatal=true},
    {ERR_UNKNOWN_CLI_ARG,                "C004", "Unknown command-line argument", .fatal=true},

    // Lexer
    {ERR_LEX_UNTERMINATED_STR_LIT,       "L001", "Unterminated string literal", .fatal=false},
    {ERR_LEX_INVALID_CHAR,               "L002", "Invalid character", .fatal=false},
    {ERR_LEX_OUT_OF_MEMORY,              "L003", "Lexer out of memory", .fatal=true},
    {ERR_LEX_INVALID_NUM,                "L004", "Invalid number literal", .fatal=false},

    // Parser
    {ERR_PARSER_EXPECTED_IDENT,          "P001", "Expected identifier", .fatal=false},
    {ERR_PARSER_EXPECTED_TOKEN,          "P001", "Expected token", .fatal=false},
    {ERR_PARSER_VAR_NOT_DEFINED,         "P002", "Variable not defined", .fatal=false},
    {ERR_PARSER_VAR_PERVIOUSLY_DEFINED,  "P003", "Variable already defined", .fatal=false},
    {ERR_PARSER_FUNC_NOT_DEFINED,        "P004", "Function not defined", .fatal=false},
    {ERR_PARSER_FUNC_PERVIOUSLY_DEFINED, "P005", "Function already defined", .fatal=false},

    {ERR_PARSER_CANNOT_SHADOW_KW,        "P006", "Cannot shadow a keyword", .fatal=false},
    {ERR_PARSER_TYPE_CONFLICT_EXPR,      "P007", "Type mismatch in expression", .fatal=false},
    {ERR_PARSER_UNKOWN_OPERATOR,         "P008", "Unknown operator", .fatal=false},
    {ERR_PARSER_ATOM_UNKOWN,             "P009", "Unknown expression", .fatal=false},
    {ERR_PARSER_UNKOWN_IDENT_STMT,       "P010", "Unknown identifier statement", .fatal=false},
    {ERR_PARSER_UNKOWN_STMT,             "P011", "Unknown statement", .fatal=false},
    {ERR_PARSER_UNKOWN_NATIVE,           "P012", "Unknown native function", .fatal=false},

    {ERR_PARSER_NO_OPEN_TYPE_BRACKET,    "P013", "Expected '<'", .fatal=false},
    {ERR_PARSER_NO_CLOSE_TYPE_BRACKET,   "P014", "Expected '>'", .fatal=false},

    {ERR_PARSER_MODULE_PREVIOUSLY_LOADED,"P015", "Module already loaded", .fatal=false},
    {ERR_PARSER_MODULE_CANNOT_OPEN,      "P016", "Failed to open module", .fatal=true},
    {ERR_PARSER_LEXER_FAILURE_CAUGHT,    "P017", "Lexer error during parsing", .fatal=true},

    {ERR_PARSER_INTO_ENDOFSTREAM,        "P018", "Unexpected end of token stream", .fatal=true},
    {ERR_PARSER_INTO_STARTOFSTREAM,      "P019", "Unexpected beginning of token stream", .fatal=true},
    {ERR_PARSER_UNEXPECTED_STATE,        "P020", "Unexpected parser state", .fatal=true},
    {ERR_PARSER_STALLED,                 "P021", "Parser stalled", .fatal=true},
    {ERR_PARSER_CANNOT_ALLOCATE,         "P022", "Parser out of memory", .fatal=true},
    {ERR_PARSER_BYTE_OVERFLOW,           "P023", "Byte value overflow", .fatal=true},
    {ERR_PARSER_OUT_OF_BOUNDS,           "P024", "Parser index out of bounds", .fatal=true},
    {ERR_PARSER_NO_ENTRY_POINT,          "P025", "No entry point found", .fatal=true},

    {ERR_PARSER_NO_SEMICOLON,            "P026", "Expected semicolon ';'", .fatal=true},

    // Bytecode
    {ERR_INVALID_BYTECODE_HEADER,        "B001", "Invalid bytecode header", .fatal=true},
    {ERR_INVALID_BYTECODE,               "B002", "Invalid bytecode", .fatal=true},
    {ERR_CANNOT_READ_BYTECODE,           "B003", "Failed to read bytecode", .fatal=true},
    {ERR_WARN_DIFFERERENT_VERSIONS,      "B004", "WARN: Bytecode and VM have differing versions", .fatal=false},

    // Virtual Machine
    {ERR_VM_UNDERFLOW,                   "V001", "Stack underflow", .fatal=true},
    {ERR_VM_INVALID_BINARY_OP,           "V002", "Invalid binary operation", .fatal=true},
    {ERR_VM_CANNOT_DECODE_CONST_POOL,    "V003", "Failed to decode constant pool", .fatal=true},
    {ERR_VM_CANNOT_ALLOCATE,             "V004", "Virtual machine out of memory", .fatal=true},
    {ERR_VM_INVALID_CONST_POOL,          "V005", "Invalid constant pool", .fatal=true},
    {ERR_VM_INVALID_JUMP,                "V006", "Invalid jump target", .fatal=true},
    {ERR_VM_GLOBAL_SLOT_OUT_OF_RANGE,    "V007", "Global slot out of range", .fatal=true},
    {ERR_VM_CONST_OUT_OF_RANGE,          "V008", "Constant index out of range", .fatal=true},
    {ERR_VM_UNKOWN_NATIVE_COMMAND,       "V009", "Unknown native command", .fatal=true},
    {ERR_VM_TEXT_NUM_TYPE_CALC,          "V010", "Cannot perform arithmetic between text and number", .fatal=false},
    {ERR_VM_TEXT_CALC_UNSUPPORTED,       "V011", "Text arithmetic is unsupported", .fatal=false},
    {ERR_VM_UNEXPECTED_EXIT,             "V012", "Program exited unexpectedly", .fatal=true},

    // Internal
    {ERR_LYST_FAILED_LOAD,               "I001", "Failed to load language system table", .fatal=true},
    {ERR_UNKOWN_GLOBAL_FLAG,             "I002", "Unknown global flag", .fatal=true},
    {ERR_NOT_IMPLEMENTED_YET,            "I003", "Feature not implemented yet", .fatal=true},
    {ERR_NOT_IMPLEMENTED,                "I004", "Operation not implemented", .fatal=true},
};

const unsigned int errorTableAmt = sizeof(errorTable) / sizeof(Error);