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
    X(PRINT) \
    X(STATIC)

#define DECL_PRIM_ENUM(name) PRIM_##name,
enum prim_kind { FOR_ALL_PRIMS(DECL_PRIM_ENUM) };
#undef DECL_PRIM_ENUM

extern const char *prim_names[];


#define FOR_ALL_EXPRS(X) \
    X(CONST) \
    X(SYM) \
    X(FUN) \
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
    struct expr *name_expr;
    struct expr *type_expr;
    struct expr *value_expr;
    struct expr_decl *next;
    bool is_static : 1;
    bool is_mut : 1;
};

struct expr {
    union {
        struct {
            union {
                struct {
                    struct function *func;
                    struct expr_decl *captured_consts;
                } fun;
                struct expr *expr;
                struct type *typeval;
                int _bool;
                int _int;
            };
            struct type *type;
        } c; /* const */

        struct {
            slice_t name;
            uint name_hash;
        } sym;

        struct {
            struct expr_decl *params;
            struct expr *return_type_expr;
            struct expr *body_expr; /* NULL for fun type declaration */
        } fun;

        struct {
            struct function *func;
        } cap;

        struct {
            struct expr_decl *bindings;
            struct expr *body_expr;
        } let;

        struct {
            struct expr_decl *fields;
        } struc;

        struct {
            struct expr *cond_expr;
            struct expr *then_expr;
            struct expr *else_expr;
        } _if;

        struct {
            struct expr *arg_exprs[2];
            enum prim_kind prim;
        } prim;
        
        struct {
            struct expr *callable_expr;
            struct expr_link *args;
        } call;
    };

    slice_t source_text;
    struct expr *antecedent;

    enum expr_kind expr;
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
    while (e) {
        if (e->source_text.ptr) {
            return e->source_text;
        }
        e = e->antecedent;
    }
    return empty;
}

#endif
