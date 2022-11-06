#include "expr.h"
#include "mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#define DECL_PRIM_NAME(name) #name,
const char *prim_names[] = {
    FOR_ALL_PRIMS(DECL_PRIM_NAME)
};

#define DECL_EXPR_NAME(name) #name,
const char *expr_names[] = {
    FOR_ALL_EXPRS(DECL_EXPR_NAME)
};



struct expr *expr_visit(struct expr_visit_ctx *ctx, struct expr *e) {
    if (!e) {
        return NULL;
    }
    struct expr e_new = *e;
    ctx->visitor(ctx, &e_new);
    if (memcmp(e, &e_new, sizeof(struct expr))) {
        struct expr *e_copy = allocate(ctx->arena, sizeof(struct expr));
        *e_copy = e_new;
        return e_copy;
    }
    return e;
}

static struct expr_decl *expr_decl_visit(struct expr_visit_ctx *ctx, struct expr_decl *decl) {
    if (!decl) {
        return NULL;
    }
    struct expr_decl decl_new = *decl;
    decl_new.next = expr_decl_visit(ctx, decl_new.next);
    decl_new.type_expr = expr_visit(ctx, decl_new.type_expr);
    decl_new.value_expr = expr_visit(ctx, decl_new.value_expr);
    if (memcmp(&decl_new, decl, sizeof(struct expr_decl))) {
        struct expr_decl *decl_copy = allocate(ctx->arena, sizeof(struct expr_decl));
        *decl_copy = decl_new;
        return decl_copy;
    }
    return decl;
}

static struct expr_link *expr_call_arg_visit(struct expr_visit_ctx *ctx, struct expr_link *arg) {
    if (!arg) {
        return NULL;
    }
    struct expr_link arg_new = *arg;
    arg_new.next = expr_call_arg_visit(ctx, arg_new.next);
    arg_new.expr = expr_visit(ctx, arg_new.expr);
    if (memcmp(&arg_new, arg, sizeof(struct expr_link))) {
        struct expr_link *arg_copy = allocate(ctx->arena, sizeof(struct expr_link));
        *arg_copy = arg_new;
        return arg_copy;
    }
    return arg;
}

void expr_visit_children(struct expr_visit_ctx *ctx, struct expr *e) {
    switch (e->kind) {
    case EXPR_CONST:
    case EXPR_SYM:
        break;
    case EXPR_FUN:
        e->fun.params = expr_decl_visit(ctx, e->fun.params);
        e->fun.return_type_expr = expr_visit(ctx, e->fun.return_type_expr);
        e->fun.body_expr = expr_visit(ctx, e->fun.body_expr);
        break;
    case EXPR_BLOCK:
        e->block.body_expr = expr_visit(ctx, e->block.body_expr);
        break;
    case EXPR_DEF:
        e->def.type_expr = expr_visit(ctx, e->def.type_expr);
        e->def.value_expr = expr_visit(ctx, e->def.value_expr);
        break;
    case EXPR_STRUCT:
        e->struc.body_expr = expr_visit(ctx, e->struc.body_expr);
        break;
    case EXPR_SELF:
        break;
    case EXPR_COND:
        e->cond.pred_expr = expr_visit(ctx, e->cond.pred_expr);
        e->cond.then_expr = expr_visit(ctx, e->cond.then_expr);
        e->cond.else_expr = expr_visit(ctx, e->cond.else_expr);
        break;
    case EXPR_PRIM:
        e->prim.arg_exprs[0] = expr_visit(ctx, e->prim.arg_exprs[0]);
        e->prim.arg_exprs[1] = expr_visit(ctx, e->prim.arg_exprs[1]);
        break;
    case EXPR_CALL:
        e->call.callable_expr = expr_visit(ctx, e->call.callable_expr);
        e->call.args = expr_call_arg_visit(ctx, e->call.args);
        break;
    default:
        assert(NULL && "unexpected expr type");
        break;
    }
}

