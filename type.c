#include "type.h"

struct type type_type = { TYPE_TYPE };
struct type type_unit = { TYPE_UNIT };
struct type type_bool = { TYPE_BOOL };
struct type type_int = { TYPE_INT };
struct type type_fn = { TYPE_FN };

#define DECL_TYPE_NAME(name) #name,
const char *type_names[] = {
    FOR_ALL_TYPES(DECL_TYPE_NAME)
};
