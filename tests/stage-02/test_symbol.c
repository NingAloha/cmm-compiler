#include <assert.h>
#include <stdio.h>

#include "../../src/semantic/symbol.h"

int main(void) {
    SymbolTable table;

    symbol_table_init(&table);
    assert(table.head == NULL);

    assert(symbol_table_insert(&table,
        "count",
        SYMBOL_VARIABLE,
        type_int(),
        3));

    const Symbol *count = symbol_table_find(&table, "count");
    assert(count != NULL);
    assert(count->kind == SYMBOL_VARIABLE);
    assert(count->type == type_int());
    assert(count->line == 3);

    assert(!symbol_table_insert(&table,
        "count",
        SYMBOL_VARIABLE,
        type_float(),
        8));

    count = symbol_table_find(&table, "count");
    assert(count != NULL);
    assert(count->type == type_int());
    assert(count->line == 3);

    Type *average_type = type_new_function(type_float(), NULL);
    assert(average_type != NULL);

    assert(symbol_table_insert(&table,
        "average",
        SYMBOL_FUNCTION,
        average_type,
        12));

    const Symbol *average = symbol_table_find(&table, "average");
    assert(average != NULL);
    assert(average->kind == SYMBOL_FUNCTION);
    assert(average->type == average_type);

    Type *point_type = type_new_structure("Point", NULL);
    assert(point_type != NULL);

    assert(symbol_table_insert(&table,
                               "Point",
                               SYMBOL_STRUCT,
                               point_type,
                               20));

    const Symbol *point = symbol_table_find(&table, "Point");
    assert(point != NULL);
    assert(point->kind == SYMBOL_STRUCT);
    assert(point->type == point_type);

    assert(symbol_table_find(&table, "missing") == NULL);

    symbol_table_clear(&table);
    assert(table.head == NULL);
    assert(symbol_table_find(&table, "count") == NULL);

    symbol_table_clear(&table);

    puts("symbol table tests passed");
    return 0;
}
