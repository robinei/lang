#include "expr.h"
#include "mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define DECL_PRIM_NAME(name) #name,
const char *prim_names[] = {
    FOR_ALL_PRIMS(DECL_PRIM_NAME)
};

#define DECL_EXPR_NAME(name) #name,
const char *expr_names[] = {
    FOR_ALL_EXPRS(DECL_EXPR_NAME)
};



struct expr *expr_visit(struct expr_visit_ctx *ctx, struct expr *e) {
    struct expr e_new;
    if (!e) {
        return NULL;
    }
    e_new = *e;
    ctx->visitor(ctx, &e_new);
    if (memcmp(e, &e_new, sizeof(e_new))) {
        /* TODO: don't duplicate this (from dup_expr) */
        struct expr *e_copy = malloc(sizeof(struct expr));
        *e_copy = e_new;
        e_copy->antecedent = e;
        e_copy->source_text.ptr = NULL;
        e_copy->source_text.len = 0;
        return e_copy;
    }
    return e;
}

static struct expr_decl *expr_decl_visit(struct expr_visit_ctx *ctx, struct expr_decl *decl) {
    struct expr_decl decl_new;
    if (!decl) {
        return NULL;
    }
    decl_new = *decl;
    decl_new.next = expr_decl_visit(ctx, decl_new.next);
    decl_new.type_expr = expr_visit(ctx, decl_new.type_expr);
    decl_new.value_expr = expr_visit(ctx, decl_new.value_expr);
    if (memcmp(&decl_new, decl, sizeof(decl_new))) {
        struct expr_decl *decl_copy = malloc(sizeof(struct expr_decl));
        *decl_copy = decl_new;
        return decl_copy;
    }
    return decl;
}

static struct expr_link *expr_call_arg_visit(struct expr_visit_ctx *ctx, struct expr_link *arg) {
    struct expr_link arg_new;
    if (!arg) {
        return NULL;
    }
    arg_new = *arg;
    arg_new.next = expr_call_arg_visit(ctx, arg_new.next);
    arg_new.expr = expr_visit(ctx, arg_new.expr);
    if (memcmp(&arg_new, arg, sizeof(arg_new))) {
        struct expr_link *arg_copy = malloc(sizeof(struct expr_link));
        *arg_copy = arg_new;
        return arg_copy;
    }
    return arg;
}

void expr_visit_children(struct expr_visit_ctx *ctx, struct expr *e) {
    switch (e->expr) {
    case EXPR_CONST:
    case EXPR_SYM:
        break;
    case EXPR_FUN:
        e->u.fun.params = expr_decl_visit(ctx, e->u.fun.params);
        e->u.fun.return_type_expr = expr_visit(ctx, e->u.fun.return_type_expr);
        e->u.fun.body_expr = expr_visit(ctx, e->u.fun.body_expr);
        break;
    case EXPR_LET:
        e->u.let.bindings = expr_decl_visit(ctx, e->u.let.bindings);
        e->u.let.body_expr = expr_visit(ctx, e->u.let.body_expr);
        break;
    case EXPR_STRUCT:
        e->u._struct.fields = expr_decl_visit(ctx, e->u._struct.fields);
        break;
    case EXPR_IF:
        e->u._if.cond_expr = expr_visit(ctx, e->u._if.cond_expr);
        e->u._if.then_expr = expr_visit(ctx, e->u._if.then_expr);
        e->u._if.else_expr = expr_visit(ctx, e->u._if.else_expr);
        break;
    case EXPR_PRIM:
        e->u.prim.arg_exprs[0] = expr_visit(ctx, e->u.prim.arg_exprs[0]);
        e->u.prim.arg_exprs[1] = expr_visit(ctx, e->u.prim.arg_exprs[1]);
        break;
    case EXPR_CALL:
        e->u.call.callable_expr = expr_visit(ctx, e->u.call.callable_expr);
        e->u.call.args = expr_call_arg_visit(ctx, e->u.call.args);
        break;
    default:
        assert(NULL && "unexpected expr type");
        break;
    }
}


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

static void print_primcall(struct print_ctx *ctx, const char *name, struct expr *e) {
    print(ctx, "%s(", name);
    if (e->u.prim.arg_exprs[0]) {
        print_expr(ctx, e->u.prim.arg_exprs[0]);
    }
    if (e->u.prim.arg_exprs[1]) {
        print(ctx, ", ");
        print_expr(ctx, e->u.prim.arg_exprs[1]);
    }
    print(ctx, ")");
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
        case TYPE_EXPR:
            printf("Expr<");
            print_expr(ctx, e->u._const.u.expr);
            printf(">");
            break;
        case TYPE_TYPE:
            switch (e->u._const.u.type->type) {
            case TYPE_EXPR: print(ctx, "<Expr>"); break;
            case TYPE_TYPE: print(ctx, "<Type>"); break;
            case TYPE_UNIT: print(ctx, "<Unit>"); break;
            case TYPE_BOOL: print(ctx, "<Bool>"); break;
            case TYPE_INT: print(ctx, "<Int>"); break;
            case TYPE_FUN: print(ctx, "<Fn>"); break;
            case TYPE_STRUCT: print(ctx, "<Struct>"); break;
            default:
                print(ctx, "<type:%s>", type_names[e->u._const.u.type->type]); break;
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
        case TYPE_FUN:
            print(ctx, "<fun:%.*s>", e->u._const.u.fun.func->name.len, e->u._const.u.fun.func->name.ptr);
            break;
        default:
            print(ctx, "<const:%s>", type_names[e->u._const.type->type]);
            break;
        }
        break;
    case EXPR_SYM:
        print(ctx, "%.*s", e->u.sym.name.len, e->u.sym.name.ptr);
        break;
    case EXPR_FUN: {
        struct expr_decl *p;
        print(ctx, "fun(");
        for (p = e->u.fun.params; p; p = p->next) {
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
        if (e->u.fun.return_type_expr) {
            print(ctx, " ");
            print_expr(ctx, e->u.fun.return_type_expr);
        }
        print(ctx, ": ");
        print_expr(ctx, e->u.fun.body_expr);
        break;
    }
    case EXPR_CAP:
        print(ctx, "<cap:%.*s>", e->u.cap.func->name.len, e->u.cap.func->name.ptr);
        break;
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
        case PRIM_ASSERT: print_primcall(ctx, "assert", e); break;
        case PRIM_QUOTE: print_primcall(ctx, "quote", e); break;
        case PRIM_SPLICE: print_primcall(ctx, "splice", e); break;
        case PRIM_PRINT: print_primcall(ctx, "print", e); break;
        case PRIM_STATIC: print_primcall(ctx, "static", e); break;
        }
        break;
    }
    case EXPR_CALL: {
        struct expr_link *arg;
        print_expr(ctx, e->u.call.callable_expr);
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
