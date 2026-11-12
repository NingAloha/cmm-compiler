#include "generator_internal.h"

#include <stdlib.h>
#include <string.h>

int generator_fail(IrGenerator *generator) {
  if (generator != NULL) {
    generator->failed = 1;
  }
  return 0;
}

int set_operand(IrGenerator *generator, char *result, size_t result_size,
                const char *value) {
  if (generator == NULL || result == NULL || result_size == 0 ||
      value == NULL) {
    return generator_fail(generator);
  }

  int written = snprintf(result, result_size, "%s", value);
  if (written < 0 || (size_t)written >= result_size) {
    return generator_fail(generator);
  }
  return 1;
}

const IrVariable *variable_find(const IrGenerator *generator,
                                const char *source_name) {
  if (generator == NULL || source_name == NULL) {
    return NULL;
  }

  for (const IrVariable *variable = generator->variables; variable != NULL;
       variable = variable->next) {
    if (strcmp(variable->source_name, source_name) == 0) {
      return variable;
    }
  }
  return NULL;
}

int variable_declare(IrGenerator *generator, const char *source_name,
                     const ArrayShape *shape, int is_array_parameter,
                     char *result, size_t result_size) {
  if (generator == NULL || source_name == NULL || shape == NULL) {
    return generator_fail(generator);
  }

  IrVariable *variable = calloc(1, sizeof(*variable));
  if (variable == NULL) {
    return generator_fail(generator);
  }

  size_t name_length = strlen(source_name);
  variable->source_name = malloc(name_length + 1);
  if (variable->source_name == NULL) {
    free(variable);
    return generator_fail(generator);
  }
  memcpy(variable->source_name, source_name, name_length + 1);

  if (shape->dimension_count > 0) {
    variable->dimensions =
        malloc(shape->dimension_count * sizeof(*variable->dimensions));
    if (variable->dimensions == NULL) {
      free(variable->source_name);
      free(variable);
      return generator_fail(generator);
    }
    memcpy(variable->dimensions, shape->dimensions,
           shape->dimension_count * sizeof(*variable->dimensions));
    variable->dimension_count = shape->dimension_count;
  }
  variable->is_array_parameter = is_array_parameter;

  int written = snprintf(variable->ir_name, sizeof(variable->ir_name), "v%u",
                         generator->next_variable);
  if (written < 0 || (size_t)written >= sizeof(variable->ir_name) ||
      !set_operand(generator, result, result_size, variable->ir_name)) {
    free(variable->source_name);
    free(variable->dimensions);
    free(variable);
    return generator_fail(generator);
  }

  generator->next_variable++;
  variable->next = generator->variables;
  generator->variables = variable;
  return 1;
}

void ir_generator_init(IrGenerator *generator, FILE *output) {
  if (generator == NULL) {
    return;
  }

  ir_context_init(&generator->ir, output);
  generator->variables = NULL;
  generator->next_variable = 1;
  generator->failed = generator->ir.failed;
}

void ir_generator_clear(IrGenerator *generator) {
  if (generator == NULL) {
    return;
  }

  IrVariable *variable = generator->variables;
  while (variable != NULL) {
    IrVariable *next = variable->next;
    free(variable->source_name);
    free(variable->dimensions);
    free(variable);
    variable = next;
  }
  generator->variables = NULL;
}

int ir_generate_program(IrGenerator *generator, const TreeNode *root) {
  if (generator == NULL || root == NULL || generator->failed ||
      generator->ir.failed || !tree_is(root, "Program") ||
      !translate_ext_def_list(generator, tree_child_at(root, 0)) ||
      generator->failed || generator->ir.failed) {
    return generator_fail(generator);
  }
  return 1;
}
