#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define DECL_PRIM_NAME(name) #name,
const char *prim_names[] = {
    FOR_ALL_PRIMS(DECL_PRIM_NAME)
};

#define DECL_EXPR_NAME(name) #name,
const char *expr_names[] = {
    FOR_ALL_EXPRS(DECL_EXPR_NAME)
};


struct expr expr_unit = { EXPR_CONST, { { &type_unit } } };
struct expr expr_true = { EXPR_CONST, { { &type_bool, { 1 } } } };
struct expr expr_false = { EXPR_CONST, { { &type_bool, { 0 } } } };


static void print(struct print_ctx *ctx, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

static void print_indent(struct print_ctx *ctx) {
    uint i;
    for (i = 0; i < ctx->indent * 4; ++i) {
        putchar(' ');
    }
}

static void print_unop(struct print_ctx *ctx, const char *op, struct expr *e) {
    print(ctx, "%s", op);
    print_expr(ctx, e->u.prim.arg_exprs[0]);
}

static void print_binop(struct print_ctx *ctx, const char *op, struct expr *e) {
    print_expr(ctx, e->u.prim.arg_exprs[0]);
    print(ctx, "%s", op);
    print_expr(ctx, e->u.prim.arg_exprs[1]);
}

void print_expr(struct print_ctx *ctx, struct expr *e) {
    struct print_ctx ctx_storage;
    if (!ctx) {
        ctx = &ctx_storage;
        ctx->indent = 0;
    }
    if (!e) {
        return;
    }
    switch (e->expr) {
    case EXPR_CONST:
        switch (e->u._const.type->type) {
        case TYPE_TYPE:
            switch (e->u._const.u.type->type) {
            case TYPE_TYPE: print(ctx, "Type"); break;
            case TYPE_UNIT: print(ctx, "Unit"); break;
            case TYPE_BOOL: print(ctx, "Bool"); break;
            case TYPE_INT: print(ctx, "Int"); break;
            default:
                print(ctx, "type:%s", type_names[e->u._const.u.type->type]); break;
            }
            break;
        case TYPE_BOOL:
            print(ctx, "%s", e->u._const.u._bool ? "true" : "false");
            break;
        case TYPE_UNIT:
            print(ctx, "()");
            break;
        case TYPE_INT:
            print(ctx, "%d", e->u._const.u._int);
            break;
        case TYPE_FN:
            print(ctx, "<%.*s>", e->u._const.u.fn_name.len, e->u._const.u.fn_name.ptr);
            break;
        default:
            print(ctx, "const:%s", type_names[e->u._const.type->type]);
            break;
        }
        break;
    case EXPR_SYM:
        print(ctx, "%.*s", e->u.sym.name.len, e->u.sym.name.ptr);
        break;
    case EXPR_FN: {
        struct expr_decl *p;
        print(ctx, "fn(");
        for (p = e->u.fn.params; p; p = p->next) {
            print(ctx, "%.*s", p->name.len, p->name.ptr);
            if (p->type_expr) {
                print(ctx, ": ");
                print_expr(ctx, p->type_expr);
            }
            if (p->next) {
                print(ctx, "; ");
            }
        }
        print(ctx, ")");
        if (e->u.fn.return_type_expr) {
            print(ctx, " ");
            print_expr(ctx, e->u.fn.return_type_expr);
        }
        print(ctx, ": ");
        print_expr(ctx, e->u.fn.body_expr);
        break;
    }
    case EXPR_LET: {
        struct expr_decl *b;
        print(ctx, "let ");
        for (b = e->u.let.bindings; b; b = b->next) {
            print(ctx, "%.*s", b->name.len, b->name.ptr);
            if (b->type_expr) {
                print(ctx, ": ");
                print_expr(ctx, b->type_expr);
            }
            print(ctx, " = ");
            print_expr(ctx, b->value_expr);
            if (b->next) {
                print(ctx, "; ");
            }
        }
        break;
    }
    case EXPR_STRUCT: {
        struct expr_decl *f;
        print(ctx, "struct\n");
        ++ctx->indent;
        for (f = e->u._struct.fields; f; f = f->next) {
            print_indent(ctx);
            print(ctx, "%.*s", f->name.len, f->name.ptr);
            if (f->type_expr) {
                print(ctx, ": ");
                print_expr(ctx, f->type_expr);
            }
            if (f->value_expr) {
                print(ctx, " = ");
                print_expr(ctx, f->value_expr);
            }
            print(ctx, ";");
            if (f->next) {
                print(ctx, "\n");
            }
        }
        print(ctx, ";");
        --ctx->indent;
        break;
    }
    case EXPR_IF: {
        print(ctx, "if ");
        print_expr(ctx, e->u._if.cond_expr);
        print(ctx, ": ");
        print_expr(ctx, e->u._if.then_expr);
        print(ctx, " else ");
        print_expr(ctx, e->u._if.else_expr);
        break;
    }
    case EXPR_PRIM: {
        switch (e->u.prim.prim) {
        case PRIM_PLUS: print_unop(ctx, "+", e); break;
        case PRIM_NEGATE: print_unop(ctx, "-", e); break;
        case PRIM_LOGI_NOT: print_unop(ctx, "!", e); break;
        case PRIM_BITWISE_NOT: print_unop(ctx, "~", e); break;
        case PRIM_SEQ: print_binop(ctx, " ", e); break;
        case PRIM_LOGI_OR: print_binop(ctx, " || ", e); break;
        case PRIM_LOGI_AND: print_binop(ctx, " && ", e); break;
        case PRIM_BITWISE_OR: print_binop(ctx, " | ", e); break;
        case PRIM_BITWISE_XOR: print_binop(ctx, " ^ ", e); break;
        case PRIM_BITWISE_AND: print_binop(ctx, " & ", e); break;
        case PRIM_EQ: print_binop(ctx, " == ", e); break;
        case PRIM_NEQ: print_binop(ctx, " != ", e); break;
        case PRIM_LT: print_binop(ctx, " < ", e); break;
        case PRIM_GT: print_binop(ctx, " > ", e); break;
        case PRIM_LTEQ: print_binop(ctx, " <= ", e); break;
        case PRIM_GTEQ: print_binop(ctx, " >= ", e); break;
        case PRIM_BITWISE_LSH: print_binop(ctx, " << ", e); break;
        case PRIM_BITWISE_RSH: print_binop(ctx, " >> ", e); break;
        case PRIM_ADD: print_binop(ctx, " + ", e); break;
        case PRIM_SUB: print_binop(ctx, " - ", e); break;
        case PRIM_MUL: print_binop(ctx, " * ", e); break;
        case PRIM_DIV: print_binop(ctx, " / ", e); break;
        case PRIM_MOD: print_binop(ctx, " % ", e); break;
        }
        break;
    }
    case EXPR_CALL: {
        struct expr_call_arg *arg;
        print_expr(ctx, e->u.call.fn_expr);
        print(ctx, "(");
        for (arg = e->u.call.args; arg; arg = arg->next) {
            print_expr(ctx, arg->expr);
            if (arg->next) {
                print(ctx, ", ");
            }
        }
        print(ctx, ")");
        break;
    }
    }
}
