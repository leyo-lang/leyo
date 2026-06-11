#include "../include/type.h"
#include "../include/errors.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char OPERATORS[] = "+-*/^";
const char CONDITIONS[] = "><!";

char *src;

bool isAlpha(char src) {
    return (toupper(src) != tolower(src));
}

bool isInt(char src) {
    return isdigit(src);
}

char *charToStr(char c) {
    static char tmp[2];
    tmp[0] = c;
    tmp[1] = '\0';
    return tmp;
}

bool charIn(char c, const char *toCheck) {
    for (int i = 0; toCheck[i] != '\0'; i++) {
        if (c == toCheck[i]) { return true; }
    };
    return false;
}

typedef enum {
    M_NORMAL,
    M_STRING,
    M_NUMBER,
    M_IDENTIFIER,
    M_COMMENT
} LexerMode;

typedef struct {
    int i;
    int line;
    int collumn;
    int scol;
    LexerMode mode;
} Lexer;

Lexer lexer;
Lexer *l = &lexer;

TokenStream lexRes = {0};

Token _token(const char *value, TokenType type, int line, int collumn) {
    Token t;
    t.value = strdup(value);
    t.type = type;
    t.line = line;
    t.collumn = collumn;
    return t;
}

Token token(const char *value, TokenType type) {
    return _token(value, type, l->line, l->scol);
}

static void push(Token token) {
    logBuildLexer("Token pushed to stream");
    l->scol = l->collumn;
    lexRes.stream[lexRes.count++] = token;
}

static char peek(void) {
    return src[l->i+1];
}

static char current(void) {
    return src[l->i];
}

static char previous(void) {
    return src[l->i-1];
}

static void advance(void) {
    l->i++;
    if (current() == '\n') {
        l->line++;
        l->collumn = 0;
        l->scol = 0;
    } else {
        l->collumn++;
    }
}

void handleNormal(void) {
    char c = current();

    logBuildLexer("Entering NORMAL mode");

    if (isspace(c) || c == '\n') { advance(); return; }

    if (c == '~') {
        l->mode = M_COMMENT;
        logBuildLexer("Switching to COMMENT mode");
        advance();
        return;
    } 
    else if (c == '"') {
        l->mode = M_STRING;
        logBuildLexer("Switching to STRING mode");
        advance();
        return;
    } 
    else if (c == '\'') {
        logBuildLexer("Handling Char");
        advance();
        push(token(charToStr(current()), CHR));
        advance();
        advance();
        return;
    } 
    else if (isalpha(c)) {
        l->mode = M_IDENTIFIER;
        logBuildLexer("Switching to IDENTIFIER mode");
        return;
    } 
    else if (isdigit(c)) {
        l->mode = M_NUMBER;
        logBuildLexer("Switching to NUMBER mode");
        return;
    };

    if (c == '(') { push(token(charToStr(c), OPENBRAC)); } else
    if (c == ')') { push(token(charToStr(c), CLOSEBRAC)); } else
    if (c == ',') { push(token(charToStr(c), COMMA)); } else 
    if (c == ';') { push(token(charToStr(c), SEMICOLON)); } else
    if (c == '[') { push(token(charToStr(c), OPENSQUARE)); } else
    if (c == ']') { push(token(charToStr(c), CLOSESQUARE)); } else
    if (c == '{') { push(token(charToStr(c), OPENBRACE)); } else
    if (c == '}') { push(token(charToStr(c), CLOSEBRACE)); } else
    if (c == '=') {
        if (peek() == '=') {
            advance();
            if (peek() == '=') {
                advance();
                push(token("===", CONDITION));
            } else {
                push(token("==", CONDITION));
            }
        } else {
            push(token(charToStr(c), EQUALS));
        }
    } 
    else if (charIn(c, OPERATORS)) {
        push(token(charToStr(c), OPERATION));
    } 
    else if (charIn(c, CONDITIONS)) {
        if (peek() == '=') {
            advance();
            char tmp[3] = { previous(), current(), '\0'};
            push(token(tmp, CONDITION));
        }
        push(token(charToStr(c), CONDITION));
    } 
    else {
        char buffer[48];
        snprintf(buffer, sizeof(buffer), "Invalid Character: %c", current());
        logBuildLexer(buffer);
        raise(buffer, l->line, l->collumn);
        push(token(charToStr(c), UNKNOWN));
    }

    advance();
}

