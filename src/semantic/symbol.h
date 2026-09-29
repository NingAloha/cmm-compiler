#ifndef SEMANTIC_SYMBOL_H
#define SEMANTIC_SYMBOL_H

#include "type.h"

typedef enum SymbolKind {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_STRUCT
} SymbolKind;

typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;

struct Symbol {
    char *name;
    SymbolKind kind;
    Type *type;
    int line;
    Symbol *next;
};

struct SymbolTable {
    Symbol *head;
};

void symbol_table_init(SymbolTable *table);
void symbol_table_clear(SymbolTable *table);

const Symbol *symbol_table_find(const SymbolTable *table, const char *name);

int symbol_table_insert(SymbolTable *table, const char *name, SymbolKind kind,
                        Type *type, int line);

#endif
