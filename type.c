#include "expr.h"
#include <stdlib.h>
#include <assert.h>

void type_set_attr(struct type *type, struct symbol *name, struct expr *val) {
    assert(val->kind == EXPR_CONST);
    pointer_table_put(&type->attrs, name, val);
}

struct expr *type_get_attr(struct type *type, struct symbol *name) {
    struct expr *result = NULL;
    pointer_table_get(&type->attrs, name, (void **)&result);
    return result;
}

struct type type_expr = { .kind = TYPE_EXPR };
struct type type_type = { .kind = TYPE_TYPE };
struct type type_unit = { .kind = TYPE_UNIT };
struct type type_bool = { .kind = TYPE_BOOL };
struct type type_int = { .kind = TYPE_INT };
struct type type_uint = { .kind = TYPE_UINT };
struct type type_real = { .kind = TYPE_REAL };
struct type type_string = { .kind = TYPE_STRING };
struct type type_fun = { .kind = TYPE_FUN };

#define DECL_TYPE_NAME(name) #name,
const char *type_names[] = {
    FOR_ALL_TYPES(DECL_TYPE_NAME)
};

#define EXPAND_IMPLEMENTATION
#define NAME        pointer_table
#define KEY_TYPE    void *
#define VALUE_TYPE  void *
#define HASH_FUNC   hashutil_ptr_hash
#define EQUAL_FUNC(a, b) ((a) == (b))
#include "hashtable.h"
