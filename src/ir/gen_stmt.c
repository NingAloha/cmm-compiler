#include "generator_internal.h"

static int translate_stmt(IrGenerator *generator, const TreeNode *node) {
	const TreeNode *child_0 = tree_child_at(node, 0);
	if (!tree_is(node, "Stmt")) {
		return generator_fail(generator);
	}
	if (tree_is(child_0, "Exp") && tree_is(tree_child_at(node, 1), "SEMI")) {
		char unused[OPERAND_CAPACITY];
		return translate_exp(generator, child_0, unused, sizeof(unused));
	}
	if (tree_is(child_0, "RETURN") && tree_is(tree_child_at(node, 1), "Exp") &&
	    tree_is(tree_child_at(node, 2), "SEMI")) {
		char value[OPERAND_CAPACITY];
		if (!translate_exp(generator, tree_child_at(node, 1), value,
		                   sizeof(value)) ||
		    !ir_emit(&generator->ir, "RETURN %s", value)) {
			return generator_fail(generator);
		}
		return 1;
	}
	if (tree_is(child_0, "CompSt")) {
		return translate_comp_st(generator, child_0);
	}
	if (tree_is(child_0, "IF") && tree_is(tree_child_at(node, 1), "LP") &&
	    tree_is(tree_child_at(node, 2), "Exp") &&
	    tree_is(tree_child_at(node, 3), "RP") &&
	    tree_is(tree_child_at(node, 4), "Stmt")) {
		char true_label[OPERAND_CAPACITY];
		char false_label[OPERAND_CAPACITY];
		char end_label[OPERAND_CAPACITY];
		if (!ir_new_label(&generator->ir, true_label, sizeof(true_label)) ||
		    !ir_new_label(&generator->ir, false_label, sizeof(false_label)) ||
		    !translate_cond(generator, tree_child_at(node, 2), true_label,
		                    false_label) ||
		    !ir_emit(&generator->ir, "LABEL %s :", true_label) ||
		    !translate_stmt(generator, tree_child_at(node, 4))) {
			return generator_fail(generator);
		}
		if (tree_child_at(node, 5) == NULL) {
			return ir_emit(&generator->ir, "LABEL %s :", false_label)
			           ? 1
								 : generator_fail(generator);
		}
		if (!tree_is(tree_child_at(node, 5), "ELSE") ||
		    !tree_is(tree_child_at(node, 6), "Stmt") ||
		    tree_child_at(node, 7) != NULL ||
		    !ir_new_label(&generator->ir, end_label, sizeof(end_label)) ||
		    !ir_emit(&generator->ir, "GOTO %s", end_label) ||
		    !ir_emit(&generator->ir, "LABEL %s :", false_label) ||
		    !translate_stmt(generator, tree_child_at(node, 6)) ||
		    !ir_emit(&generator->ir, "LABEL %s :", end_label)) {
			return generator_fail(generator);
		}
		return 1;
	}
	if (tree_is(child_0, "WHILE") && tree_is(tree_child_at(node, 1), "LP") &&
	    tree_is(tree_child_at(node, 2), "Exp") &&
	    tree_is(tree_child_at(node, 3), "RP") &&
	    tree_is(tree_child_at(node, 4), "Stmt") &&
	    tree_child_at(node, 5) == NULL) {
		char begin_label[OPERAND_CAPACITY];
		char true_label[OPERAND_CAPACITY];
		char false_label[OPERAND_CAPACITY];
		if (!ir_new_label(&generator->ir, begin_label, sizeof(begin_label)) ||
		    !ir_new_label(&generator->ir, true_label, sizeof(true_label)) ||
		    !ir_new_label(&generator->ir, false_label, sizeof(false_label)) ||
		    !ir_emit(&generator->ir, "LABEL %s :", begin_label) ||
		    !translate_cond(generator, tree_child_at(node, 2), true_label,
		                    false_label) ||
		    !ir_emit(&generator->ir, "LABEL %s :", true_label) ||
		    !translate_stmt(generator, tree_child_at(node, 4)) ||
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
	if (!tree_is(node, "StmtList") ||
	    !translate_stmt(generator, tree_child_at(node, 0))) {
		return generator_fail(generator);
	}
	return translate_stmt_list(generator, tree_child_at(node, 1));
}

int translate_comp_st(IrGenerator *generator, const TreeNode *node) {
	if (!tree_is(node, "CompSt") ||
	    !translate_def_list(generator, tree_find_child(node, "DefList")) ||
	    !translate_stmt_list(generator, tree_find_child(node, "StmtList"))) {
		return generator_fail(generator);
	}
	return 1;
}
