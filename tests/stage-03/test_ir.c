#include "../../src/ir/ir.h"
#include "../test_util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_emit_and_naming(void) {
  FILE *output = tmpfile();
  assert(output != NULL);

  IrContext context;
  ir_context_init(&context, output);

  char too_small[2];
  assert(!ir_new_temp(&context, too_small, sizeof(too_small)));

  char temp[16];
  char label[16];

  assert(ir_new_temp(&context, temp, sizeof(temp)));
  assert(strcmp(temp, "t1") == 0);

  assert(ir_new_label(&context, label, sizeof(label)));
  assert(strcmp(label, "label1") == 0);

  assert(ir_emit(&context, "FUNCTION main :"));
  assert(ir_emit(&context, "%s := #42", temp));
  assert(ir_emit(&context, "WRITE %s", temp));
  assert(ir_emit(&context, "RETURN #0"));

  rewind(output);

  char actual[128] = {0};
  size_t length = fread(actual, 1, sizeof(actual) - 1, output);

  assert(length > 0);
  assert(strcmp(actual, "FUNCTION main :\n"
                        "t1 := #42\n"
                        "WRITE t1\n"
                        "RETURN #0\n") == 0);

  fclose(output);
}

static void test_short_label_buffer_preserves_counter(void) {
  FILE *output = tmpfile();
  assert(output != NULL);

  IrContext context;
  ir_context_init(&context, output);

  char too_small[sizeof("label1") - 1];
  char exact[sizeof("label1")];
  assert(!ir_new_label(&context, too_small, sizeof(too_small)));
  assert(ir_new_label(&context, exact, sizeof(exact)));
  assert(strcmp(exact, "label1") == 0);
  assert(fclose(output) == 0);
}

static void test_missing_output_rejects_emission(void) {
  IrContext context;
  ir_context_init(&context, NULL);

  assert(!ir_emit(&context, "RETURN #0"));
}

static void test_name_counters_advance_independently(void) {
  FILE *output = tmpfile();
  assert(output != NULL);

  IrContext context;
  ir_context_init(&context, output);

  char name[16];
  assert(ir_new_temp(&context, name, sizeof(name)));
  assert(strcmp(name, "t1") == 0);
  assert(ir_new_temp(&context, name, sizeof(name)));
  assert(strcmp(name, "t2") == 0);
  assert(ir_new_label(&context, name, sizeof(name)));
  assert(strcmp(name, "label1") == 0);
  assert(ir_new_label(&context, name, sizeof(name)));
  assert(strcmp(name, "label2") == 0);
  assert(ir_new_temp(&context, name, sizeof(name)));
  assert(strcmp(name, "t3") == 0);
  assert(fclose(output) == 0);
}

int main(void) {
  RUN_TEST(test_emit_and_naming);
  RUN_TEST(test_short_label_buffer_preserves_counter);
  RUN_TEST(test_missing_output_rejects_emission);
  RUN_TEST(test_name_counters_advance_independently);

  TEST_SUMMARY("stage-03/ir");
}
