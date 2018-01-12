#ifndef EXPR_H
#define EXPR_H

#include "type.h"
#include "slice.h"


#define FOR_ALL_PRIMS(X) \
    X(PLUS) \
    X(NEGATE) \
    X(LOGI_NOT) \
    X(BITWISE_NOT) \
    X(SEQ) \
    X(LOGI_OR) \
    X(LOGI_AND) \
    X(BITWISE_OR) \
    X(BITWISE_XOR) \
    X(BITWISE_AND) \
    X(EQ) \
    X(NEQ) \
    X(LT) \
    X(GT) \
    X(LTEQ) \
    X(GTEQ) \
    X(BITWISE_LSH) \
    X(BITWISE_RSH) \
    X(ADD) \
    X(SUB) \
    X(MUL) \
    X(DIV) \
    X(MOD) \
    X(ASSERT) \
    X(QUOTE) \
    X(SPLICE) \
    X(PRINT)

#define DECL_PRIM_ENUM(name) PRIM_##name,
enum {
    FOR_ALL_PRIMS(DECL_PRIM_ENUM)
};
#undef DECL_PRIM_ENUM

extern const char *prim_names[];


#define FOR_ALL_EXPRS(X) \
    X(CONST) \
    X(SYM) \
    X(FN) \
    X(LET) \
    X(STRUCT) \
    X(IF) \
    X(PRIM) \
    X(CALL)

#define DECL_EXPR_ENUM(name) EXPR_##name,
enum {
    FOR_ALL_EXPRS(DECL_EXPR_ENUM)
};
#undef DECL_EXPR_ENUM

extern const char *expr_names[];


struct expr_decl {
    slice_t name;
    struct expr *type_expr;
    struct expr *value_expr;
    struct expr_decl *next;
};

struct expr_const {
    struct type *type;
    union {
        struct expr *expr;
        struct type *type;
        int _bool;
        int _int;
        slice_t fn_name;
    } u;
};

struct expr_sym {
    slice_t name;
};

struct expr_fn {
    struct expr_decl *params;
    struct expr *return_type_expr;
    struct expr *body_expr; /* NULL for fn type declaration */
};

struct expr_let {
    struct expr_decl *bindings;
    struct expr *body_expr;
};

struct expr_struct {
    struct expr_decl *fields;
};

struct expr_if {
    struct expr *cond_expr;
    struct expr *then_expr;
    struct expr *else_expr;
};

struct expr_prim {
    uint prim;
    struct expr *arg_exprs[2];
};

struct expr_call {
    struct expr *fn_expr;
    struct expr_call_arg *args;
};
struct expr_call_arg {
    struct expr *expr;
    struct expr_call_arg *next;
};

struct expr {
    uint expr;
    slice_t source_text;
    struct expr *antecedent;

    union {
        struct expr_const _const;
        struct expr_sym sym;
        struct expr_fn fn;
        struct expr_let let;
        struct expr_struct _struct;
        struct expr_if _if;
        struct expr_prim prim;
        struct expr_call call;
    } u;
};



typedef void (*expr_visitor_t)(struct expr_visit_ctx *ctx, struct expr *e);
struct expr_visit_ctx {
    expr_visitor_t visitor;
    void *ctx;
};
struct expr *expr_visit(struct expr_visit_ctx *ctx, struct expr *e);
void expr_visit_children(struct expr_visit_ctx *ctx, struct expr *e);



struct print_ctx {
    uint indent;
};

void print_expr(struct print_ctx *ctx, struct expr *e);


static uint decl_count(struct expr_decl *decl) {
    uint count = 0;
    while (decl) {
        ++count;
        decl = decl->next;
    }
    return count;
}

static uint calc_arg_count(struct expr_call_arg *arg) {
    uint count = 0;
    while (arg) {
        ++count;
        arg = arg->next;
    }
    return count;
}

static slice_t expr_source_text(struct expr *e) {
    slice_t empty = { "", 0 };
    do {
        if (e->source_text.ptr) {
            return e->source_text;
        }
        e = e->antecedent;
    } while (e);
    return empty;
}

#endif
