#ifndef TYPE_H
#define TYPE_H
#define MAX_TOKENS_PER_LINE 256

typedef enum {
    NONE,
    NUMBER,
    IDENTIFIER,
    EQUALS,
    OPENBRAC,
    CLOSEBRAC,
    OPERATION,
    SEMICOLON,
    OPENBRACE,
    CLOSEBRACE,
    OPENSQUARE,
    CLOSESQUARE,
    CONDITION,
    COMMA,
    STRING,
    ENDOFSTREAM,
    UNKNOWN,
    CHR,
    FLT,
    NATIVE
} TokenType;

typedef struct {
    TokenType type;
    char *value;

    int line;
    int collumn;
} Token;

typedef struct {
    Token *stream;
    int capacity;
    int count;
} TokenStream;



#endif