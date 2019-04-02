#ifndef TYPE_H
#define TYPE_H

#include "defs.h"
#include "slice.h"

#define FOR_ALL_TYPES(X) \
    X(EXPR) \
    X(TYPE) \
    X(UNIT) \
    X(BOOL) \
    X(INT) \
    X(FUN) \
    X(STRUCT)

#define DECL_TYPE_ENUM(name) TYPE_##name,
enum type_kind {
    FOR_ALL_TYPES(DECL_TYPE_ENUM)
};
#undef DECL_TYPE_ENUM

extern const char *type_names[];

struct type_attr {
    struct symbol *name;
    struct expr *value_expr;
    struct type_attr *next;
};

struct type {
    struct type_attr *attrs;
    enum type_kind kind;
};

extern struct type type_expr;
extern struct type type_type;
extern struct type type_unit;
extern struct type type_bool;
extern struct type type_int;
extern struct type type_fun; /* TODO: replace with fun types actually having structure, and which are different */

#endif
