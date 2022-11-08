#ifndef TYPE_H
#define TYPE_H

#include "sym.h"

#define FOR_ALL_TYPES(X) \
    X(ANY) \
    X(EXPR) \
    X(TYPE) \
    X(UNIT) \
    X(BOOL) \
    X(INT) \
    X(UINT) \
    X(REAL) \
    X(STRING) \
    X(FUN) \
    X(STRUCT)

#define DECL_TYPE_ENUM(name) TYPE_##name,
enum type_kind {
    FOR_ALL_TYPES(DECL_TYPE_ENUM)
};
#undef DECL_TYPE_ENUM

extern const char *type_names[];

struct expr;

#define TYPEATTR_HASHTABLE(X) X(typeattr_hashtable, struct symbol *, struct expr *, calc_ptr_hash, VALUE_EQ)
DECLARE_HASHTABLE(TYPEATTR_HASHTABLE)

struct type {
    struct typeattr_hashtable attrs;
    enum type_kind kind;
};

void type_set_attr(struct type *type, struct symbol *name, struct expr *val);
struct expr *type_get_attr(struct type *type, struct symbol *name);

extern struct type type_any;
extern struct type type_expr;
extern struct type type_type;
extern struct type type_unit;
extern struct type type_bool;
extern struct type type_int;
extern struct type type_uint;
extern struct type type_real;
extern struct type type_string;
extern struct type type_fun; /* TODO: replace with fun types actually having structure, and which are different */

#endif
