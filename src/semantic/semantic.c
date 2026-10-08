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

static void analyze_program(SemanticContext *context, const TreeNode *node);

static void analyze_ext_def_list(SemanticContext *context,
                                 const TreeNode *node);

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

static void analyze_ext_def(SemanticContext *context, const TreeNode *node);

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

    return type_error();
}

static void analyze_ext_dec_list(SemanticContext *context, const TreeNode *node,
                                 Type *base_type);

static void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                            Type *return_type);

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
        analyze_fun_dec(context, child_1, base_type);
    }

    return;
}

static Field *analyze_var_list(SemanticContext *context, const TreeNode *node);

static void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                            Type *return_type) {
    if (context == NULL || node == NULL || return_type == NULL ||
        return_type == type_error() || !node_is(node, "FunDec")) {
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

    symbol_table_insert(&context->symbols, id->text, SYMBOL_FUNCTION,
                        function_type, id->line);
}

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
