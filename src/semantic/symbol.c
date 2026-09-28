#include "symbol.h"

#include <stdlib.h>
#include <string.h>

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

void symbol_table_init(SymbolTable *table) {
    if (table != NULL) {
        table->head = NULL;
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
}

const Symbol *symbol_table_find(const SymbolTable *table, const char *name) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    const Symbol *current = table->head;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

int symbol_table_insert(SymbolTable *table, const char *name, SymbolKind kind, Type *type, int line) {
    if (table == NULL || name == NULL || type == NULL) {
        return 0;
    }

    if (symbol_table_find(table, name) != NULL) {
        return 0;
    }

    Symbol *new_symbol = malloc(sizeof(*new_symbol));
    if (new_symbol == NULL) {
        return 0;
    }

    new_symbol->name = copy_string(name);
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