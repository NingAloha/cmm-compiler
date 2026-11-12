#include "generator_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int set_integer_operand(IrGenerator *generator, char *result,
                               size_t result_size, const char *text) {
  if (text == NULL) {
    return generator_fail(generator);
  }

  errno = 0;
  char *end = NULL;
  long value = strtol(text, &end, 0);
  if (errno == ERANGE || end == text || *end != '\0') {
    return generator_fail(generator);
  }

  int written = snprintf(result, result_size, "#%ld", value);
  if (written < 0 || (size_t)written >= result_size) {
    return generator_fail(generator);
  }
  return 1;
}

static const char *arithmetic_operator(const TreeNode *node) {
  if (tree_is(node, "PLUS")) {
    return "+";
  }
  if (tree_is(node, "MINUS")) {
    return "-";
  }
  if (tree_is(node, "STAR")) {
    return "*";
  }
  if (tree_is(node, "DIV")) {
    return "/";
  }
  return NULL;
}

static int translate_boolean_value(IrGenerator *generator, const TreeNode *node,
                                   char *result, size_t result_size) {
  char value[OPERAND_CAPACITY];
  char true_label[OPERAND_CAPACITY];
  char false_label[OPERAND_CAPACITY];
  char end_label[OPERAND_CAPACITY];

  if (!ir_new_temp(&generator->ir, value, sizeof(value)) ||
      !ir_new_label(&generator->ir, true_label, sizeof(true_label)) ||
      !ir_new_label(&generator->ir, false_label, sizeof(false_label)) ||
      !ir_new_label(&generator->ir, end_label, sizeof(end_label)) ||
      !ir_emit(&generator->ir, "%s := #0", value) ||
      !translate_cond(generator, node, true_label, false_label) ||
      !ir_emit(&generator->ir, "LABEL %s :", true_label) ||
      !ir_emit(&generator->ir, "%s := #1", value) ||
      !ir_emit(&generator->ir, "GOTO %s", end_label) ||
      !ir_emit(&generator->ir, "LABEL %s :", false_label) ||
      !ir_emit(&generator->ir, "LABEL %s :", end_label)) {
    return generator_fail(generator);
  }

  return set_operand(generator, result, result_size, value);
}

