#include "../../src/ir/ir.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
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
  return 0;
}
