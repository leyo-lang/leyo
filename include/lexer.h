#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

#include "../include/type.h"


bool charIn(char c, const char *toCheck);

bool strIn(const char *c, const char **toCheck);

bool isAlpha(char src);

bool isInt(char src);


Token _token(const char *value, TokenType type, int line, int column);
Token token(const char *value, TokenType type);

TokenStream tokenise(char *src);

#endif
