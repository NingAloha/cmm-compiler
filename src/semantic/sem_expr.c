#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

/* File-local helpers */

static int exp_is_lvalue(const TreeNode *node);
static int arguments_match(SemanticContext *context, const TreeNode *node,
                           const Field *parameters, int *has_error);
static Type *lookup_function_type(SemanticContext *context, const TreeNode *id);
static const Field *struct_find_field(const Type *type, const char *name);

Type *analyze_exp(SemanticContext *context, const TreeNode *node) {
	if (context == NULL || node == NULL || !tree_is(node, "Exp")) {
		return type_error();
	}

	const TreeNode *child_0 = tree_child_at(node, 0);
	const TreeNode *child_1 = tree_child_at(node, 1);
	const TreeNode *child_2 = tree_child_at(node, 2);
	const TreeNode *child_3 = tree_child_at(node, 3);

	if (tree_is(child_0, "ID") && child_1 == NULL) {
		if (child_0->text == NULL) {
			return type_error();
		}

		const Symbol *symbol = symbol_table_find(&context->symbols, child_0->text);

		if (symbol == NULL || symbol->kind != SYMBOL_VARIABLE) {
			report_error(context, 1, child_0->line, "Undefined variable");
			return type_error();
		}

		return symbol->type;
	}

	if (tree_is(child_0, "INT") && child_1 == NULL) {
		return type_int();
	}

	if (tree_is(child_0, "FLOAT") && child_1 == NULL) {
		return type_float();
	}

	if (tree_is(child_0, "ID") && tree_is(child_1, "LP") &&
	    tree_is(child_2, "RP")) {
		Type *function_type = lookup_function_type(context, child_0);

		if (function_type == type_error()) {
			return type_error();
		}

		if (function_type->as.function.parameters != NULL) {
			report_error(context, 9, child_0->line,
			             "Function is not applicable for arguments");
			return type_error();
		}

		return function_type->as.function.return_type;
	}

	if (tree_is(child_0, "ID") && tree_is(child_1, "LP") &&
	    tree_is(child_2, "Args") && tree_is(child_3, "RP")) {
		Type *function_type = lookup_function_type(context, child_0);

		if (function_type == type_error()) {
			return type_error();
		}

		int has_error = 0;

		if (!arguments_match(context, child_2,
		                     function_type->as.function.parameters, &has_error)) {
			if (!has_error) {
				report_error(context, 9, child_0->line,
				             "Function is not applicable for arguments");
			}

			return type_error();
		}

		return function_type->as.function.return_type;
	}

	if (tree_is(child_0, "Exp") && tree_is(child_1, "LB") &&
	    tree_is(child_2, "Exp") && tree_is(child_3, "RB")) {
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
			report_error(context, 12, node->line, "Array index is not an integer");
			return type_error();
		}

		return array_type->as.array.element_type;
	}

	if (tree_is(child_0, "Exp") && tree_is(child_1, "DOT") &&
	    tree_is(child_2, "ID")) {
		Type *structure_type = analyze_exp(context, child_0);

		if (structure_type == type_error()) {
			return type_error();
		}

		if (structure_type->kind != TYPE_STRUCT) {
			report_error(context, 13, node->line, "Illegal use of \".\"");
			return type_error();
		}

		if (child_2->text == NULL) {
			return type_error();
		}

		const Field *field = struct_find_field(structure_type, child_2->text);

		if (field == NULL) {
			report_error(context, 14, child_2->line, "Non-existent field");
			return type_error();
		}

		return field->type;
	}

	if (tree_is(child_0, "LP") && tree_is(child_1, "Exp") &&
	    tree_is(child_2, "RP")) {
		return analyze_exp(context, child_1);
	}

	if ((tree_is(child_0, "MINUS") || tree_is(child_0, "NOT")) &&
	    tree_is(child_1, "Exp") && child_2 == NULL) {
		Type *operand_type = analyze_exp(context, child_1);

		if (operand_type == type_error()) {
			return type_error();
		}

		if (tree_is(child_0, "MINUS")) {
			if (!type_is_numeric(operand_type)) {
				report_error(context, 7, node->line, "Type mismatched for operands");
				return type_error();
			}

			return operand_type;
		}

		if (!type_equal(operand_type, type_int())) {
			report_error(context, 7, node->line, "Type mismatched for operands");
			return type_error();
		}

		return type_int();
	}

	if (tree_is(child_0, "Exp") && tree_is(child_1, "RELOP") &&
	    tree_is(child_2, "Exp")) {
		Type *left_type = analyze_exp(context, child_0);
		Type *right_type = analyze_exp(context, child_2);

		if (left_type == type_error() || right_type == type_error()) {
			return type_error();
		}

		if (!type_is_numeric(left_type) || !type_is_numeric(right_type) ||
		    !type_equal(left_type, right_type)) {
			report_error(context, 7, node->line, "Type mismatched for operands");
			return type_error();
		}

		return type_int();
	}

	if (tree_is(child_0, "Exp") &&
	    (tree_is(child_1, "AND") || tree_is(child_1, "OR")) &&
	    tree_is(child_2, "Exp")) {
		Type *left_type = analyze_exp(context, child_0);
		Type *right_type = analyze_exp(context, child_2);

		if (left_type == type_error() || right_type == type_error()) {
			return type_error();
		}

		if (!type_equal(left_type, type_int()) ||
		    !type_equal(right_type, type_int())) {
			report_error(context, 7, node->line, "Type mismatched for operands");
			return type_error();
		}

		return type_int();
	}

	if (tree_is(child_0, "Exp") && tree_is(child_1, "ASSIGNOP") &&
	    tree_is(child_2, "Exp")) {
		Type *left_type = analyze_exp(context, child_0);
		Type *right_type = analyze_exp(context, child_2);

		if (left_type == type_error() || right_type == type_error()) {
			return type_error();
		}

		if (!exp_is_lvalue(child_0)) {
			report_error(context, 6, node->line,
			             "The left-hand side of an assignment must be a variable");
			return type_error();
		}

		if (!type_equal(left_type, right_type)) {
			report_error(context, 5, node->line, "Type mismatched for assignment");
			return type_error();
		}

		return left_type;
	}

	if (tree_is(child_0, "Exp") && tree_is(child_2, "Exp") &&
	    (tree_is(child_1, "PLUS") || tree_is(child_1, "MINUS") ||
	     tree_is(child_1, "STAR") || tree_is(child_1, "DIV"))) {
		Type *left_type = analyze_exp(context, child_0);
		Type *right_type = analyze_exp(context, child_2);

		if (left_type == type_error() || right_type == type_error()) {
			return type_error();
		}

		if (!type_is_numeric(left_type) || !type_is_numeric(right_type) ||
		    !type_equal(left_type, right_type)) {
			report_error(context, 7, node->line, "Type mismatched for operands");
			return type_error();
		}

		return left_type;
	}

	return type_error();
}

