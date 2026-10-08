#include "semantic.h"
#include "symbol.h"
#include "type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SemanticContext {
    SymbolTable symbols;
    Type *current_return_type;
    int error_count;
} SemanticContext;

/* AST helpers */

static const TreeNode *child_at(const TreeNode *node, int index) {
    if (node == NULL || index < 0) {
        return NULL;
    }

    const TreeNode *current = node->first_child;
    int count = 0;

    while (current != NULL && count < index) {
        current = current->next_sibling;
        count++;
    }

    return current;
}

static int node_is(const TreeNode *node, const char *name) {
    if (node == NULL || name == NULL) {
        return 0;
    }

    return strcmp(node->name, name) == 0;
}

static const TreeNode *find_child(const TreeNode *node, const char *name) {
    if (node == NULL || name == NULL) {
        return NULL;
    }

    const TreeNode *current = node->first_child;

    while (current != NULL) {
        if (node_is(current, name)) {
            return current;
        }

        current = current->next_sibling;
    }

    return NULL;
}

static void report_error(SemanticContext *context, int error_type, int line,
                         const char *message) {
    if (context == NULL || message == NULL) {
        return;
    }

    if (error_type < 1 || error_type > 17) {
        return;
    }

    fprintf(stdout, "Error type %d at Line %d: %s.\n", error_type, line,
            message);

    context->error_count++;
}

/* Semantic traversal declarations */

static void analyze_program(SemanticContext *context, const TreeNode *node);
static void analyze_ext_def_list(SemanticContext *context,
                                 const TreeNode *node);
static void analyze_ext_def(SemanticContext *context, const TreeNode *node);
static Type *analyze_specifier(SemanticContext *context, const TreeNode *node);
static void analyze_ext_dec_list(SemanticContext *context, const TreeNode *node,
                                 Type *base_type);
static const TreeNode *var_dec_identifier(const TreeNode *node);
static Type *analyze_var_dec(const TreeNode *node, Type *base_type);
static Field *analyze_param_dec(SemanticContext *context, const TreeNode *node);
static Field *analyze_var_list(SemanticContext *context, const TreeNode *node);
static void insert_parameters(SemanticContext *context,
                              const Field *parameters);
static void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                            Type *return_type, const TreeNode *body);
static void analyze_comp_st(SemanticContext *context, const TreeNode *node);
static void analyze_def_list(SemanticContext *context, const TreeNode *node);
static void analyze_def(SemanticContext *context, const TreeNode *node);
static void analyze_dec_list(SemanticContext *context, const TreeNode *node,
                             Type *base_type);
static void analyze_dec(SemanticContext *context, const TreeNode *node,
                        Type *base_type);
static void analyze_stmt_list(SemanticContext *context, const TreeNode *node);
static void analyze_stmt(SemanticContext *context, const TreeNode *node);
static Type *analyze_exp(SemanticContext *context, const TreeNode *node);
static int exp_is_lvalue(const TreeNode *node);
static int arguments_match(SemanticContext *context, const TreeNode *node,
                           const Field *parameters, int *has_error);
static void analyze_condition(SemanticContext *context, const TreeNode *node);
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
static int struct_has_field(const Field *fields, const char *name);

int semantic_analyze(const TreeNode *root) {
    if (root == NULL) {
        return 0;
    }

    SemanticContext context;

    symbol_table_init(&context.symbols);
    context.current_return_type = NULL;
    context.error_count = 0;

    analyze_program(&context, root);

    symbol_table_clear(&context.symbols);

    return context.error_count;
}

/* Top-level program analysis */

static void analyze_program(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || !node_is(node, "Program")) {
        return;
    }

    const TreeNode *child_0 = child_at(node, 0);

    if (child_0 == NULL) {
        return;
    }

    analyze_ext_def_list(context, child_0);
}

static void analyze_ext_def_list(SemanticContext *context,
                                 const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "ExtDefList")) {
        return;
    }

    const TreeNode *child_0 = child_at(node, 0);

    if (child_0 == NULL) {
        return;
    }

    analyze_ext_def(context, child_0);
    analyze_ext_def_list(context, child_at(node, 1));
}