struct expr *expr_run_visitor(struct expr *e, expr_visitor_t visitor, void *ctx, struct allocator *arena) {
    struct expr_visit_ctx visit_ctx = {
        .visitor = visitor,
        .ctx = ctx,
        .arena = arena
    };
    return expr_visit(&visit_ctx, e);
}




struct print_ctx {
    uint indent;
};

void print_expr(struct print_ctx *ctx, struct expr *e);

#define COLOR_NORMAL    "\x1B[0m"
#define COLOR_RED       "\x1B[31m"
#define COLOR_GREEN     "\x1B[32m"
#define COLOR_YELLOW    "\x1B[33m"
#define COLOR_BLUE      "\x1B[34m"
#define COLOR_MAGENTA   "\x1B[35m"
#define COLOR_CYAN      "\x1B[36m"
#define COLOR_WHITE     "\x1B[37m"

#define PAREN_COLOR     COLOR_CYAN
#define KEYWORD_COLOR   COLOR_YELLOW
#define DESTRUCTIVE_OP_COLOR COLOR_YELLOW
#define OPERATOR_COLOR  COLOR_YELLOW
#define STRING_COLOR    COLOR_RED
#define NUMBER_COLOR    COLOR_MAGENTA
#define BOOL_COLOR      COLOR_MAGENTA
#define TYPE_COLOR      COLOR_GREEN
#define SYMBOL_COLOR    COLOR_NORMAL

static const char *curr_color = COLOR_NORMAL;

static void set_color(const char *color) {
#ifndef _WIN32
    if (strcmp(curr_color, color)) {
        printf("%s", color);
        curr_color = color;
    }
#endif
}