static int exp_is_lvalue(const TreeNode *node) {
	if (node == NULL || !tree_is(node, "Exp")) {
		return 0;
	}

	const TreeNode *child_0 = tree_child_at(node, 0);
	const TreeNode *child_1 = tree_child_at(node, 1);
	const TreeNode *child_2 = tree_child_at(node, 2);
	const TreeNode *child_3 = tree_child_at(node, 3);

	if (tree_is(child_0, "ID") && child_1 == NULL) {
		return 1;
	}

	if (tree_is(child_0, "Exp") && tree_is(child_1, "LB") &&
	    tree_is(child_2, "Exp") && tree_is(child_3, "RB")) {
		return exp_is_lvalue(child_0);
	}

	return tree_is(child_0, "Exp") && tree_is(child_1, "DOT") &&
	       tree_is(child_2, "ID") && exp_is_lvalue(child_0);
}

static int arguments_match(SemanticContext *context, const TreeNode *node,
                           const Field *parameters, int *has_error) {
	if (context == NULL || node == NULL || parameters == NULL ||
	    has_error == NULL || !tree_is(node, "Args")) {
		return 0;
	}

	const TreeNode *argument_node = tree_child_at(node, 0);
	Type *argument_type = analyze_exp(context, argument_node);

	if (argument_type == type_error()) {
		*has_error = 1;
		return 0;
	}

	if (!type_equal(argument_type, parameters->type)) {
		return 0;
	}

	const TreeNode *child_1 = tree_child_at(node, 1);

	if (child_1 == NULL) {
		return parameters->next == NULL;
	}

	if (!tree_is(child_1, "COMMA")) {
		return 0;
	}

	return arguments_match(context, tree_child_at(node, 2), parameters->next,
	                       has_error);
}

static Type *lookup_function_type(SemanticContext *context,
                                  const TreeNode *id) {
	if (context == NULL || !tree_is(id, "ID") || id->text == NULL) {
		return type_error();
	}

	const Symbol *symbol = symbol_table_find(&context->symbols, id->text);

	if (symbol == NULL) {
		report_error(context, 2, id->line, "Undefined function");
		return type_error();
	}

	if (symbol->kind != SYMBOL_FUNCTION || symbol->type == NULL ||
	    symbol->type->kind != TYPE_FUNCTION) {
		report_error(context, 11, id->line, "Not a function");
		return type_error();
	}

	return symbol->type;
}

static const Field *struct_find_field(const Type *type, const char *name) {
	if (type == NULL || type->kind != TYPE_STRUCT || name == NULL) {
		return NULL;
	}

	return field_find(type->as.structure.fields, name);
}