static Type *analyze_specifier(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "Specifier")) {
        return type_error();
    }

    const TreeNode *child_0 = child_at(node, 0);

    if (child_0 == NULL) {
        return type_error();
    }

    if (node_is(child_0, "TYPE") && child_0->text != NULL) {
        if (strcmp(child_0->text, "int") == 0) {
            return type_int();
        }

        if (strcmp(child_0->text, "float") == 0) {
            return type_float();
        }
    }

    if (node_is(child_0, "StructSpecifier")) {
        return analyze_struct_specifier(context, child_0);
    }

    return type_error();
}

static int struct_has_field(const Field *fields, const char *name) {
    if (name == NULL) {
        return 0;
    }

    const Field *current = fields;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return 1;
        }

        current = current->next;
    }

    return 0;
}

static void analyze_struct_dec(SemanticContext *context, const TreeNode *node,
                               Type *base_type, Field **fields) {
    if (context == NULL || node == NULL || base_type == NULL ||
        fields == NULL || !node_is(node, "Dec")) {
        return;
    }

    const TreeNode *var_dec = child_at(node, 0);
    const TreeNode *id = var_dec_identifier(var_dec);
    Type *type = analyze_var_dec(var_dec, base_type);

    if (id == NULL || id->text == NULL || type == type_error()) {
        return;
    }

    if (struct_has_field(*fields, id->text)) {
        report_error(context, 15, id->line, "Redefined field");
        return;
    }

    Field *field = field_new(id->text, type, id->line);

    if (field != NULL) {
        field_append(fields, field);
    }
}

static void analyze_struct_dec_list(SemanticContext *context,
                                    const TreeNode *node, Type *base_type,
                                    Field **fields) {
    if (context == NULL || node == NULL || base_type == NULL ||
        fields == NULL || !node_is(node, "DecList")) {
        return;
    }

    analyze_struct_dec(context, child_at(node, 0), base_type, fields);
    analyze_struct_dec_list(context, child_at(node, 2), base_type, fields);
}

static void analyze_struct_def(SemanticContext *context, const TreeNode *node,
                               Field **fields) {
    if (context == NULL || node == NULL || fields == NULL ||
        !node_is(node, "Def")) {
        return;
    }

    Type *base_type = analyze_specifier(context, child_at(node, 0));

    if (base_type == type_error()) {
        return;
    }

    analyze_struct_dec_list(context, find_child(node, "DecList"), base_type,
                            fields);
}

static void analyze_struct_def_list(SemanticContext *context,
                                    const TreeNode *node, Field **fields) {
    if (context == NULL || node == NULL || fields == NULL ||
        !node_is(node, "DefList")) {
        return;
    }

    analyze_struct_def(context, child_at(node, 0), fields);
    analyze_struct_def_list(context, child_at(node, 1), fields);
}

static Type *analyze_struct_specifier(SemanticContext *context,
                                      const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "StructSpecifier")) {
        return type_error();
    }

    const TreeNode *child_0 = child_at(node, 0);
    const TreeNode *child_1 = child_at(node, 1);

    if (!node_is(child_0, "STRUCT")) {
        return type_error();
    }

    if (node_is(child_1, "Tag")) {
        const TreeNode *id = child_at(child_1, 0);

        if (!node_is(id, "ID") || id->text == NULL) {
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

    if (node_is(child_1, "OptTag")) {
        tag_id = child_at(child_1, 0);

        if (!node_is(tag_id, "ID") || tag_id->text == NULL) {
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
        !symbol_table_insert(&context->symbols, tag_id->text, SYMBOL_STRUCT,
                            type, tag_id->line)) {
        return type_error();
    }

    Field *fields = NULL;
    analyze_struct_def_list(context, find_child(node, "DefList"), &fields);
    type->as.structure.fields = fields;

    return type;
}

static void analyze_ext_def(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "ExtDef")) {
        return;
    }

    const TreeNode *child_0 = child_at(node, 0);
    if (child_0 == NULL) {
        return;
    }

    Type *base_type = analyze_specifier(context, child_0);

    const TreeNode *child_1 = child_at(node, 1);
    if (child_1 == NULL) {
        return;
    }

    if (node_is(child_1, "ExtDecList")) {
        analyze_ext_dec_list(context, child_1, base_type);
    } else if (node_is(child_1, "FunDec")) {
        analyze_fun_dec(context, child_1, base_type, child_at(node, 2));
    }

    return;
}

