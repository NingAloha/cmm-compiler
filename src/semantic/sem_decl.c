#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

/* File-local helpers */

static Type *analyze_struct_specifier(SemanticContext *context,
                                      const TreeNode *node);
static void analyze_struct_def_list(SemanticContext *context,
                                    const TreeNode *node, Field **fields);
static void analyze_struct_def(SemanticContext *context, const TreeNode *node,
                               Field **fields);
static void analyze_struct_dec_list(SemanticContext *context,
                                    const TreeNode *node, Type *base_type,
                                    Field **fields);
static void analyze_struct_dec(SemanticContext *context, const TreeNode *node,
                               Type *base_type, Field **fields);
static const TreeNode *var_dec_identifier(const TreeNode *node);
static Type *analyze_var_dec(const TreeNode *node, Type *base_type);
static void analyze_def(SemanticContext *context, const TreeNode *node);
static void analyze_dec_list(SemanticContext *context, const TreeNode *node,
                             Type *base_type);
static void analyze_dec(SemanticContext *context, const TreeNode *node,
                        Type *base_type);
static int declare_variable(SemanticContext *context, const char *name,
                            Type *type, int line);
static Field *analyze_param_dec(SemanticContext *context, const TreeNode *node);
static Field *analyze_var_list(SemanticContext *context, const TreeNode *node);
static void insert_parameters(SemanticContext *context,
                              const Field *parameters);

Type *analyze_specifier(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "Specifier")) {
    return type_error();
  }

  const TreeNode *child_0 = tree_child_at(node, 0);

  if (child_0 == NULL) {
    return type_error();
  }

  if (tree_is(child_0, "TYPE") && child_0->text != NULL) {
    if (strcmp(child_0->text, "int") == 0) {
      return type_int();
    }

    if (strcmp(child_0->text, "float") == 0) {
      return type_float();
    }
  }

  if (tree_is(child_0, "StructSpecifier")) {
    return analyze_struct_specifier(context, child_0);
  }

  return type_error();
}

static Type *analyze_struct_specifier(SemanticContext *context,
                                      const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "StructSpecifier")) {
    return type_error();
  }

  const TreeNode *child_0 = tree_child_at(node, 0);
  const TreeNode *child_1 = tree_child_at(node, 1);

  if (!tree_is(child_0, "STRUCT")) {
    return type_error();
  }

  if (tree_is(child_1, "Tag")) {
    const TreeNode *id = tree_child_at(child_1, 0);

    if (!tree_is(id, "ID") || id->text == NULL) {
      return type_error();
    }

    const Symbol *symbol = symbol_table_find(&context->symbols, id->text);

    if (symbol == NULL || symbol->kind != SYMBOL_STRUCT) {
      report_error(context, 17, id->line, "Undefined structure");
      return type_error();
    }

    return symbol->type;
  }

  const TreeNode *tag_id = NULL;

  if (tree_is(child_1, "OptTag")) {
    tag_id = tree_child_at(child_1, 0);

    if (!tree_is(tag_id, "ID") || tag_id->text == NULL) {
      return type_error();
    }

    if (symbol_table_find(&context->symbols, tag_id->text) != NULL) {
      report_error(context, 16, tag_id->line, "Duplicated name");
      return type_error();
    }
  }

  Type *type = type_new_structure(tag_id == NULL ? NULL : tag_id->text, NULL);

  if (type == NULL) {
    return type_error();
  }

  if (tag_id != NULL &&
      !symbol_table_insert(&context->symbols, tag_id->text, SYMBOL_STRUCT, type,
                           tag_id->line)) {
    return type_error();
  }

  Field *fields = NULL;
  analyze_struct_def_list(context, tree_find_child(node, "DefList"), &fields);
  type->as.structure.fields = fields;

  return type;
}

static void analyze_struct_def_list(SemanticContext *context,
                                    const TreeNode *node, Field **fields) {
  if (context == NULL || node == NULL || fields == NULL ||
      !tree_is(node, "DefList")) {
    return;
  }

  analyze_struct_def(context, tree_child_at(node, 0), fields);
  analyze_struct_def_list(context, tree_child_at(node, 1), fields);
}

static void analyze_struct_def(SemanticContext *context, const TreeNode *node,
                               Field **fields) {
  if (context == NULL || node == NULL || fields == NULL ||
      !tree_is(node, "Def")) {
    return;
  }

  Type *base_type = analyze_specifier(context, tree_child_at(node, 0));

  if (base_type == type_error()) {
    return;
  }

  analyze_struct_dec_list(context, tree_find_child(node, "DecList"), base_type,
                          fields);
}

