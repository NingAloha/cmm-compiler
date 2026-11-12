#include "semantic_internal.h"

#include <stdio.h>
#include <stdlib.h>

static void semantic_context_init(SemanticContext *context) {
  symbol_table_init(&context->symbols);
  context->current_return_type = NULL;
  context->error_count = 0;

  Type *read_type = type_new_function(type_int(), NULL);
  if (read_type != NULL) {
    (void)symbol_table_insert(&context->symbols, "read", SYMBOL_FUNCTION,
                              read_type, 0);
  }

  Field *write_parameter = field_new("value", type_int(), 0);
  Type *write_type = type_new_function(type_int(), write_parameter);
  if (write_type != NULL) {
    (void)symbol_table_insert(&context->symbols, "write", SYMBOL_FUNCTION,
                              write_type, 0);
  }
}

static void semantic_context_clear(SemanticContext *context) {
  symbol_table_clear(&context->symbols);
}

void report_error(SemanticContext *context, int error_type, int line,
                  const char *message) {
  if (context == NULL || message == NULL) {
    return;
  }

  if (error_type < 1 || error_type > 17) {
    return;
  }

  fprintf(stdout, "Error type %d at Line %d: %s.\n", error_type, line, message);

  context->error_count++;
}

static void analyze_ext_def(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "ExtDef")) {
    return;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);
  if (child_0 == NULL) {
    return;
  }

  Type *base_type = analyze_specifier(context, child_0);

  const TreeNode *child_1 = tree_child_at(node, 1);
  if (child_1 == NULL) {
    return;
  }

  if (tree_is(child_1, "ExtDecList")) {
    analyze_ext_dec_list(context, child_1, base_type);
  } else if (tree_is(child_1, "FunDec")) {
    analyze_fun_dec(context, child_1, base_type, tree_child_at(node, 2));
  }

  return;
}

static void analyze_ext_def_list(SemanticContext *context,
                                 const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "ExtDefList")) {
    return;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);

  if (child_0 == NULL) {
    return;
  }

  analyze_ext_def(context, child_0);
  analyze_ext_def_list(context, tree_child_at(node, 1));
}

static void analyze_program(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || !tree_is(node, "Program")) {
    return;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);

  if (child_0 == NULL) {
    return;
  }

  analyze_ext_def_list(context, child_0);
}

int semantic_analyze(const TreeNode *root) {
  if (root == NULL) {
    return 0;
  }

  SemanticContext context;
  semantic_context_init(&context);

  analyze_program(&context, root);

  int error_count = context.error_count;
  semantic_context_clear(&context);

  return error_count;
}
