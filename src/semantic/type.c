#include "type.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static Type int_type = {.kind = TYPE_INT};
static Type float_type = {.kind = TYPE_FLOAT};
static Type error_type = {.kind = TYPE_ERROR};

static char *copy_string(const char *source) {
    if (source == NULL) {
        return NULL;
    }

    size_t size = strlen(source) + 1;
    char *copy = malloc(size);

    if (copy != NULL) {
        memcpy(copy, source, size);
    }

    return copy;
}

Type *type_int(void) { return &int_type; }

Type *type_float(void) { return &float_type; }

Type *type_error(void) { return &error_type; }

Type *type_new_array(Type *element_type, size_t length) {
    Type *type = malloc(sizeof(*type));

    if (type == NULL) {
        return NULL;
    }

    type->kind = TYPE_ARRAY;
    type->as.array.element_type = element_type;
    type->as.array.length = length;
    return type;
}

Type *type_new_structure(const char *tag, Field *fields) {
    Type *type = malloc(sizeof(*type));

    if (type == NULL) {
        return NULL;
    }

    type->kind = TYPE_STRUCT;
    type->as.structure.tag = copy_string(tag);
    if (tag != NULL && type->as.structure.tag == NULL) {
        free(type);
        return NULL;
    }

    type->as.structure.fields = fields;
    return type;
}

Type *type_new_function(Type *return_type, Field *parameters) {
    Type *type = malloc(sizeof(*type));

    if (type == NULL) {
        return NULL;
    }

    type->kind = TYPE_FUNCTION;
    type->as.function.return_type = return_type;
    type->as.function.parameters = parameters;
    return type;
}

Field *field_new(const char *name, Type *type, int line) {
    Field *field = malloc(sizeof(*field));

    if (field == NULL) {
        return NULL;
    }

    field->name = copy_string(name);
    if (name != NULL && field->name == NULL) {
        free(field);
        return NULL;
    }

    field->type = type;
    field->line = line;
    field->next = NULL;
    return field;
}

void field_append(Field **head, Field *field) {
    Field *tail;

    if (head == NULL || field == NULL) {
        return;
    }

    if (*head == NULL) {
        *head = field;
        return;
    }

    tail = *head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = field;
}

static int field_list_equal(const Field *left, const Field *right) {
    while (left != NULL && right != NULL) {
        if (!type_equal(left->type, right->type)) {
            return 0;
        }
        left = left->next;
        right = right->next;
    }

    return left == NULL && right == NULL;
}

int type_equal(const Type *left, const Type *right) {
    if (left == NULL || right == NULL) {
        return 0;
    }

    if (left->kind == TYPE_ERROR || right->kind == TYPE_ERROR) {
        return 1;
    }
    if (left->kind != right->kind) {
        return 0;
    }

    switch (left->kind) {
    case TYPE_INT:
    case TYPE_FLOAT:
        return 1;

    case TYPE_ARRAY:
        return type_equal(left->as.array.element_type,
                          right->as.array.element_type);

    case TYPE_STRUCT:
        return left == right;

    case TYPE_FUNCTION:
        return type_equal(left->as.function.return_type,
                          right->as.function.return_type) &&
               field_list_equal(left->as.function.parameters,
                                right->as.function.parameters);

    case TYPE_ERROR:
        return 1;
    }

    return 0;
}

int type_is_numeric(const Type *type) {
    return type != NULL && (type->kind == TYPE_INT || type->kind == TYPE_FLOAT);
}
