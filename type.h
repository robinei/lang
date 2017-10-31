#ifndef TYPE_H
#define TYPE_H

#include "defs.h"
#include "slice.h"

#define FOR_ALL_TYPES(X) \
    X(TYPE) \
    X(BOOL) \
    X(INT) \
    X(FN) \
    X(STRUCT)

#define DECL_TYPE_ENUM(name) TYPE_##name,
enum type_enum {
    FOR_ALL_TYPES(DECL_TYPE_ENUM)
};
#undef DECL_TYPE_ENUM

extern const char *type_names[];

struct type_fn {
    uint param_count;
    struct type_fn_param *params;
    struct type *return_type;
};
struct type_fn_param {
    slice_t name;
    struct type *type;
    struct type_fn_param *next;
};

struct type_struct {
    uint field_count;
    struct type_struct_field *fields;
};
struct type_struct_field {
    slice_t name;
    struct type *type;
    struct type_struct_field *next;
};

struct type {
    enum type_enum type;
    union {
        struct type_fn fn;
        struct type_struct _struct;
    } u;
};

extern struct type type_type;
extern struct type type_bool;
extern struct type type_int;

#endif
