#include <assert.h>
#include <stdio.h>

#include "../../src/semantic/type.h"

int main(void) {
    assert(type_int() == type_int());
    assert(type_float() == type_float());

    assert(type_equal(type_int(), type_int()));
    assert(type_equal(type_float(), type_float()));
    assert(!type_equal(type_int(), type_float()));

    assert(type_is_numeric(type_int()));
    assert(type_is_numeric(type_float()));

    Type *int_array_10 = type_new_array(type_int(), 10);
    Type *int_array_5 = type_new_array(type_int(), 5);

    assert(int_array_10 != NULL);
    assert(int_array_5 != NULL);
    assert(type_equal(int_array_10, int_array_5));

    Type *int_matrix_10_2 = type_new_array(int_array_10, 2);
    Type *int_matrix_5_3 = type_new_array(int_array_5, 3);

    assert(int_matrix_10_2 != NULL);
    assert(int_matrix_5_3 != NULL);
    assert(type_equal(int_matrix_10_2, int_matrix_5_3));
    assert(!type_equal(int_array_10, int_matrix_10_2));

    Type *point = type_new_structure("Point", NULL);
    Type *vector = type_new_structure("Vector", NULL);

    assert(point != NULL);
    assert(vector != NULL);
    assert(type_equal(point, point));
    assert(!type_equal(point, vector));

    puts("type tests passed");
    return 0;
}