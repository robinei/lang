#include "type.h"
#include <stdlib.h>

struct type type_expr = { .kind = TYPE_EXPR };
struct type type_type = { .kind = TYPE_TYPE };
struct type type_unit = { .kind = TYPE_UNIT };
struct type type_bool = { .kind = TYPE_BOOL };
struct type type_int = { .kind = TYPE_INT };
struct type type_fun = { .kind = TYPE_FUN };

#define DECL_TYPE_NAME(name) #name,
const char *type_names[] = {
    FOR_ALL_TYPES(DECL_TYPE_NAME)
};

struct type *create_struct_type(uint field_count) {
    struct type *type = calloc(1, sizeof(struct type));
    type->kind = TYPE_STRUCT;
    type->struc.field_count = field_count;
    type->struc.fields = calloc(1, sizeof(struct struct_field) * field_count);
    return type;
}