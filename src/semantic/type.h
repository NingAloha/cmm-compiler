#ifndef SEMANTIC_TYPE_H
#define SEMANTIC_TYPE_H

#include <stddef.h>

typedef enum TypeKind {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_FUNCTION,
    TYPE_ERROR
} TypeKind;

typedef struct Type Type;
typedef struct Field Field;

struct Field {
    char *name;
    Type *type;
    int line;
    Field *next;
};

struct Type {
    TypeKind kind;

    union {
        struct {
            Type *element_type;
            size_t length;
        } array;

        struct {
            char *tag;
            Field *fields;
        } structure;

        struct {
            Type *return_type;
            Field *parameters;
        } function;
    } as;
};

/* Shared primitive types */

Type *type_int(void);
Type *type_float(void);
Type *type_error(void);

/* Type and field construction */

Type *type_new_array(Type *element_type, size_t length);
Type *type_new_structure(const char *tag, Field *fields);
Type *type_new_function(Type *return_type, Field *parameters);

Field *field_new(const char *name, Type *type, int line);
void field_append(Field **head, Field *field);
const Field *field_find(const Field *fields, const char *name);

/* Type predicates and comparison */

int type_equal(const Type *left, const Type *right);
int type_is_numeric(const Type *type);

#endif
