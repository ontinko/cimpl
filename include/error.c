#include "error.h"
#include <stdio.h>
#include <stdlib.h>

Error *error_create() {
    Error *err = malloc(sizeof(Error));
    err->token = NULL;
    err->message = NULL;
    return err;
}

void error_print(Error *err) {
    switch (err->type) {
    case SyntaxError:
        printf("Syntax Error");
        break;
    case ReferenceError:
        printf("Syntax Error");
        break;
    case TypeError:
        printf("Syntax Error");
        break;
    case ParseError:
        printf("Parse Error");
        break;
    }
    printf(" at %lu:%lu: ", err->token->ln, err->token->start - err->token->ln_start + 1);
    printf("%s\n", err->message);
}
