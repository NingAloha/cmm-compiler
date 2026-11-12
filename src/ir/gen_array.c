#include "generator_internal.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

void array_shape_clear(ArrayShape *shape) {
	if (shape == NULL) {
		return;
	}

	free(shape->dimensions);
	shape->dimensions = NULL;
	shape->dimension_count = 0;
	shape->id = NULL;
}

static int array_shape_add_dimension(ArrayShape *shape, size_t length) {
	if (shape == NULL || length == 0 ||
	    shape->dimension_count == SIZE_MAX / sizeof(*shape->dimensions)) {
		return 0;
	}

	size_t new_count = shape->dimension_count + 1;
	size_t *dimensions =
	    realloc(shape->dimensions, new_count * sizeof(*shape->dimensions));
	if (dimensions == NULL) {
		return 0;
	}

	shape->dimensions = dimensions;
	shape->dimensions[shape->dimension_count] = length;
	shape->dimension_count = new_count;
	return 1;
}

int parse_var_dec(const TreeNode *node, ArrayShape *shape) {
	const TreeNode *child_0 = tree_child_at(node, 0);

	if (node == NULL || shape == NULL || !tree_is(node, "VarDec")) {
		return 0;
	}
	if (tree_is(child_0, "ID") && tree_child_at(node, 1) == NULL) {
		shape->id = child_0;
		return child_0->text != NULL;
	}

	const TreeNode *length = tree_child_at(node, 2);
	if (!tree_is(child_0, "VarDec") || !tree_is(tree_child_at(node, 1), "LB") ||
	    !tree_is(length, "INT") || !tree_is(tree_child_at(node, 3), "RB") ||
	    tree_child_at(node, 4) != NULL || length->text == NULL ||
	    !parse_var_dec(child_0, shape)) {
		return 0;
	}

	errno = 0;
	char *end = NULL;
	unsigned long value = strtoul(length->text, &end, 10);
	return errno != ERANGE && end != length->text && *end == '\0' && value > 0 &&
	       value <= SIZE_MAX && array_shape_add_dimension(shape, (size_t)value);
}

typedef struct {
	const TreeNode *id;
	const TreeNode **indices;
	size_t index_count;
} ArrayAccess;

static void array_access_clear(ArrayAccess *access) {
	if (access == NULL) {
		return;
	}

	free(access->indices);
	access->indices = NULL;
	access->index_count = 0;
	access->id = NULL;
}

static int array_access_add_index(ArrayAccess *access, const TreeNode *index) {
	if (access == NULL || index == NULL ||
	    access->index_count == SIZE_MAX / sizeof(*access->indices)) {
		return 0;
	}

	size_t new_count = access->index_count + 1;
	const TreeNode **indices =
	    realloc(access->indices, new_count * sizeof(*access->indices));
	if (indices == NULL) {
		return 0;
	}

	access->indices = indices;
	access->indices[access->index_count] = index;
	access->index_count = new_count;
	return 1;
}

static int collect_array_access(const TreeNode *node, ArrayAccess *access) {
	const TreeNode *child_0 = tree_child_at(node, 0);

	if (node == NULL || access == NULL || !tree_is(node, "Exp")) {
		return 0;
	}
	if (tree_is(child_0, "LP") && tree_is(tree_child_at(node, 1), "Exp") &&
	    tree_is(tree_child_at(node, 2), "RP") && tree_child_at(node, 3) == NULL) {
		return collect_array_access(tree_child_at(node, 1), access);
	}
	if (tree_is(child_0, "ID") && tree_child_at(node, 1) == NULL) {
		access->id = child_0;
		return child_0->text != NULL;
	}

	const TreeNode *index = tree_child_at(node, 2);
	return tree_is(child_0, "Exp") && tree_is(tree_child_at(node, 1), "LB") &&
	       tree_is(index, "Exp") && tree_is(tree_child_at(node, 3), "RB") &&
	       tree_child_at(node, 4) == NULL &&
	       collect_array_access(child_0, access) &&
	       array_access_add_index(access, index);
}

static int array_stride(const IrVariable *variable, size_t index,
                        size_t *result) {
	if (variable == NULL || result == NULL ||
	    index >= variable->dimension_count) {
		return 0;
	}

	size_t stride = 4;
	for (size_t dimension = variable->dimension_count; dimension > index + 1;
	     dimension--) {
		size_t length = variable->dimensions[dimension - 1];
		if (length == 0 || stride > SIZE_MAX / length) {
			return 0;
		}
		stride *= length;
	}
	*result = stride;
	return 1;
}

int array_total_bytes(const ArrayShape *shape, size_t *result) {
	if (shape == NULL || result == NULL || shape->dimension_count == 0) {
		return 0;
	}

	size_t total = 4;
	for (size_t index = 0; index < shape->dimension_count; index++) {
		if (shape->dimensions[index] == 0 ||
		    total > SIZE_MAX / shape->dimensions[index]) {
			return 0;
		}
		total *= shape->dimensions[index];
	}
	*result = total;
	return 1;
}

int translate_array_address(IrGenerator *generator, const TreeNode *node,
                            char *result, size_t result_size,
                            size_t *remaining_dimensions) {
	ArrayAccess access = {0};
	const IrVariable *variable = NULL;
	char offset[OPERAND_CAPACITY];
	char address[OPERAND_CAPACITY];
	int succeeded = 0;

	if (!collect_array_access(node, &access) || access.id == NULL ||
	    (variable = variable_find(generator, access.id->text)) == NULL ||
	    variable->dimension_count == 0 || access.index_count == 0 ||
	    access.index_count > variable->dimension_count) {
		goto done;
	}

	for (size_t position = 0; position < access.index_count; position++) {
		char index[OPERAND_CAPACITY];
		char term[OPERAND_CAPACITY];
		size_t stride = 0;

		if (!translate_exp(generator, access.indices[position], index,
		                   sizeof(index)) ||
		    !array_stride(variable, position, &stride) ||
		    !ir_new_temp(&generator->ir, term, sizeof(term)) ||
		    !ir_emit(&generator->ir, "%s := %s * #%zu", term, index, stride)) {
			goto done;
		}

		if (position == 0) {
			if (!set_operand(generator, offset, sizeof(offset), term)) {
				goto done;
			}
		} else {
			char sum[OPERAND_CAPACITY];
			if (!ir_new_temp(&generator->ir, sum, sizeof(sum)) ||
			    !ir_emit(&generator->ir, "%s := %s + %s", sum, offset, term) ||
			    !set_operand(generator, offset, sizeof(offset), sum)) {
				goto done;
			}
		}
	}

	if (!ir_new_temp(&generator->ir, address, sizeof(address)) ||
	    !ir_emit(&generator->ir, "%s := %s%s + %s", address,
	             variable->is_array_parameter ? "" : "&", variable->ir_name,
	             offset) ||
	    !set_operand(generator, result, result_size, address)) {
		goto done;
	}
	if (remaining_dimensions != NULL) {
		*remaining_dimensions = variable->dimension_count - access.index_count;
	}
	succeeded = 1;

done:
	array_access_clear(&access);
	return succeeded ? 1 : generator_fail(generator);
}
