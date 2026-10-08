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
    int scope_depth;
    Symbol *next;
};

struct SymbolTable {
    Symbol *head;
    int scope_depth;
};

/* Lifetime */

void symbol_table_init(SymbolTable *table);
void symbol_table_clear(SymbolTable *table);

/* Lookup */

const Symbol *symbol_table_find_current(const SymbolTable *table,
                                        const char *name);
const Symbol *symbol_table_find(const SymbolTable *table, const char *name);

/* Insertion */

int symbol_table_insert(SymbolTable *table, const char *name, SymbolKind kind,
                        Type *type, int line);

/* Scope management */

void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_leave_scope(SymbolTable *table);

#endif
