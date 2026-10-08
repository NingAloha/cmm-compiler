#include "util.h"

#include <stdlib.h>
#include <string.h>

char *string_duplicate(const char *source) {
    if (source == NULL) {
        return NULL;
    }

    size_t size = strlen(source) + 1;
    char *copy = malloc(size);

    if (copy != NULL) {
        memcpy(copy, source, size);
    }

    return copy;
}