static void analyze_struct_dec_list(SemanticContext *context,
                                    const TreeNode *node, Type *base_type,
                                    Field **fields) {
  if (context == NULL || node == NULL || base_type == NULL || fields == NULL ||
      !tree_is(node, "DecList")) {
    return;
  }

  analyze_struct_dec(context, tree_child_at(node, 0), base_type, fields);
  analyze_struct_dec_list(context, tree_child_at(node, 2), base_type, fields);
}

static void analyze_struct_dec(SemanticContext *context, const TreeNode *node,
                               Type *base_type, Field **fields) {
  if (context == NULL || node == NULL || base_type == NULL || fields == NULL ||
      !tree_is(node, "Dec")) {
    return;
  }

  const TreeNode *var_dec = tree_child_at(node, 0);
  const TreeNode *id = var_dec_identifier(var_dec);
  Type *type = analyze_var_dec(var_dec, base_type);

  if (id == NULL || id->text == NULL || type == type_error()) {
    return;
  }

  if (tree_is(tree_child_at(node, 1), "ASSIGNOP")) {
    report_error(context, 15, id->line, "Illegal initialization of field");
  }

  if (field_find(*fields, id->text) != NULL) {
    report_error(context, 15, id->line, "Redefined field");
    return;
  }

  Field *field = field_new(id->text, type, id->line);

  if (field != NULL) {
    field_append(fields, field);
  }
}

static const TreeNode *var_dec_identifier(const TreeNode *node) {
  if (node == NULL || !tree_is(node, "VarDec")) {
    return NULL;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);
  if (child_0 == NULL) {
    return NULL;
  }

  if (tree_is(child_0, "ID")) {
    return child_0;
  }

  if (tree_is(child_0, "VarDec")) {
    return var_dec_identifier(child_0);
  }

  return NULL;
}

static Type *analyze_var_dec(const TreeNode *node, Type *base_type) {
  if (node == NULL || base_type == NULL || !tree_is(node, "VarDec")) {
    return type_error();
  }

  const TreeNode *child_0 = tree_child_at(node, 0);
  if (child_0 == NULL) {
    return type_error();
  }

  if (tree_is(child_0, "ID")) {
    return base_type;
  }

  if (tree_is(child_0, "VarDec")) {
    Type *inner_type = analyze_var_dec(child_0, base_type);

    if (inner_type == type_error()) {
      return type_error();
    }

    const TreeNode *child_2 = tree_child_at(node, 2);

    if (!tree_is(child_2, "INT") || child_2->text == NULL) {
      return type_error();
    }

    size_t length = strtoul(child_2->text, NULL, 10);
    Type *result = type_new_array(inner_type, length);
    if (result == NULL) {
      return type_error();
    }

    return result;
  }

  return type_error();
}

static void analyze_def(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "Def")) {
    return;
  }

  const TreeNode *specifier = tree_child_at(node, 0);
  Type *base_type = analyze_specifier(context, specifier);

  if (base_type == type_error()) {
    return;
  }

  const TreeNode *dec_list = tree_find_child(node, "DecList");

  if (dec_list != NULL) {
    analyze_dec_list(context, dec_list, base_type);
  }
}

void analyze_def_list(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "DefList")) {
    return;
  }

  const TreeNode *def = tree_child_at(node, 0);

  if (def != NULL) {
    analyze_def(context, def);
  }

  analyze_def_list(context, tree_child_at(node, 1));
}

static void analyze_dec_list(SemanticContext *context, const TreeNode *node,
                             Type *base_type) {
  if (context == NULL || node == NULL || base_type == NULL ||
      !tree_is(node, "DecList")) {
    return;
  }

  const TreeNode *dec = tree_child_at(node, 0);
  if (dec != NULL) {
    analyze_dec(context, dec, base_type);
  }

  analyze_dec_list(context, tree_child_at(node, 2), base_type);
}

static void analyze_dec(SemanticContext *context, const TreeNode *node,
                        Type *base_type) {
  if (context == NULL || node == NULL || base_type == NULL ||
      !tree_is(node, "Dec")) {
    return;
  }

  const TreeNode *var_dec = tree_child_at(node, 0);
  const TreeNode *id = var_dec_identifier(var_dec);
  Type *type = analyze_var_dec(var_dec, base_type);

  if (id == NULL || id->text == NULL || type == NULL || type == type_error()) {
    return;
  }

  declare_variable(context, id->text, type, id->line);

  const TreeNode *assign_op = tree_child_at(node, 1);
  const TreeNode *initializer = tree_child_at(node, 2);

  if (tree_is(assign_op, "ASSIGNOP")) {
    Type *initializer_type = analyze_exp(context, initializer);

    if (initializer_type != type_error() &&
        !type_equal(type, initializer_type)) {
      report_error(context, 5, assign_op->line,
                   "Type mismatched for assignment");
    }
  }
}