/* Type and declaration analysis */

static const TreeNode *var_dec_identifier(const TreeNode *node) {
    if (node == NULL || !node_is(node, "VarDec")) {
        return NULL;
    }

    const TreeNode *child_0 = child_at(node, 0);
    if (child_0 == NULL) {
        return NULL;
    }

    if (node_is(child_0, "ID")) {
        return child_0;
    }

    if (node_is(child_0, "VarDec")) {
        return var_dec_identifier(child_0);
    }

    return NULL;
}

static Type *analyze_var_dec(const TreeNode *node, Type *base_type) {
    if (node == NULL || base_type == NULL || !node_is(node, "VarDec")) {
        return type_error();
    }

    const TreeNode *child_0 = child_at(node, 0);
    if (child_0 == NULL) {
        return type_error();
    }

    if (node_is(child_0, "ID")) {
        return base_type;
    }

    if (node_is(child_0, "VarDec")) {
        Type *inner_type = analyze_var_dec(child_0, base_type);

        if (inner_type == type_error()) {
            return type_error();
        }

        const TreeNode *child_2 = child_at(node, 2);

        if (!node_is(child_2, "INT") || child_2->text == NULL) {
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

static Field *analyze_param_dec(SemanticContext *context,
                                const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "ParamDec")) {
        return NULL;
    }

    const TreeNode *specifier = child_at(node, 0);
    const TreeNode *var_dec = child_at(node, 1);

    Type *base_type = analyze_specifier(context, specifier);
    const TreeNode *id = var_dec_identifier(var_dec);
    Type *type = analyze_var_dec(var_dec, base_type);

    if (id == NULL || id->text == NULL || type == NULL ||
        type == type_error()) {
        return NULL;
    }

    return field_new(id->text, type, id->line);
}

static void analyze_ext_dec_list(SemanticContext *context, const TreeNode *node,
                                 Type *base_type) {
    if (context == NULL || node == NULL || base_type == NULL ||
        !node_is(node, "ExtDecList")) {
        return;
    }

    const TreeNode *var_dec = child_at(node, 0);
    if (var_dec != NULL) {
        const TreeNode *id = var_dec_identifier(var_dec);
        Type *type = analyze_var_dec(var_dec, base_type);

        if (id != NULL && id->text != NULL && type != NULL &&
            type != type_error()) {
            if (symbol_table_find(&context->symbols, id->text) != NULL) {
                report_error(context, 3, id->line, "Redefined variable");
            } else {
                symbol_table_insert(&context->symbols, id->text,
                                    SYMBOL_VARIABLE, type, id->line);
            }
        }
    }

    analyze_ext_dec_list(context, child_at(node, 2), base_type);
}

static void analyze_dec(SemanticContext *context, const TreeNode *node,
                        Type *base_type) {
    if (context == NULL || node == NULL || base_type == NULL ||
        !node_is(node, "Dec")) {
        return;
    }

    const TreeNode *var_dec = child_at(node, 0);
    const TreeNode *id = var_dec_identifier(var_dec);
    Type *type = analyze_var_dec(var_dec, base_type);

    if (id == NULL || id->text == NULL || type == NULL ||
        type == type_error()) {
        return;
    }

    if (symbol_table_find_current(&context->symbols, id->text) != NULL) {
        report_error(context, 3, id->line, "Redefined variable");
    } else {
        symbol_table_insert(&context->symbols, id->text, SYMBOL_VARIABLE, type,
                            id->line);
    }

    const TreeNode *assign_op = child_at(node, 1);
    const TreeNode *initializer = child_at(node, 2);

    if (node_is(assign_op, "ASSIGNOP")) {
        Type *initializer_type = analyze_exp(context, initializer);

        if (initializer_type != type_error() &&
            !type_equal(type, initializer_type)) {
            report_error(context, 5, assign_op->line,
                         "Type mismatched for assignment");
        }
    }
}

static void analyze_dec_list(SemanticContext *context, const TreeNode *node,
                             Type *base_type) {
    if (context == NULL || node == NULL || base_type == NULL ||
        !node_is(node, "DecList")) {
        return;
    }

    const TreeNode *dec = child_at(node, 0);
    if (dec != NULL) {
        analyze_dec(context, dec, base_type);
    }

    analyze_dec_list(context, child_at(node, 2), base_type);
}

static void analyze_def(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "Def")) {
        return;
    }

    const TreeNode *specifier = child_at(node, 0);
    Type *base_type = analyze_specifier(context, specifier);

    if (base_type == type_error()) {
        return;
    }

    const TreeNode *dec_list = find_child(node, "DecList");

    if (dec_list != NULL) {
        analyze_dec_list(context, dec_list, base_type);
    }
}

