#include "symbol.h"

#include <stdlib.h>
#include <string.h>

/* Storage helpers */

static char *copy_string(const char *source) {
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

/* Lifetime */

void symbol_table_init(SymbolTable *table) {
    if (table != NULL) {
        table->head = NULL;
        table->scope_depth = 0;
    }
}

void symbol_table_clear(SymbolTable *table) {
    if (table == NULL) {
        return;
    }

    Symbol *current = table->head;

    while (current != NULL) {
        Symbol *next = current->next;
        free(current->name);
        free(current);
        current = next;
    }

    table->head = NULL;
    table->scope_depth = 0;
}

/* Lookup */

const Symbol *symbol_table_find_current(const SymbolTable *table,
                                        const char *name) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    const Symbol *current = table->head;

    while (current != NULL) {
        if (current->scope_depth == table->scope_depth &&
            strcmp(current->name, name) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

const Symbol *symbol_table_find(const SymbolTable *table, const char *name) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    const Symbol *current = table->head;
    const Symbol *result = NULL;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            result = current;
        }

        current = current->next;
    }

    return result;
}

/* Insertion */

int symbol_table_insert(SymbolTable *table, const char *name, SymbolKind kind,
                        Type *type, int line) {
    if (table == NULL || name == NULL || type == NULL) {
        return 0;
    }

    if (symbol_table_find_current(table, name) != NULL) {
        return 0;
    }

    Symbol *new_symbol = malloc(sizeof(*new_symbol));
    if (new_symbol == NULL) {
        return 0;
    }

    new_symbol->name = copy_string(name);
    new_symbol->scope_depth = table->scope_depth;
    if (new_symbol->name == NULL) {
        free(new_symbol);
        return 0;
    }

    new_symbol->kind = kind;
    new_symbol->type = type;
    new_symbol->line = line;
    new_symbol->next = NULL;

    if (table->head == NULL) {
        table->head = new_symbol;
        return 1;
    }

    Symbol *tail = table->head;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = new_symbol;
    return 1;
}

/* Scope management */

void symbol_table_enter_scope(SymbolTable *table) {
    if (table != NULL) {
        table->scope_depth++;
    }
}

void symbol_table_leave_scope(SymbolTable *table) {
    if (table != NULL && table->scope_depth > 0) {
        int leaving_depth = table->scope_depth;
        Symbol *previous = NULL;
        Symbol *current = table->head;

        while (current != NULL) {
            if (current->scope_depth == leaving_depth) {
                Symbol *next = current->next;

                if (previous == NULL) {
                    table->head = next;
                } else {
                    previous->next = next;
                }

                free(current->name);
                free(current);
                current = next;
            } else {
                previous = current;
                current = current->next;
            }
        }

        table->scope_depth--;
    }
}