static void print(struct print_ctx *ctx, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

static void print_colored(struct print_ctx *ctx, const char *color, const char *format, ...) {
    va_list args;
    va_start(args, format);
    set_color(color);
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
    print_colored(ctx, OPERATOR_COLOR, "%s", op);
    print_expr(ctx, e->prim.arg_exprs[0]);
}

static void print_binop(struct print_ctx *ctx, const char *op, struct expr *e) {
    print_expr(ctx, e->prim.arg_exprs[0]);
    print_colored(ctx, OPERATOR_COLOR, "%s", op);
    print_expr(ctx, e->prim.arg_exprs[1]);
}

static void print_primcall(struct print_ctx *ctx, const char *name, struct expr *e) {
    print_colored(ctx, SYMBOL_COLOR, "%s", name);
    print_colored(ctx, PAREN_COLOR, "(", name);
    if (e->prim.arg_exprs[0]) {
        print_expr(ctx, e->prim.arg_exprs[0]);
    }
    if (e->prim.arg_exprs[1]) {
        print_colored(ctx, OPERATOR_COLOR, ", ");
        print_expr(ctx, e->prim.arg_exprs[1]);
    }
    print_colored(ctx, PAREN_COLOR, ")");
}

static void print_cond(struct print_ctx *ctx, struct expr *e, bool is_elif) {
    print_colored(ctx, KEYWORD_COLOR, is_elif ? "elif " : "if ");
    print_expr(ctx, e->cond.pred_expr);
    print_colored(ctx, KEYWORD_COLOR, " then\n");
    ++ctx->indent;
    print_indent(ctx);
    print_expr(ctx, e->cond.then_expr);
    --ctx->indent;
    print(ctx, "\n");
    print_indent(ctx);
    if (e->cond.else_expr->kind == EXPR_COND) {
        print_cond(ctx, e->cond.else_expr, true);
    } else if (e->cond.else_expr->kind == EXPR_CONST && e->t == &type_unit) {
        print_colored(ctx, KEYWORD_COLOR, "end");
    } else {
        print_colored(ctx, KEYWORD_COLOR, "else\n");
        ++ctx->indent;
        print_indent(ctx);
        print_expr(ctx, e->cond.else_expr);
        --ctx->indent;
        print(ctx, "\n");
        print_indent(ctx);
        print_colored(ctx, KEYWORD_COLOR, "end");
    }
}

void print_expr(struct print_ctx *ctx, struct expr *e) {
    if (!e) {
        return;
    }
    switch (e->kind) {
    case EXPR_CONST:
        switch (e->t->kind) {
        case TYPE_EXPR:
            print_colored(ctx, COLOR_NORMAL, "Expr<");
            print_expr(ctx, e->c.expr);
            print_colored(ctx, COLOR_NORMAL, ">");
            break;
        case TYPE_TYPE:
            switch (e->c.type->kind) {
            case TYPE_EXPR: print_colored(ctx, COLOR_NORMAL, "<Expr>"); break;
            case TYPE_TYPE: print_colored(ctx, COLOR_NORMAL, "<Type>"); break;
            case TYPE_UNIT: print_colored(ctx, COLOR_NORMAL, "<Unit>"); break;
            case TYPE_BOOL: print_colored(ctx, COLOR_NORMAL, "<Bool>"); break;
            case TYPE_INT: print_colored(ctx, COLOR_NORMAL, "<Int>"); break;
            case TYPE_FUN: print_colored(ctx, COLOR_NORMAL, "<Fn>"); break;
            case TYPE_STRUCT: print_colored(ctx, COLOR_NORMAL, "<Struct>"); break;
            default:
                print_colored(ctx, COLOR_NORMAL, "<type:%s>", type_names[e->c.type->kind]); break;
            }
            break;
        case TYPE_BOOL:
            print_colored(ctx, BOOL_COLOR, "%s", e->c.boolean ? "true" : "false");
            break;
        case TYPE_UNIT:
            print_colored(ctx, PAREN_COLOR, "()");
            break;
        case TYPE_INT:
            print_colored(ctx, NUMBER_COLOR, "%"PRIi64, e->c.integer);
            break;
        case TYPE_UINT:
            print_colored(ctx, NUMBER_COLOR, "%"PRIu64, e->c.uinteger);
            break;
        case TYPE_REAL:
            print_colored(ctx, NUMBER_COLOR, "%f", e->c.real);
            break;
        case TYPE_STRING:
            print_colored(ctx, STRING_COLOR, "\"%.*s\"", e->c.string.len, e->c.string.ptr);
            break;
        case TYPE_FUN:
            print_colored(ctx, COLOR_NORMAL, "<fun:%s>", e->c.fun->name->data);
            break;
        default:
            print_colored(ctx, COLOR_NORMAL, "<const:%s>", type_names[e->t->kind]);
            break;
        }
        break;
    case EXPR_SYM:
        print_colored(ctx, SYMBOL_COLOR, "%s", e->sym->data);
        break;
    case EXPR_FUN:
        print_colored(ctx, KEYWORD_COLOR, "fun");
        if (e->fun.params) {
            print_colored(ctx, PAREN_COLOR, "(");
            for (struct expr_decl *p = e->fun.params; p; p = p->next) {
                print_colored(ctx, SYMBOL_COLOR, "%s", p->name_expr->sym->data);
                if (p->type_expr) {
                    print_colored(ctx, OPERATOR_COLOR, ": ");
                    if (p->is_static) {
                        print_colored(ctx, KEYWORD_COLOR, "static ");
                    }
                    print_expr(ctx, p->type_expr);
                }
                if (p->next) {
                    print_colored(ctx, OPERATOR_COLOR, "; ");
                }
            }
            print_colored(ctx, PAREN_COLOR, ")");
        }
        if (e->fun.return_type_expr) {
            print_colored(ctx, OPERATOR_COLOR, ": ");
            print_expr(ctx, e->fun.return_type_expr);
        }
        if (e->fun.body_expr) {
            print_colored(ctx, OPERATOR_COLOR, " ->\n");
            ++ctx->indent;
            print_indent(ctx);
            print_expr(ctx, e->fun.body_expr);
            --ctx->indent;
        }
        break;
    case EXPR_STRUCT:
        print_colored(ctx, KEYWORD_COLOR, "struct\n");
        ++ctx->indent;
        print_indent(ctx);
        print_expr(ctx, e->struc.body_expr);
        print(ctx, "\n");
        --ctx->indent;
        print_indent(ctx);
        print_colored(ctx, KEYWORD_COLOR, "end");
        break;
    case EXPR_SELF:
        print_colored(ctx, KEYWORD_COLOR, "self");
        break;
    case EXPR_BLOCK:
        print_colored(ctx, KEYWORD_COLOR, "begin\n");
        ++ctx->indent;
        print_indent(ctx);
        print_expr(ctx, e->block.body_expr);
        print(ctx, "\n");
        --ctx->indent;
        print_indent(ctx);
        print_colored(ctx, KEYWORD_COLOR, "end");
        break;
    case EXPR_DEF:
        if (e->flags & EXPR_FLAG_DEF_VAR) {
            print_colored(ctx, KEYWORD_COLOR, "var ");
        } else {
            print_colored(ctx, KEYWORD_COLOR, "const ");
        }
        if (e->flags & EXPR_FLAG_DEF_STATIC) {
            print_colored(ctx, KEYWORD_COLOR, "static ");
        }
        print_expr(ctx, e->def.name_expr);
        if (e->def.type_expr) {
            print_colored(ctx, OPERATOR_COLOR, ": ");
            print_expr(ctx, e->def.type_expr);
        }
        if (e->def.value_expr) {
            print_colored(ctx, OPERATOR_COLOR, " = ");
            print_expr(ctx, e->def.value_expr);
        }
        break;
    case EXPR_COND:
        print_cond(ctx, e, false);
        break;
    case EXPR_PRIM:
        switch (e->prim.kind) {
        case PRIM_PLUS: print_unop(ctx, "+", e); break;
        case PRIM_NEGATE: print_unop(ctx, "-", e); break;
        case PRIM_LOGI_NOT: print_unop(ctx, "not ", e); break;
        case PRIM_BITWISE_NOT: print_unop(ctx, "~", e); break;
        case PRIM_SEQ:
            print_expr(ctx, e->prim.arg_exprs[0]);
            print_colored(ctx, OPERATOR_COLOR, ";\n");
            print_indent(ctx);
            print_expr(ctx, e->prim.arg_exprs[1]);
            break;
        case PRIM_ASSIGN: print_binop(ctx, " = ", e); break;
        case PRIM_LOGI_OR: print_binop(ctx, " or ", e); break;
        case PRIM_LOGI_AND: print_binop(ctx, " and ", e); break;
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
        case PRIM_DOT: print_binop(ctx, ".", e); break;
        case PRIM_ASSERT: print_primcall(ctx, "assert", e); break;
        case PRIM_QUOTE: print_primcall(ctx, "quote", e); break;
        case PRIM_SPLICE: print_primcall(ctx, "splice", e); break;
        case PRIM_PRINT: print_primcall(ctx, "print", e); break;
        case PRIM_IMPORT: print_primcall(ctx, "import", e); break;
        case PRIM_STATIC: print_primcall(ctx, "static", e); break;
        }
        break;
    case EXPR_CALL: {
        struct expr_link *arg;
        print_expr(ctx, e->call.callable_expr);
        print_colored(ctx, PAREN_COLOR, "(");
        for (arg = e->call.args; arg; arg = arg->next) {
            print_expr(ctx, arg->expr);
            if (arg->next) {
                print_colored(ctx, OPERATOR_COLOR, ", ");
            }
        }
        print_colored(ctx, PAREN_COLOR, ")");
        break;
    }
    }
}


void pretty_print_indented(struct expr *e, uint indent) {
    struct print_ctx print_ctx = { indent, };
    print_indent(&print_ctx);
    print_expr(&print_ctx, e);
    set_color(COLOR_NORMAL);
    printf("\n");
}

void pretty_print(struct expr *e) {
    pretty_print_indented(e, 0);
}