void analyze_ext_dec_list(SemanticContext *context, const TreeNode *node,
                          Type *base_type) {
  if (context == NULL || node == NULL || base_type == NULL ||
      !tree_is(node, "ExtDecList")) {
    return;
  }

  const TreeNode *var_dec = tree_child_at(node, 0);
  if (var_dec != NULL) {
    const TreeNode *id = var_dec_identifier(var_dec);
    Type *type = analyze_var_dec(var_dec, base_type);

    if (id != NULL && id->text != NULL && type != NULL &&
        type != type_error()) {
      declare_variable(context, id->text, type, id->line);
    }
  }

  analyze_ext_dec_list(context, tree_child_at(node, 2), base_type);
}

static int declare_variable(SemanticContext *context, const char *name,
                            Type *type, int line) {
  if (context == NULL || name == NULL || type == NULL || type == type_error()) {
    return 0;
  }

  if (symbol_table_find_current(&context->symbols, name) != NULL) {
    report_error(context, 3, line, "Redefined variable");
    return 0;
  }

  return symbol_table_insert(&context->symbols, name, SYMBOL_VARIABLE, type,
                             line);
}

static Field *analyze_param_dec(SemanticContext *context,
                                const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "ParamDec")) {
    return NULL;
  }

  const TreeNode *specifier = tree_child_at(node, 0);
  const TreeNode *var_dec = tree_child_at(node, 1);

  Type *base_type = analyze_specifier(context, specifier);
  const TreeNode *id = var_dec_identifier(var_dec);
  Type *type = analyze_var_dec(var_dec, base_type);

  if (id == NULL || id->text == NULL || type == NULL || type == type_error()) {
    return NULL;
  }

  return field_new(id->text, type, id->line);
}

static Field *analyze_var_list(SemanticContext *context, const TreeNode *node) {
  if (context == NULL || node == NULL || !tree_is(node, "VarList")) {
    return NULL;
  }

  const TreeNode *child_0 = tree_child_at(node, 0);
  if (child_0 == NULL || !tree_is(child_0, "ParamDec")) {
    return NULL;
  }

  Field *field = analyze_param_dec(context, child_0);
  if (field == NULL) {
    return NULL;
  }

  const TreeNode *child_2 = tree_child_at(node, 2);
  if (child_2 == NULL) {
    return field;
  }

  if (!tree_is(child_2, "VarList")) {
    return NULL;
  }

  Field *rest = analyze_var_list(context, child_2);
  if (rest == NULL) {
    return NULL;
  }

  field_append(&field, rest);
  return field;
}

static void insert_parameters(SemanticContext *context,
                              const Field *parameters) {
  if (context == NULL) {
    return;
  }

  const Field *current = parameters;

  while (current != NULL) {
    if (current->name != NULL && current->type != NULL) {
      declare_variable(context, current->name, current->type, current->line);
    }

    current = current->next;
  }
}

void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                     Type *return_type, const TreeNode *body) {
  if (context == NULL || node == NULL || return_type == NULL ||
      return_type == type_error() || !tree_is(node, "FunDec") || body == NULL) {
    return;
  }

  const TreeNode *id = tree_child_at(node, 0);
  const TreeNode *child_2 = tree_child_at(node, 2);

  if (!tree_is(id, "ID") || id->text == NULL) {
    return;
  }

  Field *parameters = NULL;

  if (tree_is(child_2, "VarList")) {
    parameters = analyze_var_list(context, child_2);
    if (parameters == NULL) {
      return;
    }
  } else if (!tree_is(child_2, "RP")) {
    return;
  }

  Type *function_type = type_new_function(return_type, parameters);
  if (function_type == NULL) {
    return;
  }

  if (symbol_table_find(&context->symbols, id->text) != NULL) {
    report_error(context, 4, id->line, "Redefined function");
    return;
  }

  if (!symbol_table_insert(&context->symbols, id->text, SYMBOL_FUNCTION,
                           function_type, id->line)) {
    return;
  }

  Type *saved_return_type = context->current_return_type;
  context->current_return_type = return_type;

  symbol_table_enter_scope(&context->symbols);
  insert_parameters(context, parameters);
  analyze_comp_st(context, body);
  symbol_table_leave_scope(&context->symbols);

  context->current_return_type = saved_return_type;
}