static void analyze_def_list(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "DefList")) {
        return;
    }

    const TreeNode *def = child_at(node, 0);

    if (def != NULL) {
        analyze_def(context, def);
    }

    analyze_def_list(context, child_at(node, 1));
}

static Field *analyze_var_list(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "VarList")) {
        return NULL;
    }

    const TreeNode *child_0 = child_at(node, 0);
    if (child_0 == NULL || !node_is(child_0, "ParamDec")) {
        return NULL;
    }

    Field *field = analyze_param_dec(context, child_0);
    if (field == NULL) {
        return NULL;
    }

    const TreeNode *child_2 = child_at(node, 2);
    if (child_2 == NULL) {
        return field;
    }

    if (!node_is(child_2, "VarList")) {
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
            if (symbol_table_find_current(&context->symbols, current->name) !=
                NULL) {
                report_error(context, 3, current->line, "Redefined variable");
            } else {
                symbol_table_insert(&context->symbols, current->name,
                                    SYMBOL_VARIABLE, current->type,
                                    current->line);
            }
        }

        current = current->next;
    }
}

/* Function and compound-statement analysis */

static void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                            Type *return_type, const TreeNode *body) {
    if (context == NULL || node == NULL || return_type == NULL ||
        return_type == type_error() || !node_is(node, "FunDec") ||
        body == NULL) {
        return;
    }

    const TreeNode *id = child_at(node, 0);
    const TreeNode *child_2 = child_at(node, 2);

    if (!node_is(id, "ID") || id->text == NULL) {
        return;
    }

    Field *parameters = NULL;

    if (node_is(child_2, "VarList")) {
        parameters = analyze_var_list(context, child_2);
        if (parameters == NULL) {
            return;
        }
    } else if (!node_is(child_2, "RP")) {
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

static void analyze_comp_st(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "CompSt")) {
        return;
    }

    const TreeNode *def_list = find_child(node, "DefList");

    if (def_list != NULL) {
        analyze_def_list(context, def_list);
    }

    const TreeNode *stmt_list = find_child(node, "StmtList");

    if (stmt_list != NULL) {
        analyze_stmt_list(context, stmt_list);
    }
}

static void analyze_stmt_list(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "StmtList")) {
        return;
    }

    const TreeNode *stmt = child_at(node, 0);

    if (stmt != NULL) {
        analyze_stmt(context, stmt);
    }

    analyze_stmt_list(context, child_at(node, 1));
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

static void analyze_stmt(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "Stmt")) {
        return;
    }

    const TreeNode *child_0 = child_at(node, 0);

    if (node_is(child_0, "Exp")) {
        analyze_exp(context, child_0);
    } else if (node_is(child_0, "RETURN")) {
        Type *returned_type = analyze_exp(context, child_at(node, 1));

        if (returned_type == type_error() ||
            context->current_return_type == NULL) {
            return;
        }

        if (!type_equal(returned_type, context->current_return_type)) {
            report_error(context, 8, child_0->line,
                         "Type mismatched for return");
        }
    } else if (node_is(child_0, "CompSt")) {
        symbol_table_enter_scope(&context->symbols);
        analyze_comp_st(context, child_0);
        symbol_table_leave_scope(&context->symbols);
    } else if (node_is(child_0, "WHILE")) {
        analyze_condition(context, child_at(node, 2));
        analyze_stmt(context, child_at(node, 4));
    } else if (node_is(child_0, "IF")) {
        analyze_condition(context, child_at(node, 2));
        analyze_stmt(context, child_at(node, 4));

        if (node_is(child_at(node, 5), "ELSE")) {
            analyze_stmt(context, child_at(node, 6));
        }
    }
}

