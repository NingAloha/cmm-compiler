#include "semantic_internal.h"

/* File-local helpers */

static void analyze_stmt_list(SemanticContext *context, const TreeNode *node);
static void analyze_stmt(SemanticContext *context, const TreeNode *node);
static void analyze_condition(SemanticContext *context, const TreeNode *node);

void analyze_comp_st(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "CompSt")) {
    return;
  }

  const TreeNode *def_list = tree_find_child(node, "DefList");

  if (def_list != NULL) {
    analyze_def_list(context, def_list);
  }

  const TreeNode *stmt_list = tree_find_child(node, "StmtList");

  if (stmt_list != NULL) {
    analyze_stmt_list(context, stmt_list);
  }
}

static void analyze_stmt_list(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "StmtList")) {
    return;
  }

  const TreeNode *stmt = tree_child_at(node, 0);

  if (stmt != NULL) {
    analyze_stmt(context, stmt);
  }

  analyze_stmt_list(context, tree_child_at(node, 1));
}

static void analyze_stmt(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "Stmt")) {
    return;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);

  if (tree_is(child_0, "Exp")) {
    analyze_exp(context, child_0);
  } else if (tree_is(child_0, "RETURN")) {
    Type *returned_type = analyze_exp(context, tree_child_at(node, 1));

    if (returned_type == type_error() || context->current_return_type == NULL) {
      return;
    }

    if (!type_equal(returned_type, context->current_return_type)) {
      report_error(context, 8, child_0->line, "Type mismatched for return");
    }
  } else if (tree_is(child_0, "CompSt")) {
    symbol_table_enter_scope(&context->symbols);
    analyze_comp_st(context, child_0);
    symbol_table_leave_scope(&context->symbols);
  } else if (tree_is(child_0, "WHILE")) {
    analyze_condition(context, tree_child_at(node, 2));
    analyze_stmt(context, tree_child_at(node, 4));
  } else if (tree_is(child_0, "IF")) {
    analyze_condition(context, tree_child_at(node, 2));
    analyze_stmt(context, tree_child_at(node, 4));

    if (tree_is(tree_child_at(node, 5), "ELSE")) {
      analyze_stmt(context, tree_child_at(node, 6));
    }
  }
}

static void analyze_condition(SemanticContext *context, const TreeNode *node) {
  Type *condition_type = analyze_exp(context, node);

  if (condition_type == type_error()) {
    return;
  }

  if (!type_equal(condition_type, type_int())) {
    report_error(context, 7, node->line, "Type mismatched for operands");
  }
}
