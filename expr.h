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
enum prim_kind { FOR_ALL_PRIMS(DECL_PRIM_ENUM) };
#undef DECL_PRIM_ENUM

extern const char *prim_names[];


#define FOR_ALL_EXPRS(X) \
    X(CONST) \
    X(SYM) \
    X(FN) \
    X(CAP) \
    X(LET) \
    X(STRUCT) \
    X(IF) \
    X(PRIM) \
    X(CALL)

#define DECL_EXPR_ENUM(name) EXPR_##name,
enum expr_kind { FOR_ALL_EXPRS(DECL_EXPR_ENUM) };
#undef DECL_EXPR_ENUM

extern const char *expr_names[];


struct expr_link {
    struct expr *expr;
    struct expr_link *next;
};

struct expr_decl {
    slice_t name;
    struct expr *type_expr;
    struct expr *value_expr;
    struct expr_decl *next;
};


struct expr_const {
    struct type *type;
    union {
        struct {
            struct expr *expr;
            struct expr *next; /* for chaining in the peval scope */
        } dummy_fn;
        struct {
            slice_t name; /* name of the function (in the module function table) */
            struct expr_decl *captured_consts;
        } fn;
        struct expr *expr;
        struct type *type;
        int _bool;
        int _int;
    } u;
};

struct expr_sym {
    slice_t name;
    struct expr *next; /* for chaining closure vars */
};

struct expr_fn {
    struct expr_decl *params;
    struct expr *return_type_expr;
    struct expr *body_expr; /* NULL for fn type declaration */
};

struct expr_cap {
    slice_t fn_name;
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
    struct expr *arg_exprs[2];
    enum prim_kind prim;
};

struct expr_call {
    struct expr *fn_expr;
    struct expr_link *args;
};

struct expr {
    union {
        struct expr_const _const;
        struct expr_sym sym;
        struct expr_fn fn;
        struct expr_cap cap;
        struct expr_let let;
        struct expr_struct _struct;
        struct expr_if _if;
        struct expr_prim prim;
        struct expr_call call;
    } u;

    slice_t source_text;
    struct expr *antecedent;

    enum expr_kind expr;
    bool is_closure_fn;
};



struct expr_visit_ctx;
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


static uint decl_list_length(struct expr_decl *decl) {
    uint count = 0;
    while (decl) {
        ++count;
        decl = decl->next;
    }
    return count;
}

static uint expr_list_length(struct expr_link *link) {
    uint count = 0;
    while (link) {
        ++count;
        link = link->next;
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