static int exp_is_lvalue(const TreeNode *node) {
    if (node == NULL || !node_is(node, "Exp")) {
        return 0;
    }

    const TreeNode *child_0 = child_at(node, 0);
    const TreeNode *child_1 = child_at(node, 1);
    const TreeNode *child_2 = child_at(node, 2);
    const TreeNode *child_3 = child_at(node, 3);

    if (node_is(child_0, "ID") && child_1 == NULL) {
        return 1;
    }

    return node_is(child_0, "Exp") && node_is(child_1, "LB") &&
           node_is(child_2, "Exp") && node_is(child_3, "RB") &&
           exp_is_lvalue(child_0);
}

static int arguments_match(SemanticContext *context, const TreeNode *node,
                           const Field *parameters, int *has_error) {
    if (context == NULL || node == NULL || parameters == NULL ||
        has_error == NULL || !node_is(node, "Args")) {
        return 0;
    }

    const TreeNode *argument_node = child_at(node, 0);
    Type *argument_type = analyze_exp(context, argument_node);

    if (argument_type == type_error()) {
        *has_error = 1;
        return 0;
    }

    if (!type_equal(argument_type, parameters->type)) {
        return 0;
    }

    const TreeNode *child_1 = child_at(node, 1);

    if (child_1 == NULL) {
        return parameters->next == NULL;
    }

    if (!node_is(child_1, "COMMA")) {
        return 0;
    }

    return arguments_match(context, child_at(node, 2), parameters->next,
                           has_error);
}

