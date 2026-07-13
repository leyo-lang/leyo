#include "../include/type.h"
#include "../include/errors.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

const char OPERATORS[] = "+-*/^";
const char CONDITIONS[] = "><!";
const char *VARDEFS[] = {"int", "char", "bool", "string"};


static inline bool isSkippable(char c) {
    return isspace(c);
}


bool charIn(char c, const char *toCheck) {
    for (int i = 0; toCheck[i] != '\0'; i++) {
        if (c == toCheck[i]) { return true; }
    };

    return false;
}

bool strIn(const char *c, const char **toCheck) {
    for (int i = 0; toCheck[i] != NULL; i++) {
        if (strcmp(c, toCheck[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool isAlpha(char src) {
    return (toupper(src) != tolower(src));
}

bool isInt(char src) {
    return isdigit(src);
}

Token token(const char *value, TokenType type, int line, int collumn) {
    Token t;
    t.value = strdup(value);
    t.type = type;
    t.line = line;
    t.collumn = collumn;
    return t;
}

TokenStream tokenise(char* src) {
    Token tokens[4096];
    int tokenCount = 0;

    char ident[256];
    int identCount = 0;

    bool commented = false;
    bool stringMode = false;

    int lineNum = 1;
    int collumn = 0;

    for (int i = 0; src[i] != '\0'; i++) {
        collumn++;

        if (src[i] == '\n') {lineNum++; collumn = 0; continue; commented = false;};

        if (commented) {
            continue;
        }

        if (src[i] == '~') {
            commented = true;
        };
    
        if (isSkippable(src[i])) {
            continue;
        }

        if (src[i] == '(') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, OPENBRAC, lineNum, collumn);
        } else if (src[i] == ')') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, CLOSEBRAC, lineNum, collumn);
        } else if (src[i] == ',') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, COMMA, lineNum, collumn);
        } else if (charIn(src[i], OPERATORS)) {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, OPERATION, lineNum, collumn);
        } else if (charIn(src[i], CONDITIONS)) {
            if (src[i+1] == '=') {
                i++;
                char tmp[3] = { src[i-1], src[i], '\0' }; 
                tokens[tokenCount++] = token(tmp, CONDITION, lineNum, collumn);
                continue;
            }
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, OPERATION, lineNum, collumn);
        } else if (src[i] == '=') {
            if (src[i+1] == '=') {
                i++;
                if (src[i+1] == '=') {
                    i++;
                    char tmp[4] = { src[i-2], src[i-1], src[i], '\0' }; 
                    tokens[tokenCount++] = token(tmp, CONDITION, lineNum, collumn);
                    continue;
                }
                char tmp[3] = { src[i-1], src[i], '\0' }; 
                tokens[tokenCount++] = token(tmp, CONDITION, lineNum, collumn);
                continue;
            }
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, EQUALS, lineNum, collumn);
        } else if (src[i] == ';') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, SEMICOLON, lineNum, collumn);
        } else if (src[i] == '[') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, OPENSQUARE, lineNum, collumn);
        } else if (src[i] == ']') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, CLOSESQUARE, lineNum, collumn);
        } else if (src[i] == '{') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, OPENBRACE, lineNum, collumn);
        } else if (src[i] == '}') {
            char tmp[2] = { src[i], '\0' }; 
            tokens[tokenCount++] = token(tmp, CLOSEBRACE, lineNum, collumn);
        } else {
            if (isInt(src[i])) {
                identCount = 0;
                memset(ident, 0, sizeof(ident));
                for (; src[i] != '\0' && isInt(src[i]); i++) {
                    ident[identCount++] = src[i];
                    if (src[i+1] != '\0' && isAlpha(src[i+1])) {
                        continue;
                    } else {
                        break;
                    }
                };
                tokens[tokenCount++] = token(ident, NUMBER, lineNum, collumn);
                ident[identCount] = '\0';
            } else if (isAlpha(src[i])) {
                identCount = 0;
                memset(ident, 0, sizeof(ident));
                for (; src[i] != '\0' && isAlpha(src[i]); i++) {
                    ident[identCount++] = src[i];
                    if (src[i+1] != '\0' && isAlpha(src[i+1])) {
                        continue;
                    } else {
                        break;
                    }
                };
                ident[identCount] = '\0';

                tokens[tokenCount++] = token(ident, IDENTIFIER, lineNum, collumn);
            } else if (src[i] == '"') {
                identCount = 0;
                memset(ident, 0, sizeof(ident));
                for (; src[i] != '\0' && src[i] != '"'; i++) {
                    if (src[i] == '\n') {
                        lraise("Unterminated String Literal", lineNum, collumn);
                        break;
                    }
                    ident[identCount++] = src[i];
                    if (src[i+1] != '\0' && src[i] != '"') {
                        continue;
                    } else {
                        ident[identCount++] = src[i+1];
                        break;
                    }
                };
                ident[identCount] = '\0';
                tokens[tokenCount++] = token(ident, STRING, lineNum, collumn);
            } else {
                char tmp[2] = { src[i], '\0' }; 
                tokens[tokenCount++] = token(tmp, UNKNOWN, lineNum, collumn);
                char buffer[48];
                snprintf(buffer, sizeof(buffer), "Invalid Character: %c", src[i]);
                lraise(buffer, lineNum, collumn);
            }
        }
        
    }


    TokenStream tokenstream;
    memcpy(tokenstream.stream, tokens, tokenCount * sizeof(Token));
    tokenstream.count = tokenCount;
    return tokenstream;
}