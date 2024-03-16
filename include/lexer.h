#ifndef LEXER_H
#define LEXER_H
#include "token.h"

ParseSource tokenize(char *source, size_t source_size);
#endif
