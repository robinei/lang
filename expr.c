#include "expr.h"
#include <stdio.h>
#include <stdlib.h>

#define DECL_PRIM_NAME(name) #name,
const char *prim_names[] = {
    FOR_ALL_PRIMS(DECL_PRIM_NAME)
};

#define DECL_EXPR_NAME(name) #name,
const char *expr_names[] = {
    FOR_ALL_EXPRS(DECL_EXPR_NAME)
};


struct expr expr_true = { EXPR_CONST, { { &type_bool, { 1 } } } };
struct expr expr_false = { EXPR_CONST, { { &type_bool, { 0 } } } };

static void print_indent(int indent) {
    int i;
    for (i = 0; i < indent * 2; ++i) {
        putchar(' ');
    }
}

void print_expr(struct expr *e, int indent) {
    if (!e) {
        print_indent(indent); printf("NULL\n");
        return;
    }
    print_indent(indent); printf("expr %s: ", expr_names[e->expr]);
    switch (e->expr) {
    case EXPR_CONST:
        switch (e->u._const.type->type) {
        case TYPE_BOOL:
            printf("type = %s, value = %s\n", type_names[e->u._const.type->type], e->u._const.u._bool ? "true" : "false");
            break;
        case TYPE_INT:
            printf("type = %s, value = %d\n", type_names[e->u._const.type->type], e->u._const.u._int);
            break;
        default:
            printf("type = %s, value = ?\n", type_names[e->u._const.type->type]);
            break;
        }
        break;
    case EXPR_SYM:
        printf("name = '%.*s'\n", e->u.sym.name.len, e->u.sym.name.ptr);
        break;
    case EXPR_FN: {
        struct expr_fn_param *p;
        printf("\n");
        for (p = e->u.fn.params; p; p = p->next) {
            print_indent(indent + 1); printf("param '%.*s' type:\n", p->name.len, p->name.ptr);
            print_expr(p->type_expr, indent + 2);
        }
        print_indent(indent + 1); printf("return_type:\n");
        print_expr(e->u.fn.return_type_expr, indent + 2);
        print_indent(indent + 1); printf("body:\n");
        print_expr(e->u.fn.body_expr, indent + 2);
        printf("\n");
        break;
    }
    case EXPR_LET: {
        struct expr_let_binding *b;
        printf("\n");
        for (b = e->u.let.bindings; b; b = b->next) {
            print_indent(indent + 1); printf("binding '%.*s'\n", b->name.len, b->name.ptr);
            print_indent(indent + 2); printf("type:\n");
            print_expr(b->type_expr, indent + 2);
            print_indent(indent + 2); printf("value:\n");
            print_expr(b->value_expr, indent + 3);
        }
        printf("\n");
        break;
    }
    case EXPR_STRUCT: {
        struct expr_struct_field *f;
        printf("\n");
        for (f = e->u._struct.fields; f; f = f->next) {
            print_indent(indent + 1); printf("field '%.*s'\n", f->name.len, f->name.ptr);
            print_indent(indent + 2); printf("type:\n");
            print_expr(f->type_expr, indent + 3);
            print_indent(indent + 2); printf("value:\n");
            print_expr(f->value_expr, indent + 3);
        }
        printf("\n");
        break;
    }
    case EXPR_IF: {
        printf("\n");
        print_indent(indent + 1); printf("cond:\n");
        print_expr(e->u._if.cond_expr, indent + 2);
        print_indent(indent + 1); printf("then:\n");
        print_expr(e->u._if.then_expr, indent + 2);
        print_indent(indent + 1); printf("else:\n");
        print_expr(e->u._if.else_expr, indent + 2);
        break;
    }
    case EXPR_PRIM: {
        printf("type = %s\n", prim_names[e->u.prim.prim]);
        if (e->u.prim.arg_count > 0) {
            print_indent(indent + 1); printf("arg 0:\n");
            print_expr(e->u.prim.arg_expr0, indent + 2);
        }
        if (e->u.prim.arg_count > 1) {
            print_indent(indent + 1); printf("arg 1:\n");
            print_expr(e->u.prim.arg_expr1, indent + 2);
        }
        break;
    }
    case EXPR_CALL: {
        struct expr_call_arg *arg;
        int i;
        printf("\n");
        print_indent(indent + 1); printf("fn\n");
        print_expr(e->u.call.fn_expr, indent + 2);
        for (arg = e->u.call.args, i = 0; arg; arg = arg->next, ++i) {
            print_indent(indent + 1); printf("arg %d:\n", i);
            print_expr(arg->expr, indent + 2);
        }
        break;
    }
    }
}
