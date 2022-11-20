#ifndef EXPR_H
#define EXPR_H

#include "type.h"
#include "slice.h"


enum prim_kind {
    PRIM_PLUS,
    PRIM_NEGATE,
    PRIM_LOGI_NOT,
    PRIM_BITWISE_NOT,
    PRIM_ASSIGN,
    PRIM_LOGI_OR,
    PRIM_LOGI_AND,
    PRIM_BITWISE_OR,
    PRIM_BITWISE_XOR,
    PRIM_BITWISE_AND,
    PRIM_EQ,
    PRIM_NEQ,
    PRIM_LT,
    PRIM_GT,
    PRIM_LTEQ,
    PRIM_GTEQ,
    PRIM_BITWISE_LSH,
    PRIM_BITWISE_RSH,
    PRIM_ADD,
    PRIM_SUB,
    PRIM_MUL,
    PRIM_DIV,
    PRIM_MOD,
    PRIM_DOT,
    PRIM_ASSERT,
    PRIM_QUOTE,
    PRIM_SPLICE,
    PRIM_PRINT,
    PRIM_IMPORT,
    PRIM_STATIC,
};


enum expr_kind {
    EXPR_CONST,
    EXPR_SYM,
    EXPR_FUN,
    EXPR_DEF,
    EXPR_BLOCK,
    EXPR_STRUCT,
    EXPR_SELF,
    EXPR_COND,
    EXPR_CALL,
    EXPR_PRIM,
};


#define MAX_PARAM 128
#define MAX_BLOCK 1024

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
            struct expr **exprs;
            uint expr_count;
            bool creates_scope;
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
