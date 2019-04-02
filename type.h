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

#define EXPAND_INTERFACE
#define NAME        pointer_table
#define KEY_TYPE    void *
#define VALUE_TYPE  void *
#include "hashtable.h"

struct expr;

struct symbol {
    uint length;
    char data[0];
};

struct type {
    struct pointer_table attrs;
    enum type_kind kind;
};

void type_set_attr(struct type *type, struct symbol *name, struct expr *val);
struct expr *type_get_attr(struct type *type, struct symbol *name);

extern struct type type_expr;
extern struct type type_type;
extern struct type type_unit;
extern struct type type_bool;
extern struct type type_int;
extern struct type type_fun; /* TODO: replace with fun types actually having structure, and which are different */

#endif