static Type *analyze_exp(SemanticContext *context, const TreeNode *node) {
    if (context == NULL || node == NULL || !node_is(node, "Exp")) {
        return type_error();
    }

    const TreeNode *child_0 = child_at(node, 0);
    const TreeNode *child_1 = child_at(node, 1);
    const TreeNode *child_2 = child_at(node, 2);
    const TreeNode *child_3 = child_at(node, 3);

    if (node_is(child_0, "ID") && child_1 == NULL) {
        if (child_0->text == NULL) {
            return type_error();
        }

        const Symbol *symbol =
            symbol_table_find(&context->symbols, child_0->text);

        if (symbol == NULL || symbol->kind != SYMBOL_VARIABLE) {
            report_error(context, 1, child_0->line, "Undefined variable");
            return type_error();
        }

        return symbol->type;
    }

    if (node_is(child_0, "INT") && child_1 == NULL) {
        return type_int();
    }

    if (node_is(child_0, "FLOAT") && child_1 == NULL) {
        return type_float();
    }

    if (node_is(child_0, "ID") && node_is(child_1, "LP") &&
        node_is(child_2, "RP")) {
        if (child_0->text == NULL) {
            return type_error();
        }

        const Symbol *symbol =
            symbol_table_find(&context->symbols, child_0->text);

        if (symbol == NULL) {
            report_error(context, 2, child_0->line, "Undefined function");
            return type_error();
        }

        if (symbol->kind != SYMBOL_FUNCTION || symbol->type == NULL ||
            symbol->type->kind != TYPE_FUNCTION) {
            report_error(context, 11, child_0->line, "Not a function");
            return type_error();
        }

        if (symbol->type->as.function.parameters != NULL) {
            report_error(context, 9, child_0->line,
                         "Function is not applicable for arguments");
            return type_error();
        }

        return symbol->type->as.function.return_type;
    }

    if (node_is(child_0, "ID") && node_is(child_1, "LP") &&
        node_is(child_2, "Args") && node_is(child_3, "RP")) {
        if (child_0->text == NULL) {
            return type_error();
        }

        const Symbol *symbol =
            symbol_table_find(&context->symbols, child_0->text);

        if (symbol == NULL) {
            report_error(context, 2, child_0->line, "Undefined function");
            return type_error();
        }

        if (symbol->kind != SYMBOL_FUNCTION || symbol->type == NULL ||
            symbol->type->kind != TYPE_FUNCTION) {
            report_error(context, 11, child_0->line, "Not a function");
            return type_error();
        }

        int has_error = 0;

        if (!arguments_match(context, child_2,
                             symbol->type->as.function.parameters,
                             &has_error)) {
            if (!has_error) {
                report_error(context, 9, child_0->line,
                             "Function is not applicable for arguments");
            }

            return type_error();
        }

        return symbol->type->as.function.return_type;
    }

    if (node_is(child_0, "Exp") && node_is(child_1, "LB") &&
        node_is(child_2, "Exp") && node_is(child_3, "RB")) {
        Type *array_type = analyze_exp(context, child_0);
        Type *index_type = analyze_exp(context, child_2);

        if (array_type == type_error() || index_type == type_error()) {
            return type_error();
        }

        if (array_type->kind != TYPE_ARRAY) {
            report_error(context, 10, node->line, "Not an array");
            return type_error();
        }

        if (!type_equal(index_type, type_int())) {
            report_error(context, 12, node->line,
                         "Array index is not an integer");
            return type_error();
        }

        return array_type->as.array.element_type;
    }

    if (node_is(child_0, "LP") && node_is(child_1, "Exp") &&
        node_is(child_2, "RP")) {
        return analyze_exp(context, child_1);
    }

    if ((node_is(child_0, "MINUS") || node_is(child_0, "NOT")) &&
        node_is(child_1, "Exp") && child_2 == NULL) {
        Type *operand_type = analyze_exp(context, child_1);

        if (operand_type == type_error()) {
            return type_error();
        }

        if (node_is(child_0, "MINUS")) {
            if (!type_is_numeric(operand_type)) {
                report_error(context, 7, node->line,
                             "Type mismatched for operands");
                return type_error();
            }

            return operand_type;
        }

        if (!type_equal(operand_type, type_int())) {
            report_error(context, 7, node->line,
                         "Type mismatched for operands");
            return type_error();
        }

        return type_int();
    }

    if (node_is(child_0, "Exp") && node_is(child_1, "RELOP") &&
        node_is(child_2, "Exp")) {
        Type *left_type = analyze_exp(context, child_0);
        Type *right_type = analyze_exp(context, child_2);

        if (left_type == type_error() || right_type == type_error()) {
            return type_error();
        }

        if (!type_is_numeric(left_type) || !type_is_numeric(right_type) ||
            !type_equal(left_type, right_type)) {
            report_error(context, 7, node->line,
                         "Type mismatched for operands");
            return type_error();
        }

        return type_int();
    }

    if (node_is(child_0, "Exp") &&
        (node_is(child_1, "AND") || node_is(child_1, "OR")) &&
        node_is(child_2, "Exp")) {
        Type *left_type = analyze_exp(context, child_0);
        Type *right_type = analyze_exp(context, child_2);

        if (left_type == type_error() || right_type == type_error()) {
            return type_error();
        }

        if (!type_equal(left_type, type_int()) ||
            !type_equal(right_type, type_int())) {
            report_error(context, 7, node->line,
                         "Type mismatched for operands");
            return type_error();
        }

        return type_int();
    }

    if (node_is(child_0, "Exp") && node_is(child_1, "ASSIGNOP") &&
        node_is(child_2, "Exp")) {
        Type *left_type = analyze_exp(context, child_0);
        Type *right_type = analyze_exp(context, child_2);

        if (left_type == type_error() || right_type == type_error()) {
            return type_error();
        }

        if (!exp_is_lvalue(child_0)) {
            report_error(
                context, 6, node->line,
                "The left-hand side of an assignment must be a variable");
            return type_error();
        }

        if (!type_equal(left_type, right_type)) {
            report_error(context, 5, node->line,
                         "Type mismatched for assignment");
            return type_error();
        }

        return left_type;
    }

    if (node_is(child_0, "Exp") && node_is(child_2, "Exp") &&
        (node_is(child_1, "PLUS") || node_is(child_1, "MINUS") ||
         node_is(child_1, "STAR") || node_is(child_1, "DIV"))) {
        Type *left_type = analyze_exp(context, child_0);
        Type *right_type = analyze_exp(context, child_2);

        if (left_type == type_error() || right_type == type_error()) {
            return type_error();
        }

        if (!type_is_numeric(left_type) || !type_is_numeric(right_type) ||
            !type_equal(left_type, right_type)) {
            report_error(context, 7, node->line,
                         "Type mismatched for operands");
            return type_error();
        }

        return left_type;
    }

    return type_error();
}
