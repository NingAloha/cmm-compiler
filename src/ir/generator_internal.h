#ifndef GENERATOR_INTERNAL_H
#define GENERATOR_INTERNAL_H

/* Private interface shared by the IR generator translation units. */

#include "generator.h"

#include <stddef.h>

#define OPERAND_CAPACITY 128

typedef struct {
  const TreeNode *id;
  size_t *dimensions;
  size_t dimension_count;
} ArrayShape;

struct IrVariable {
  char *source_name;
  char ir_name[OPERAND_CAPACITY];
  size_t *dimensions;
  size_t dimension_count;
  int is_array_parameter;
  struct IrVariable *next;
};

/* generator.c: state, variables and error handling */

int generator_fail(IrGenerator *generator);
int set_operand(IrGenerator *generator, char *result, size_t result_size,
                const char *value);
const IrVariable *variable_find(const IrGenerator *generator,
                                const char *source_name);
int variable_declare(IrGenerator *generator, const char *source_name,
                     const ArrayShape *shape, int is_array_parameter,
                     char *result, size_t result_size);

/* gen_array.c: array shapes and element addressing */

void array_shape_clear(ArrayShape *shape);
int parse_var_dec(const TreeNode *node, ArrayShape *shape);
int array_total_bytes(const ArrayShape *shape, size_t *result);
int translate_array_address(IrGenerator *generator, const TreeNode *node,
                            char *result, size_t result_size,
                            size_t *remaining_dimensions);

/* gen_expr.c: expressions and conditions */

int translate_exp(IrGenerator *generator, const TreeNode *node, char *result,
                  size_t result_size);
int translate_cond(IrGenerator *generator, const TreeNode *node,
                   const char *true_label, const char *false_label);

/* gen_stmt.c: statements and compound blocks */

int translate_comp_st(IrGenerator *generator, const TreeNode *node);

/* gen_decl.c: declarations, parameters and functions */

int translate_def_list(IrGenerator *generator, const TreeNode *node);
int translate_ext_def_list(IrGenerator *generator, const TreeNode *node);

#endif
