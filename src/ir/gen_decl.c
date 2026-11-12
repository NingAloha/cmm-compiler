#include "generator_internal.h"

#include <string.h>

static int int_specifier(const TreeNode *node) {
  const TreeNode *type = tree_child_at(node, 0);
  return tree_is(node, "Specifier") && tree_is(type, "TYPE") &&
         type->text != NULL && strcmp(type->text, "int") == 0;
}

static int translate_dec(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *var_dec = tree_child_at(node, 0);
  const TreeNode *assign = tree_child_at(node, 1);
  const TreeNode *initializer = tree_child_at(node, 2);
  ArrayShape shape = {0};
  char variable[OPERAND_CAPACITY];
  int succeeded = 0;

  if (!tree_is(node, "Dec") || !tree_is(var_dec, "VarDec") ||
      !parse_var_dec(var_dec, &shape) || shape.id == NULL ||
      !variable_declare(generator, shape.id->text, &shape, 0, variable,
                        sizeof(variable))) {
    goto done;
  }

  if (shape.dimension_count == 0) {
    if (assign == NULL && initializer == NULL) {
      succeeded = 1;
      goto done;
    }
    char value[OPERAND_CAPACITY];
    if (!tree_is(assign, "ASSIGNOP") || !tree_is(initializer, "Exp") ||
        !translate_exp(generator, initializer, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := %s", variable, value)) {
      goto done;
    }
    succeeded = 1;
    goto done;
  }

  size_t bytes = 0;
  if (assign != NULL || initializer != NULL ||
      !array_total_bytes(&shape, &bytes) ||
      !ir_emit(&generator->ir, "DEC %s %zu", variable, bytes)) {
    goto done;
  }
  succeeded = 1;

done:
  array_shape_clear(&shape);
  return succeeded ? 1 : generator_fail(generator);
}

static int translate_dec_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!tree_is(node, "DecList") ||
      !translate_dec(generator, tree_child_at(node, 0))) {
    return generator_fail(generator);
  }
  if (tree_child_at(node, 1) == NULL && tree_child_at(node, 2) == NULL) {
    return 1;
  }
  return tree_is(tree_child_at(node, 1), "COMMA") &&
                 translate_dec_list(generator, tree_child_at(node, 2))
             ? 1
             : generator_fail(generator);
}

static int translate_def(IrGenerator *generator, const TreeNode *node) {
  if (!tree_is(node, "Def") || !int_specifier(tree_child_at(node, 0)) ||
      !tree_is(tree_child_at(node, 1), "DecList")) {
    return generator_fail(generator);
  }
  return translate_dec_list(generator, tree_child_at(node, 1));
}

int translate_def_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!tree_is(node, "DefList") ||
      !translate_def(generator, tree_child_at(node, 0))) {
    return generator_fail(generator);
  }
  return translate_def_list(generator, tree_child_at(node, 1));
}

static int translate_param_dec(IrGenerator *generator, const TreeNode *node) {
  ArrayShape shape = {0};
  char variable[OPERAND_CAPACITY];
  int succeeded = 0;

  if (!tree_is(node, "ParamDec") || !int_specifier(tree_child_at(node, 0)) ||
      !parse_var_dec(tree_child_at(node, 1), &shape) || shape.id == NULL ||
      shape.dimension_count > 1 ||
      !variable_declare(generator, shape.id->text, &shape,
                        shape.dimension_count == 1, variable,
                        sizeof(variable)) ||
      !ir_emit(&generator->ir, "PARAM %s", variable)) {
    goto done;
  }
  succeeded = 1;

done:
  array_shape_clear(&shape);
  return succeeded ? 1 : generator_fail(generator);
}

static int translate_var_list(IrGenerator *generator, const TreeNode *node) {
  if (!tree_is(node, "VarList") ||
      !translate_param_dec(generator, tree_child_at(node, 0))) {
    return generator_fail(generator);
  }
  if (tree_child_at(node, 1) == NULL && tree_child_at(node, 2) == NULL) {
    return 1;
  }
  return tree_is(tree_child_at(node, 1), "COMMA") &&
                 translate_var_list(generator, tree_child_at(node, 2))
             ? 1
             : generator_fail(generator);
}

static int translate_ext_def(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *function = tree_child_at(node, 1);
  const TreeNode *body = tree_child_at(node, 2);
  const TreeNode *name = tree_child_at(function, 0);
  const TreeNode *second = tree_child_at(function, 1);
  const TreeNode *third = tree_child_at(function, 2);
  const TreeNode *fourth = tree_child_at(function, 3);

  if (!tree_is(node, "ExtDef") || !int_specifier(tree_child_at(node, 0)) ||
      !tree_is(function, "FunDec") || !tree_is(body, "CompSt") ||
      !tree_is(name, "ID") || name->text == NULL || !tree_is(second, "LP")) {
    return generator_fail(generator);
  }
  if (!ir_emit(&generator->ir, "FUNCTION %s :", name->text)) {
    return generator_fail(generator);
  }
  if (tree_is(third, "RP") && fourth == NULL) {
    return translate_comp_st(generator, body);
  }
  if (!tree_is(third, "VarList") || !tree_is(fourth, "RP") ||
      tree_child_at(function, 4) != NULL ||
      !translate_var_list(generator, third) ||
      !translate_comp_st(generator, body)) {
    return generator_fail(generator);
  }
  return 1;
}

int translate_ext_def_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!tree_is(node, "ExtDefList") ||
      !translate_ext_def(generator, tree_child_at(node, 0))) {
    return generator_fail(generator);
  }
  return translate_ext_def_list(generator, tree_child_at(node, 1));
}