void handleString(void) {
    logBuildLexer("Entering STRING mode");

    int buffSize = 0;
    int buffCap = 32;
    char *buff = malloc(buffCap);

    if (!buff) {
        logBuildLexer("String buffer allocation failed");
        raise("Out of memory", l->line, l->collumn);
        return;
    }

    while (true) {
        if (current() == '\n' || current() == '\0') {
            logBuildLexer("Unterminated string detected");
            raise("Unterminated String Literal", l->line, l->collumn);
            free(buff);
            l->mode = M_NORMAL;
            return;
        }

        if (current() == '"') {
            buff[buffSize] = '\0';
            push(token(buff, STRING));
            l->mode = M_NORMAL;
            break;
        }

        if (buffSize >= buffCap - 1) {
            buffCap *= 2;
            char *tmp = realloc(buff, buffCap);

            if (!tmp) {
                logBuildLexer("String realloc failed");
                free(buff);
                raise("Out of memory", l->line, l->collumn);
                return;
            }
            buff = tmp;
        }

        buff[buffSize++] = current();
        advance();
    }

    advance();
    free(buff);
}

void handleIdentifier(void) {
    logBuildLexer("Entering IDENTIFIER mode");

    int buffSize = 0;
    int buffCap = 32;
    char *buff = malloc(buffCap);

    if (!buff) {
        logBuildLexer("Identifier allocation failed");
        raise("Out of memory", l->line, l->collumn);
        return;
    }

    while (true) {
        char c = current();

        if (!isalnum(c) && c != '_') {
            buff[buffSize] = '\0';
            push(token(buff, IDENTIFIER));
            l->mode = M_NORMAL;
            break;
        }

        if (buffSize >= buffCap - 1) {
            buffCap *= 2;
            char *tmp = realloc(buff, buffCap);

            if (!tmp) {
                logBuildLexer("Identifier realloc failed");
                free(buff);
                raise("Out of memory", l->line, l->collumn);
                return;
            }
            buff = tmp;
        }

        buff[buffSize++] = c;
        advance();
    }

    //advance();
    free(buff);
}

void handleNumber(void) {
    logBuildLexer("Entering NUMBER mode");

    int buffSize = 0;
    int buffCap = 32;
    char *buff = malloc(buffCap);

    if (!buff) {
        logBuildLexer("Number allocation failed");
        raise("Out of memory", l->line, l->collumn);
        return;
    }

    bool dotSeen = false;
    TokenType flag = NUMBER;

    while (true) {
        char c = current();

        if (!isdigit(c) && c != '.') {
            buff[buffSize] = '\0';
            push(token(buff, flag));
            l->mode = M_NORMAL;
            break;
        }

        if (c == '.') {
            if (dotSeen) {
                logBuildLexer("Invalid number format (multiple dots)");
                raise("Invalid number format", l->line, l->collumn);
                break;
            }
            flag = FLT;
            dotSeen = true;
        }

        if (buffSize >= buffCap - 1) {
            buffCap *= 2;
            char *tmp = realloc(buff, buffCap);

            if (!tmp) {
                logBuildLexer("Number realloc failed");
                free(buff);
                raise("Out of memory", l->line, l->collumn);
                return;
            }
            buff = tmp;
        }

        buff[buffSize++] = c;
        advance();
    }

    free(buff);
}

void handleComment(void) {
    logBuildLexer("Entering COMMENT mode");

    while (true) {
        if (current() == '~') {
            l->mode = M_NORMAL;
            advance();
            break;
        } else {
            advance();
        }
    }
}

TokenStream tokenise(char* _src) {
    src = _src;

    l->i = 0;
    l->collumn = 1;
    l->scol = 1;
    l->line = 1;
    l->mode = M_NORMAL;

    logBuildLexer("Lexer started");

    while (current() != '\0') {
        switch (l->mode) {
            case M_NORMAL: handleNormal(); break;
            case M_STRING: handleString(); break;
            case M_IDENTIFIER: handleIdentifier(); break;
            case M_NUMBER: handleNumber(); break;
            case M_COMMENT: handleComment(); break;
        }
    }

    push(token("EndOfStream", ENDOFSTREAM));
    logBuildLexer("Lexer finished");
    return lexRes;
}