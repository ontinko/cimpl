#ifndef ERROR_H
#define ERROR_H
#include "token.h"
#include <stdio.h>

typedef enum { ParseError, SyntaxError, TypeError, ReferenceError } ErrorType;

typedef struct {
    ErrorType type;
    char *message;
    Token *token;
} Error;

Error *error_create();

void error_print(Error *err);
#endif
