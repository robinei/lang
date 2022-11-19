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
    X(ASSIGN) \
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
    X(DOT) \
    X(ASSERT) \
    X(QUOTE) \
    X(SPLICE) \
    X(PRINT) \
    X(IMPORT) \
    X(STATIC)

#define DECL_PRIM_ENUM(name) PRIM_##name,
enum prim_kind { FOR_ALL_PRIMS(DECL_PRIM_ENUM) };
#undef DECL_PRIM_ENUM

extern const char *prim_names[];


#define FOR_ALL_EXPRS(X) \
    X(CONST) \
    X(SYM) \
    X(FUN) \
    X(DEF) \
    X(BLOCK) \
    X(STRUCT) \
    X(SELF) \
    X(COND) \
    X(PRIM) \
    X(CALL)

#define DECL_EXPR_ENUM(name) EXPR_##name,
enum expr_kind { FOR_ALL_EXPRS(DECL_EXPR_ENUM) };
#undef DECL_EXPR_ENUM

extern const char *expr_names[];

#define MAX_PARAM 128

struct fun_param {
    struct expr *name_expr;
    struct expr *type_expr;
    bool is_static;
};

struct expr {
    union {
        union {
            struct function *fun;
            struct expr *expr;
            struct type *type;
            bool boolean;
            int64_t integer;
            uint64_t uinteger;
            double real;
            slice_t string;
        } c; /* const */

        struct symbol *sym;

        struct {
            struct fun_param *params;
            struct expr *return_type_expr;
            struct expr *body_expr; /* NULL for fun type declaration */
        } fun;

        struct {
            struct expr *body_expr;
        } block;

        struct {
            struct expr *name_expr;
            struct expr *type_expr;
            struct expr *value_expr;
        } def;

        struct {
            struct expr *body_expr;
        } struc;

        struct {
            struct expr *pred_expr;
            struct expr *then_expr;
            struct expr *else_expr;
        } cond;

        struct {
            struct expr *arg_exprs[2];
            enum prim_kind kind;
        } prim;
        
        struct {
            struct expr *callable_expr;
            struct expr **args;
            uint arg_count;
        } call;
    };

    struct type *t;

    uint source_pos;

    bool def_is_static : 1;
    bool def_is_var : 1;
    uint fun_param_count : 14;
    enum expr_kind kind : 16;
};


struct expr_visit_ctx;
typedef void (*expr_visitor_t)(struct expr_visit_ctx *ctx, struct expr *e);

struct expr_visit_ctx {
    expr_visitor_t visitor;
    void *ctx;
    struct allocator *arena;
};

struct expr *expr_visit(struct expr_visit_ctx *ctx, struct expr *e);
void expr_visit_children(struct expr_visit_ctx *ctx, struct expr *e);
struct expr *expr_run_visitor(struct expr *e, expr_visitor_t visitor, void *ctx, struct allocator *arena);


void pretty_print_indented(struct expr *e, uint indent);
void pretty_print(struct expr *e);

#endif
