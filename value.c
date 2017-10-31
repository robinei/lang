#include "value.h"
#include <stdlib.h>

struct value value_true = { &type_bool, 1 };
struct value value_false = { &type_bool, 0 };

struct value *value_new_type(struct type *type) {
    struct value *result = calloc(1, sizeof(struct value));
    result->type = &type_type;
    result->u.type = type;
    return result;
}

struct value *value_new_bool(int val) {
    return val ? &value_true : &value_false;
}

struct value *value_new_int(int val) {
    struct value *result = calloc(1, sizeof(struct value));
    result->type = &type_int;
    result->u._int = val;
    return result;
}