static int translate_argument(IrGenerator *generator, const TreeNode *node,
                              char *result, size_t result_size) {
  const TreeNode *argument = node;

  while (tree_is(argument, "Exp") &&
         tree_is(tree_child_at(argument, 0), "LP") &&
         tree_is(tree_child_at(argument, 1), "Exp") &&
         tree_is(tree_child_at(argument, 2), "RP") &&
         tree_child_at(argument, 3) == NULL) {
    argument = tree_child_at(argument, 1);
  }

  const TreeNode *id = tree_child_at(argument, 0);

  if (tree_is(argument, "Exp") && tree_is(id, "ID") &&
      tree_child_at(argument, 1) == NULL) {
    const IrVariable *variable = variable_find(generator, id->text);
    if (variable != NULL && variable->dimension_count > 0) {
      if (variable->is_array_parameter) {
        return set_operand(generator, result, result_size, variable->ir_name);
      }

      char address[OPERAND_CAPACITY];
      int written =
          snprintf(address, sizeof(address), "&%s", variable->ir_name);
      return written < 0 || (size_t)written >= sizeof(address)
                 ? generator_fail(generator)
                 : set_operand(generator, result, result_size, address);
    }
  }

  if (tree_is(argument, "Exp") && tree_is(tree_child_at(argument, 0), "Exp") &&
      tree_is(tree_child_at(argument, 1), "LB")) {
    size_t remaining_dimensions = 0;
    if (!translate_array_address(generator, argument, result, result_size,
                                 &remaining_dimensions)) {
      return generator_fail(generator);
    }
    if (remaining_dimensions > 0) {
      return 1;
    }
    char value[OPERAND_CAPACITY];
    if (!ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := *%s", value, result)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }

  return translate_exp(generator, argument, result, result_size);
}

static int translate_args(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *first = tree_child_at(node, 0);
  const TreeNode *comma = tree_child_at(node, 1);
  const TreeNode *rest = tree_child_at(node, 2);
  char argument[OPERAND_CAPACITY];

  if (!tree_is(node, "Args") || !tree_is(first, "Exp") ||
      !translate_argument(generator, first, argument, sizeof(argument))) {
    return generator_fail(generator);
  }

  if (comma == NULL && rest == NULL) {
    return ir_emit(&generator->ir, "ARG %s", argument)
               ? 1
               : generator_fail(generator);
  }
  if (!tree_is(comma, "COMMA") || !tree_is(rest, "Args") ||
      !translate_args(generator, rest) ||
      !ir_emit(&generator->ir, "ARG %s", argument)) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_write_call(IrGenerator *generator, const TreeNode *args,
                                char *result, size_t result_size) {
  const TreeNode *argument_exp = tree_child_at(args, 0);
  char argument[OPERAND_CAPACITY];

  if (!tree_is(args, "Args") || !tree_is(argument_exp, "Exp") ||
      tree_child_at(args, 1) != NULL ||
      !translate_exp(generator, argument_exp, argument, sizeof(argument)) ||
      !ir_emit(&generator->ir, "WRITE %s", argument)) {
    return generator_fail(generator);
  }
  return set_operand(generator, result, result_size, "#0");
}

static int translate_call(IrGenerator *generator, const TreeNode *node,
                          char *result, size_t result_size) {
  const TreeNode *name = tree_child_at(node, 0);
  const TreeNode *left_parenthesis = tree_child_at(node, 1);
  const TreeNode *third = tree_child_at(node, 2);
  const TreeNode *fourth = tree_child_at(node, 3);
  char temporary[OPERAND_CAPACITY];

  if (!tree_is(name, "ID") || name->text == NULL ||
      !tree_is(left_parenthesis, "LP")) {
    return generator_fail(generator);
  }

  if (tree_is(third, "RP") && fourth == NULL) {
    if (strcmp(name->text, "read") == 0) {
      if (!ir_new_temp(&generator->ir, temporary, sizeof(temporary)) ||
          !ir_emit(&generator->ir, "READ %s", temporary)) {
        return generator_fail(generator);
      }
    } else if (!ir_new_temp(&generator->ir, temporary, sizeof(temporary)) ||
               !ir_emit(&generator->ir, "%s := CALL %s", temporary,
                        name->text)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, temporary);
  }

  if (!tree_is(third, "Args") || !tree_is(fourth, "RP") ||
      tree_child_at(node, 4) != NULL) {
    return generator_fail(generator);
  }
  if (strcmp(name->text, "write") == 0) {
    return translate_write_call(generator, third, result, result_size);
  }
  if (!translate_args(generator, third) ||
      !ir_new_temp(&generator->ir, temporary, sizeof(temporary)) ||
      !ir_emit(&generator->ir, "%s := CALL %s", temporary, name->text)) {
    return generator_fail(generator);
  }
  return set_operand(generator, result, result_size, temporary);
}

int translate_exp(IrGenerator *generator, const TreeNode *node, char *result,
                  size_t result_size) {
  const TreeNode *child_0 = tree_child_at(node, 0);
  const TreeNode *child_1 = tree_child_at(node, 1);
  const TreeNode *child_2 = tree_child_at(node, 2);
  const TreeNode *child_3 = tree_child_at(node, 3);

  if (generator == NULL || node == NULL || result == NULL || result_size == 0 ||
      generator->failed || generator->ir.failed || !tree_is(node, "Exp")) {
    return generator_fail(generator);
  }
  if (tree_is(child_0, "INT") && child_1 == NULL) {
    return set_integer_operand(generator, result, result_size, child_0->text);
  }
  if (tree_is(child_0, "ID") && child_1 == NULL) {
    const IrVariable *variable = variable_find(generator, child_0->text);
    return variable == NULL || variable->dimension_count > 0
               ? generator_fail(generator)
               : set_operand(generator, result, result_size, variable->ir_name);
  }
  if (tree_is(child_0, "LP") && tree_is(child_1, "Exp") &&
      tree_is(child_2, "RP") && child_3 == NULL) {
    return translate_exp(generator, child_1, result, result_size);
  }
  if (tree_is(child_0, "ID") && tree_is(child_1, "LP")) {
    return translate_call(generator, node, result, result_size);
  }
  if (tree_is(child_0, "Exp") && tree_is(child_1, "LB")) {
    char address[OPERAND_CAPACITY];
    char value[OPERAND_CAPACITY];
    size_t remaining_dimensions = 0;
    if (!translate_array_address(generator, node, address, sizeof(address),
                                 &remaining_dimensions) ||
        remaining_dimensions != 0 ||
        !ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := *%s", value, address)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }
  if (tree_is(child_0, "MINUS") && tree_is(child_1, "Exp") && child_2 == NULL) {
    char operand[OPERAND_CAPACITY];
    char value[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_1, operand, sizeof(operand)) ||
        !ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := #0 - %s", value, operand)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }
  if (tree_is(child_0, "NOT") || tree_is(child_1, "RELOP") ||
      tree_is(child_1, "AND") || tree_is(child_1, "OR")) {
    return translate_boolean_value(generator, node, result, result_size);
  }
  if (tree_is(child_0, "Exp") && tree_is(child_1, "ASSIGNOP") &&
      tree_is(child_2, "Exp") && child_3 == NULL) {
    char right[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_2, right, sizeof(right))) {
      return generator_fail(generator);
    }
    if (tree_is(tree_child_at(child_0, 0), "ID") &&
        tree_child_at(child_0, 1) == NULL) {
      const IrVariable *left =
          variable_find(generator, tree_child_at(child_0, 0)->text);
      if (left == NULL || left->dimension_count > 0 ||
          !ir_emit(&generator->ir, "%s := %s", left->ir_name, right)) {
        return generator_fail(generator);
      }
    } else if (tree_is(tree_child_at(child_0, 1), "LB")) {
      char address[OPERAND_CAPACITY];
      size_t remaining_dimensions = 0;
      if (!translate_array_address(generator, child_0, address, sizeof(address),
                                   &remaining_dimensions) ||
          remaining_dimensions != 0 ||
          !ir_emit(&generator->ir, "*%s := %s", address, right)) {
        return generator_fail(generator);
      }
    } else {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, right);
  }

  const char *operator = arithmetic_operator(child_1);
  if (tree_is(child_0, "Exp") && operator != NULL && tree_is(child_2, "Exp") &&
      child_3 == NULL) {
    char left[OPERAND_CAPACITY];
    char right[OPERAND_CAPACITY];
    char value[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_0, left, sizeof(left)) ||
        !translate_exp(generator, child_2, right, sizeof(right)) ||
        !ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := %s %s %s", value, left, operator,
                 right)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }
  return generator_fail(generator);
}

int translate_cond(IrGenerator *generator, const TreeNode *node,
                   const char *true_label, const char *false_label) {
  const TreeNode *child_0 = tree_child_at(node, 0);
  const TreeNode *child_1 = tree_child_at(node, 1);
  const TreeNode *child_2 = tree_child_at(node, 2);
  const TreeNode *child_3 = tree_child_at(node, 3);

  if (generator == NULL || node == NULL || true_label == NULL ||
      false_label == NULL || !tree_is(node, "Exp")) {
    return generator_fail(generator);
  }
  if (tree_is(child_0, "LP") && tree_is(child_1, "Exp") &&
      tree_is(child_2, "RP") && child_3 == NULL) {
    return translate_cond(generator, child_1, true_label, false_label);
  }
  if (tree_is(child_0, "NOT") && tree_is(child_1, "Exp") && child_2 == NULL) {
    return translate_cond(generator, child_1, false_label, true_label);
  }
  if (tree_is(child_0, "Exp") && tree_is(child_1, "RELOP") &&
      tree_is(child_2, "Exp") && child_3 == NULL) {
    char left[OPERAND_CAPACITY];
    char right[OPERAND_CAPACITY];
    if (child_1->text == NULL ||
        !translate_exp(generator, child_0, left, sizeof(left)) ||
        !translate_exp(generator, child_2, right, sizeof(right)) ||
        !ir_emit(&generator->ir, "IF %s %s %s GOTO %s", left, child_1->text,
                 right, true_label) ||
        !ir_emit(&generator->ir, "GOTO %s", false_label)) {
      return generator_fail(generator);
    }
    return 1;
  }
  if (tree_is(child_0, "Exp") && tree_is(child_1, "AND") &&
      tree_is(child_2, "Exp") && child_3 == NULL) {
    char next_label[OPERAND_CAPACITY];
    if (!ir_new_label(&generator->ir, next_label, sizeof(next_label)) ||
        !translate_cond(generator, child_0, next_label, false_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", next_label) ||
        !translate_cond(generator, child_2, true_label, false_label)) {
      return generator_fail(generator);
    }
    return 1;
  }
  if (tree_is(child_0, "Exp") && tree_is(child_1, "OR") &&
      tree_is(child_2, "Exp") && child_3 == NULL) {
    char next_label[OPERAND_CAPACITY];
    if (!ir_new_label(&generator->ir, next_label, sizeof(next_label)) ||
        !translate_cond(generator, child_0, true_label, next_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", next_label) ||
        !translate_cond(generator, child_2, true_label, false_label)) {
      return generator_fail(generator);
    }
    return 1;
  }

  char value[OPERAND_CAPACITY];
  if (!translate_exp(generator, node, value, sizeof(value)) ||
      !ir_emit(&generator->ir, "IF %s != #0 GOTO %s", value, true_label) ||
      !ir_emit(&generator->ir, "GOTO %s", false_label)) {
    return generator_fail(generator);
  }
  return 1;
}
