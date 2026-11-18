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

/* Initialize an unused table. Clear frees its entries and resets it to depth 0.
 */

void symbol_table_init(SymbolTable *table);
void symbol_table_clear(SymbolTable *table);

/* Find an exact name in the current scope or, with find(), the nearest
 * visible scope. NULL means no match. Results are borrowed until the entry
 * is removed by leaving its scope or clearing the table. */

const Symbol *symbol_table_find_current(const SymbolTable *table,
                                        const char *name);
const Symbol *symbol_table_find(const SymbolTable *table, const char *name);

/* Copy name and borrow type. Return 1 on success, or 0 on invalid input,
 * a duplicate name at this depth (regardless of kind), or allocation failure.
 */

int symbol_table_insert(SymbolTable *table, const char *name, SymbolKind kind,
                        Type *type, int line);

/* Leaving removes all entries at the current depth; at depth 0 it is a no-op.
 */

void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_leave_scope(SymbolTable *table);

#endif
