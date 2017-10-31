#ifndef VALUE_H
#define VALUE_H

#include "type.h"

struct value {
    struct type *type;
    union {
        int _bool;
        int _int;
        struct type *type;
    } u;
};

struct value *value_new_type(struct type *type);
struct value *value_new_bool(int val);
struct value *value_new_int(int val);

extern struct value value_true;
extern struct value value_false;

#endif
