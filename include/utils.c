#include "utils.h"
#include <stdlib.h>

char *substring(char *string, int start, int end) {
    if (start == end) {
        return NULL;
    }
    int size = end - start + 1;
    char *result = malloc((size) * sizeof(char));
    for (int i = start; i < end; i++) {
        result[i - start] = string[i];
    }
    result[size - 1] = '\0';
    return result;
}

int min(int a, int b) {
    if (a > b) {
        return b;
    }
    return a;
}
