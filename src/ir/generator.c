#include "generator.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define OPERAND_CAPACITY 128

struct IrVariable {
  char *source_name;
  char ir_name[OPERAND_CAPACITY];
  struct IrVariable *next;
};

static const TreeNode *child_at(const TreeNode *node, int index) {
  if (node == NULL || index < 0) {
    return NULL;
  }

  const TreeNode *current = node->first_child;
  for (int i = 0; current != NULL && i < index; i++) {
    current = current->next_sibling;
  }
  return current;
}

static int node_is(const TreeNode *node, const char *name) {
  return node != NULL && node->name != NULL && name != NULL &&
         strcmp(node->name, name) == 0;
}

static const TreeNode *find_child(const TreeNode *node, const char *name) {
  if (node == NULL || name == NULL) {
    return NULL;
  }

  for (const TreeNode *current = node->first_child; current != NULL;
       current = current->next_sibling) {
    if (node_is(current, name)) {
      return current;
    }
  }
  return NULL;
}

static int generator_fail(IrGenerator *generator) {
  if (generator != NULL) {
    generator->failed = 1;
  }
  return 0;
}

static int set_operand(IrGenerator *generator, char *result, size_t result_size,
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

static const char *variable_lookup(const IrGenerator *generator,
                                   const char *source_name) {
  if (generator == NULL || source_name == NULL) {
    return NULL;
  }

  for (const IrVariable *variable = generator->variables; variable != NULL;
       variable = variable->next) {
    if (strcmp(variable->source_name, source_name) == 0) {
      return variable->ir_name;
    }
  }
  return NULL;
}

static int variable_declare(IrGenerator *generator, const char *source_name,
                            char *result, size_t result_size) {
  if (generator == NULL || source_name == NULL ||
      variable_lookup(generator, source_name) != NULL) {
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

  int written = snprintf(variable->ir_name, sizeof(variable->ir_name), "v%u",
                         generator->next_variable);
  if (written < 0 || (size_t)written >= sizeof(variable->ir_name) ||
      !set_operand(generator, result, result_size, variable->ir_name)) {
    free(variable->source_name);
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
    free(variable);
    variable = next;
  }
  generator->variables = NULL;
}

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
  if (node_is(node, "PLUS")) {
    return "+";
  }
  if (node_is(node, "MINUS")) {
    return "-";
  }
  if (node_is(node, "STAR")) {
    return "*";
  }
  if (node_is(node, "DIV")) {
    return "/";
  }
  return NULL;
}

static int int_specifier(const TreeNode *node) {
  const TreeNode *type = child_at(node, 0);
  return node_is(node, "Specifier") && node_is(type, "TYPE") &&
         type->text != NULL && strcmp(type->text, "int") == 0;
}

static const TreeNode *simple_var_id(const TreeNode *node) {
  const TreeNode *id = child_at(node, 0);
  return node_is(node, "VarDec") && node_is(id, "ID") &&
                 child_at(node, 1) == NULL
             ? id
             : NULL;
}

static int translate_exp(IrGenerator *generator, const TreeNode *node,
                         char *result, size_t result_size);
static int translate_cond(IrGenerator *generator, const TreeNode *node,
                          const char *true_label, const char *false_label);

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

static int translate_array_address(IrGenerator *generator, const TreeNode *node,
                                   char *result, size_t result_size) {
  const TreeNode *base_exp = child_at(node, 0);
  const TreeNode *index_exp = child_at(node, 2);
  const TreeNode *base_id = child_at(base_exp, 0);
  const char *base = NULL;
  char index[OPERAND_CAPACITY];
  char offset[OPERAND_CAPACITY];
  char address[OPERAND_CAPACITY];

  if (!node_is(node, "Exp") || !node_is(child_at(node, 1), "LB") ||
      !node_is(index_exp, "Exp") || !node_is(child_at(node, 3), "RB") ||
      child_at(node, 4) != NULL || !node_is(base_exp, "Exp") ||
      !node_is(base_id, "ID") || child_at(base_exp, 1) != NULL ||
      (base = variable_lookup(generator, base_id->text)) == NULL ||
      !translate_exp(generator, index_exp, index, sizeof(index)) ||
      !ir_new_temp(&generator->ir, offset, sizeof(offset)) ||
      !ir_new_temp(&generator->ir, address, sizeof(address)) ||
      !ir_emit(&generator->ir, "%s := %s * #4", offset, index) ||
      !ir_emit(&generator->ir, "%s := &%s + %s", address, base, offset)) {
    return generator_fail(generator);
  }
  return set_operand(generator, result, result_size, address);
}

static int translate_args(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *first = child_at(node, 0);
  const TreeNode *comma = child_at(node, 1);
  const TreeNode *rest = child_at(node, 2);
  char argument[OPERAND_CAPACITY];

  if (!node_is(node, "Args") || !node_is(first, "Exp") ||
      !translate_exp(generator, first, argument, sizeof(argument))) {
    return generator_fail(generator);
  }

  if (comma == NULL && rest == NULL) {
    return ir_emit(&generator->ir, "ARG %s", argument) ? 1
                                                     : generator_fail(generator);
  }
  if (!node_is(comma, "COMMA") || !node_is(rest, "Args") ||
      !translate_args(generator, rest) ||
      !ir_emit(&generator->ir, "ARG %s", argument)) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_write_call(IrGenerator *generator, const TreeNode *args,
                                char *result, size_t result_size) {
  const TreeNode *argument_exp = child_at(args, 0);
  char argument[OPERAND_CAPACITY];

  if (!node_is(args, "Args") || !node_is(argument_exp, "Exp") ||
      child_at(args, 1) != NULL ||
      !translate_exp(generator, argument_exp, argument, sizeof(argument)) ||
      !ir_emit(&generator->ir, "WRITE %s", argument)) {
    return generator_fail(generator);
  }
  return set_operand(generator, result, result_size, "#0");
}

static int translate_call(IrGenerator *generator, const TreeNode *node,
                          char *result, size_t result_size) {
  const TreeNode *name = child_at(node, 0);
  const TreeNode *left_parenthesis = child_at(node, 1);
  const TreeNode *third = child_at(node, 2);
  const TreeNode *fourth = child_at(node, 3);
  char temporary[OPERAND_CAPACITY];

  if (!node_is(name, "ID") || name->text == NULL ||
      !node_is(left_parenthesis, "LP")) {
    return generator_fail(generator);
  }

  if (node_is(third, "RP") && fourth == NULL) {
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

  if (!node_is(third, "Args") || !node_is(fourth, "RP") ||
      child_at(node, 4) != NULL) {
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

static int translate_exp(IrGenerator *generator, const TreeNode *node,
                         char *result, size_t result_size) {
  const TreeNode *child_0 = child_at(node, 0);
  const TreeNode *child_1 = child_at(node, 1);
  const TreeNode *child_2 = child_at(node, 2);
  const TreeNode *child_3 = child_at(node, 3);

  if (generator == NULL || node == NULL || result == NULL || result_size == 0 ||
      generator->failed || generator->ir.failed || !node_is(node, "Exp")) {
    return generator_fail(generator);
  }
  if (node_is(child_0, "INT") && child_1 == NULL) {
    return set_integer_operand(generator, result, result_size, child_0->text);
  }
  if (node_is(child_0, "ID") && child_1 == NULL) {
    const char *variable = variable_lookup(generator, child_0->text);
    return variable == NULL ? generator_fail(generator)
                            : set_operand(generator, result, result_size, variable);
  }
  if (node_is(child_0, "LP") && node_is(child_1, "Exp") &&
      node_is(child_2, "RP") && child_3 == NULL) {
    return translate_exp(generator, child_1, result, result_size);
  }
  if (node_is(child_0, "ID") && node_is(child_1, "LP")) {
    return translate_call(generator, node, result, result_size);
  }
  if (node_is(child_0, "Exp") && node_is(child_1, "LB")) {
    char address[OPERAND_CAPACITY];
    char value[OPERAND_CAPACITY];
    if (!translate_array_address(generator, node, address, sizeof(address)) ||
        !ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := *%s", value, address)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }
  if (node_is(child_0, "MINUS") && node_is(child_1, "Exp") &&
      child_2 == NULL) {
    char operand[OPERAND_CAPACITY];
    char value[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_1, operand, sizeof(operand)) ||
        !ir_new_temp(&generator->ir, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := #0 - %s", value, operand)) {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, value);
  }
  if (node_is(child_0, "NOT") || node_is(child_1, "RELOP") ||
      node_is(child_1, "AND") || node_is(child_1, "OR")) {
    return translate_boolean_value(generator, node, result, result_size);
  }
  if (node_is(child_0, "Exp") && node_is(child_1, "ASSIGNOP") &&
      node_is(child_2, "Exp") && child_3 == NULL) {
    char right[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_2, right, sizeof(right))) {
      return generator_fail(generator);
    }
    if (node_is(child_at(child_0, 0), "ID") && child_at(child_0, 1) == NULL) {
      const char *left = variable_lookup(generator, child_at(child_0, 0)->text);
      if (left == NULL || !ir_emit(&generator->ir, "%s := %s", left, right)) {
        return generator_fail(generator);
      }
    } else if (node_is(child_at(child_0, 1), "LB")) {
      char address[OPERAND_CAPACITY];
      if (!translate_array_address(generator, child_0, address, sizeof(address)) ||
          !ir_emit(&generator->ir, "*%s := %s", address, right)) {
        return generator_fail(generator);
      }
    } else {
      return generator_fail(generator);
    }
    return set_operand(generator, result, result_size, right);
  }

  const char *operator = arithmetic_operator(child_1);
  if (node_is(child_0, "Exp") && operator != NULL && node_is(child_2, "Exp") &&
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

static int translate_cond(IrGenerator *generator, const TreeNode *node,
                          const char *true_label, const char *false_label) {
  const TreeNode *child_0 = child_at(node, 0);
  const TreeNode *child_1 = child_at(node, 1);
  const TreeNode *child_2 = child_at(node, 2);
  const TreeNode *child_3 = child_at(node, 3);

  if (generator == NULL || node == NULL || true_label == NULL ||
      false_label == NULL || !node_is(node, "Exp")) {
    return generator_fail(generator);
  }
  if (node_is(child_0, "LP") && node_is(child_1, "Exp") &&
      node_is(child_2, "RP") && child_3 == NULL) {
    return translate_cond(generator, child_1, true_label, false_label);
  }
  if (node_is(child_0, "NOT") && node_is(child_1, "Exp") && child_2 == NULL) {
    return translate_cond(generator, child_1, false_label, true_label);
  }
  if (node_is(child_0, "Exp") && node_is(child_1, "RELOP") &&
      node_is(child_2, "Exp") && child_3 == NULL) {
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
  if (node_is(child_0, "Exp") && node_is(child_1, "AND") &&
      node_is(child_2, "Exp") && child_3 == NULL) {
    char next_label[OPERAND_CAPACITY];
    if (!ir_new_label(&generator->ir, next_label, sizeof(next_label)) ||
        !translate_cond(generator, child_0, next_label, false_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", next_label) ||
        !translate_cond(generator, child_2, true_label, false_label)) {
      return generator_fail(generator);
    }
    return 1;
  }
  if (node_is(child_0, "Exp") && node_is(child_1, "OR") &&
      node_is(child_2, "Exp") && child_3 == NULL) {
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

static int translate_dec(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *var_dec = child_at(node, 0);
  const TreeNode *assign = child_at(node, 1);
  const TreeNode *initializer = child_at(node, 2);
  const TreeNode *id = simple_var_id(var_dec);
  char variable[OPERAND_CAPACITY];

  if (!node_is(node, "Dec") || !node_is(var_dec, "VarDec")) {
    return generator_fail(generator);
  }
  if (id != NULL) {
    if (!variable_declare(generator, id->text, variable, sizeof(variable))) {
      return generator_fail(generator);
    }
    if (assign == NULL && initializer == NULL) {
      return 1;
    }
    char value[OPERAND_CAPACITY];
    if (!node_is(assign, "ASSIGNOP") || !node_is(initializer, "Exp") ||
        !translate_exp(generator, initializer, value, sizeof(value)) ||
        !ir_emit(&generator->ir, "%s := %s", variable, value)) {
      return generator_fail(generator);
    }
    return 1;
  }

  const TreeNode *inner = child_at(var_dec, 0);
  const TreeNode *length = child_at(var_dec, 2);
  id = simple_var_id(inner);
  if (id == NULL || !node_is(child_at(var_dec, 1), "LB") ||
      !node_is(length, "INT") || !node_is(child_at(var_dec, 3), "RB") ||
      child_at(var_dec, 4) != NULL || assign != NULL || initializer != NULL) {
    return generator_fail(generator);
  }

  errno = 0;
  char *end = NULL;
  long element_count = strtol(length->text, &end, 0);
  if (errno == ERANGE || end == length->text || *end != '\0' ||
      element_count <= 0 || element_count > INT_MAX / 4 ||
      !variable_declare(generator, id->text, variable, sizeof(variable)) ||
      !ir_emit(&generator->ir, "DEC %s %ld", variable, element_count * 4)) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_dec_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!node_is(node, "DecList") || !translate_dec(generator, child_at(node, 0))) {
    return generator_fail(generator);
  }
  if (child_at(node, 1) == NULL && child_at(node, 2) == NULL) {
    return 1;
  }
  return node_is(child_at(node, 1), "COMMA") &&
                 translate_dec_list(generator, child_at(node, 2))
             ? 1
             : generator_fail(generator);
}

static int translate_def(IrGenerator *generator, const TreeNode *node) {
  if (!node_is(node, "Def") || !int_specifier(child_at(node, 0)) ||
      !node_is(child_at(node, 1), "DecList")) {
    return generator_fail(generator);
  }
  return translate_dec_list(generator, child_at(node, 1));
}

static int translate_def_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!node_is(node, "DefList") || !translate_def(generator, child_at(node, 0))) {
    return generator_fail(generator);
  }
  return translate_def_list(generator, child_at(node, 1));
}

static int translate_comp_st(IrGenerator *generator, const TreeNode *node);

static int translate_stmt(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *child_0 = child_at(node, 0);
  if (!node_is(node, "Stmt")) {
    return generator_fail(generator);
  }
  if (node_is(child_0, "Exp") && node_is(child_at(node, 1), "SEMI")) {
    char unused[OPERAND_CAPACITY];
    return translate_exp(generator, child_0, unused, sizeof(unused));
  }
  if (node_is(child_0, "RETURN") && node_is(child_at(node, 1), "Exp") &&
      node_is(child_at(node, 2), "SEMI")) {
    char value[OPERAND_CAPACITY];
    if (!translate_exp(generator, child_at(node, 1), value, sizeof(value)) ||
        !ir_emit(&generator->ir, "RETURN %s", value)) {
      return generator_fail(generator);
    }
    return 1;
  }
  if (node_is(child_0, "CompSt")) {
    return translate_comp_st(generator, child_0);
  }
  if (node_is(child_0, "IF") && node_is(child_at(node, 1), "LP") &&
      node_is(child_at(node, 2), "Exp") && node_is(child_at(node, 3), "RP") &&
      node_is(child_at(node, 4), "Stmt")) {
    char true_label[OPERAND_CAPACITY];
    char false_label[OPERAND_CAPACITY];
    char end_label[OPERAND_CAPACITY];
    if (!ir_new_label(&generator->ir, true_label, sizeof(true_label)) ||
        !ir_new_label(&generator->ir, false_label, sizeof(false_label)) ||
        !translate_cond(generator, child_at(node, 2), true_label, false_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", true_label) ||
        !translate_stmt(generator, child_at(node, 4))) {
      return generator_fail(generator);
    }
    if (child_at(node, 5) == NULL) {
      return ir_emit(&generator->ir, "LABEL %s :", false_label)
                 ? 1
                 : generator_fail(generator);
    }
    if (!node_is(child_at(node, 5), "ELSE") || !node_is(child_at(node, 6), "Stmt") ||
        child_at(node, 7) != NULL ||
        !ir_new_label(&generator->ir, end_label, sizeof(end_label)) ||
        !ir_emit(&generator->ir, "GOTO %s", end_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", false_label) ||
        !translate_stmt(generator, child_at(node, 6)) ||
        !ir_emit(&generator->ir, "LABEL %s :", end_label)) {
      return generator_fail(generator);
    }
    return 1;
  }
  if (node_is(child_0, "WHILE") && node_is(child_at(node, 1), "LP") &&
      node_is(child_at(node, 2), "Exp") && node_is(child_at(node, 3), "RP") &&
      node_is(child_at(node, 4), "Stmt") && child_at(node, 5) == NULL) {
    char begin_label[OPERAND_CAPACITY];
    char true_label[OPERAND_CAPACITY];
    char false_label[OPERAND_CAPACITY];
    if (!ir_new_label(&generator->ir, begin_label, sizeof(begin_label)) ||
        !ir_new_label(&generator->ir, true_label, sizeof(true_label)) ||
        !ir_new_label(&generator->ir, false_label, sizeof(false_label)) ||
        !ir_emit(&generator->ir, "LABEL %s :", begin_label) ||
        !translate_cond(generator, child_at(node, 2), true_label, false_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", true_label) ||
        !translate_stmt(generator, child_at(node, 4)) ||
        !ir_emit(&generator->ir, "GOTO %s", begin_label) ||
        !ir_emit(&generator->ir, "LABEL %s :", false_label)) {
      return generator_fail(generator);
    }
    return 1;
  }
  return generator_fail(generator);
}

static int translate_stmt_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!node_is(node, "StmtList") || !translate_stmt(generator, child_at(node, 0))) {
    return generator_fail(generator);
  }
  return translate_stmt_list(generator, child_at(node, 1));
}

static int translate_comp_st(IrGenerator *generator, const TreeNode *node) {
  if (!node_is(node, "CompSt") || !translate_def_list(generator, find_child(node, "DefList")) ||
      !translate_stmt_list(generator, find_child(node, "StmtList"))) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_param_dec(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *id = simple_var_id(child_at(node, 1));
  char variable[OPERAND_CAPACITY];
  if (!node_is(node, "ParamDec") || !int_specifier(child_at(node, 0)) ||
      id == NULL || !variable_declare(generator, id->text, variable, sizeof(variable)) ||
      !ir_emit(&generator->ir, "PARAM %s", variable)) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_var_list(IrGenerator *generator, const TreeNode *node) {
  if (!node_is(node, "VarList") || !translate_param_dec(generator, child_at(node, 0))) {
    return generator_fail(generator);
  }
  if (child_at(node, 1) == NULL && child_at(node, 2) == NULL) {
    return 1;
  }
  return node_is(child_at(node, 1), "COMMA") &&
                 translate_var_list(generator, child_at(node, 2))
             ? 1
             : generator_fail(generator);
}

static int translate_ext_def(IrGenerator *generator, const TreeNode *node) {
  const TreeNode *function = child_at(node, 1);
  const TreeNode *body = child_at(node, 2);
  const TreeNode *name = child_at(function, 0);
  const TreeNode *second = child_at(function, 1);
  const TreeNode *third = child_at(function, 2);
  const TreeNode *fourth = child_at(function, 3);

  if (!node_is(node, "ExtDef") || !int_specifier(child_at(node, 0)) ||
      !node_is(function, "FunDec") || !node_is(body, "CompSt") ||
      !node_is(name, "ID") || name->text == NULL || !node_is(second, "LP")) {
    return generator_fail(generator);
  }
  if (!ir_emit(&generator->ir, "FUNCTION %s :", name->text)) {
    return generator_fail(generator);
  }
  if (node_is(third, "RP") && fourth == NULL) {
    return translate_comp_st(generator, body);
  }
  if (!node_is(third, "VarList") || !node_is(fourth, "RP") ||
      child_at(function, 4) != NULL || !translate_var_list(generator, third) ||
      !translate_comp_st(generator, body)) {
    return generator_fail(generator);
  }
  return 1;
}

static int translate_ext_def_list(IrGenerator *generator, const TreeNode *node) {
  if (node == NULL) {
    return 1;
  }
  if (!node_is(node, "ExtDefList") || !translate_ext_def(generator, child_at(node, 0))) {
    return generator_fail(generator);
  }
  return translate_ext_def_list(generator, child_at(node, 1));
}

int ir_generate_program(IrGenerator *generator, const TreeNode *root) {
  if (generator == NULL || root == NULL || generator->failed ||
      generator->ir.failed || !node_is(root, "Program") ||
      !translate_ext_def_list(generator, child_at(root, 0)) || generator->failed ||
      generator->ir.failed) {
    return generator_fail(generator);
  }
  return 1;
}
