#include <assert.h>
#include <stdio.h>

#include "../../src/semantic/type.h"

static void test_shared_types(void) {
  assert(type_int() == type_int());
  assert(type_float() == type_float());

  assert(type_equal(type_int(), type_int()));
  assert(type_equal(type_float(), type_float()));
  assert(!type_equal(type_int(), type_float()));

  assert(type_is_numeric(type_int()));
  assert(type_is_numeric(type_float()));
}

static void test_array_type_equality(void) {
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
}

static void test_structure_identity(void) {
  Type *point = type_new_structure("Point", NULL);
  Type *vector = type_new_structure("Vector", NULL);

  assert(point != NULL);
  assert(vector != NULL);
  assert(type_equal(point, point));
  assert(!type_equal(point, vector));
}

static void test_field_operations(void) {
  Field *left = field_new("left", type_int(), 1);
  Field *right = field_new("right", type_float(), 2);

  assert(left != NULL);
  assert(right != NULL);

  field_append(&left, right);

  assert(field_find(left, "left") == left);
  assert(field_find(left, "right") == right);
  assert(field_find(left, "missing") == NULL);
}

int main(void) {
  test_shared_types();
  test_array_type_equality();
  test_structure_identity();
  test_field_operations();

  puts("stage-02/type: passed");
  return 0;
}
