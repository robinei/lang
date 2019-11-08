#include "parse.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>

unsigned long long my_strtoull(const char *str, const char **endptr, int base);
double my_strtod(const char *string, const char **endPtr);


struct parse_ctx {
    struct allocator *arena;
    struct global_ctx *global_ctx;
    struct module_ctx *mod_ctx;
    struct error_ctx *err_ctx;

    char *ptr;
    char *line_start;
    int line;
    int current_indent;
    bool skip_newline;
    jmp_buf error_jmp_buf;
};

#define PARSE_ERR(...) \
    do { \
        error_emit(ctx->err_ctx, ERROR_CATEGORY_ERROR, slice_from_str_len(ctx->ptr, 1), __VA_ARGS__); \
        longjmp(ctx->error_jmp_buf, 1); \
    } while(0)

#define SLICE_FROM_START slice_from_str_len(start, ctx->ptr - start)


enum {
    PREC_LOWEST = 1,

    PREC_ASSIGN = PREC_LOWEST,
    PREC_LOGI_OR,
    PREC_LOGI_AND,
    PREC_BW_OR,
    PREC_BW_XOR,
    PREC_BW_AND,
    PREC_EQ,
    PREC_LTGT,
    PREC_SHIFT,
    PREC_ADDSUB,
    PREC_MULDIVMOD,

    PREC_HIGHEST
};
static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence);


static struct expr *expr_create(struct parse_ctx *ctx, uint expr_type, slice_t source_text) {
    struct expr *e = allocate(ctx->arena, sizeof(struct expr));
    e->kind = expr_type;
    e->source_text = source_text;
    return e;
}
static struct expr *bool_create(struct parse_ctx *ctx, bool bool_value, slice_t source_text) {
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    e->c.tag = &type_bool;
    e->c.boolean = bool_value;
    return e;
}
static struct expr *string_create(struct parse_ctx *ctx, bool has_escapes, slice_t source_text) {
    assert(source_text.len > 2);
    assert(source_text.ptr[0] == '"');
    assert(source_text.ptr[source_text.len - 1] == '"');
    // TODO: handle escape sequences
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    source_text.ptr += 1;
    source_text.len -= 2;
    e->c.tag = &type_string;
    e->c.string = source_text;
    return e;
}
static struct expr *symbol_create(struct parse_ctx *ctx, char *start) {
    struct expr *e = expr_create(ctx, EXPR_SYM, SLICE_FROM_START);
    e->sym = intern_slice(&ctx->global_ctx->symbol_table, e->source_text);
    return e;
}
static struct expr *unit_create(struct parse_ctx *ctx, slice_t source_text) {
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    e->c.tag = &type_unit;
    return e;
}
static struct expr *prim_create_bin(struct parse_ctx *ctx, int prim, struct expr *lhs, struct expr *rhs) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, slice_span(lhs->source_text, rhs->source_text));
    e->prim.kind = prim;
    e->prim.arg_exprs[0] = lhs;
    e->prim.arg_exprs[1] = rhs;
    return e;
}



static bool is_space_char(char ch) {
    switch (ch) {
    case '\r':
    case '\n':
    case '\t':
    case ' ':
        return true;
    default:
        return false;
    }
}
static bool is_leading_ident_char(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}
static bool is_non_leading_ident_char(char ch) {
    return is_leading_ident_char(ch) || (ch >= '0' && ch <= '9');
}

static bool is_reserved_word(char *str, int len) {
    switch (*str) {
    case 'a':
        if (len == 3 && !memcmp(str, "and", len)) { return true; }
        if (len == 6 && !memcmp(str, "assert", len)) { return true; }
        break;
    case 'c':
        if (len == 5 && !memcmp(str, "const", len)) { return true; }
        break;
    case 'd':
        if (len == 2 && !memcmp(str, "do", len)) { return true; }
        break;
    case 'e':
        if (len == 4 && !memcmp(str, "else", len)) { return true; }
        if (len == 4 && !memcmp(str, "elif", len)) { return true; }
        break;
    case 'f':
        if (len == 3 && !memcmp(str, "fun", len)) { return true; }
        if (len == 5 && !memcmp(str, "false", len)) { return true; }
        break;
    case 'F':
        if (len == 3 && !memcmp(str, "Fun", len)) { return true; }
        break;
    case 'i':
        if (len == 2 && !memcmp(str, "if", len)) { return true; }
        if (len == 6 && !memcmp(str, "import", len)) { return true; }
        break;
    case 'n':
        if (len == 3 && !memcmp(str, "not", len)) { return true; }
        break;
    case 'o':
        if (len == 2 && !memcmp(str, "or", len)) { return true; }
        break;
    case 'p':
        if (len == 5 && !memcmp(str, "print", len)) { return true; }
        break;
    case 'q':
        if (len == 5 && !memcmp(str, "quote", len)) { return true; }
        break;
    case 's':
        if (len == 6 && !memcmp(str, "struct", len)) { return true; }
        if (len == 6 && !memcmp(str, "static", len)) { return true; }
        if (len == 6 && !memcmp(str, "splice", len)) { return true; }
        break;
    case 'S':
        if (len == 4 && !memcmp(str, "Self", len)) { return true; }
        break;
    case 't':
        if (len == 4 && !memcmp(str, "then", len)) { return true; }
        if (len == 4 && !memcmp(str, "true", len)) { return true; }
        break;
    case 'v':
        if (len == 3 && !memcmp(str, "var", len)) { return true; }
        break;
    }
    return false;
}

static bool match_ident(struct parse_ctx *ctx) {
    if (!is_leading_ident_char(*ctx->ptr)) {
        return false;
    }
    char *start = ctx->ptr;
    do {
        ++ctx->ptr;
    } while(is_non_leading_ident_char(*ctx->ptr));
    if (is_reserved_word(start, ctx->ptr - start)) {
        PARSE_ERR("unexpected keyword '%.*s'", ctx->ptr - start, start);
    }
    return true;
}
static void expect_ident(struct parse_ctx *ctx, const char *context_info) {
    if (!match_ident(ctx)) {
        PARSE_ERR("expected identifier %s", context_info);
    }
}

static bool match_keyword(struct parse_ctx *ctx, const char *str) {
    assert(is_leading_ident_char(*str));
    char *ptr = ctx->ptr;
    while (*ptr && *str) {
        if (*ptr != *str) {
            return false;
        }
        ++ptr;
        ++str;
        assert(!*str || is_non_leading_ident_char(*str));
    }
    if (*str) {
        return false;
    }
    if (is_non_leading_ident_char(*ptr)) {
        return false;
    }
    ctx->ptr = ptr;
    return true;
}
static void expect_keyword(struct parse_ctx *ctx, const char *str, const char *context_info) {
    if (!match_keyword(ctx, str)) {
        PARSE_ERR("expected '%s' %s", str, context_info);
    }
}

static bool match_str(struct parse_ctx *ctx, const char *str) {
    assert(*str);
    char *ptr = ctx->ptr;
    while (*ptr && *str) {
        if (*ptr != *str) {
            return false;
        }
        ++ptr;
        ++str;
    }
    if (*str) {
        return false;
    }
    ctx->ptr = ptr;
    return true;
}
static void expect_str(struct parse_ctx *ctx, const char *str, const char *context_info) {
    if (!match_str(ctx, str)) {
        PARSE_ERR("expected '%s' %s", str, context_info);
    }
}

static bool match_char(struct parse_ctx *ctx, char ch) {
    if (*ctx->ptr != ch) {
        return false;
    }
    ++ctx->ptr;
    return true;
}
static void expect_char(struct parse_ctx *ctx, char ch, const char *context_info) {
    if (!match_char(ctx, ch)) {
        PARSE_ERR("expected '%c' %s", ch, context_info);
    }
}

static void skip_leading_space(struct parse_ctx *ctx) {
    ctx->current_indent = 0;
    for (;;) {
        switch (*ctx->ptr) {
        case '\t':
            PARSE_ERR("tabs are illegal as indentation");
        case ' ':
            ++ctx->current_indent;
            ++ctx->ptr;
            continue;
        default:
            return;
        }
    }
}
static void move_to_next_line(struct parse_ctx *ctx) {
    assert(*ctx->ptr == '\n');
    ++ctx->ptr;
    ++ctx->line;
    ctx->line_start = ctx->ptr;
    skip_leading_space(ctx);
}
static void skip_whitespace(struct parse_ctx *ctx) {
    for (;;) {
        switch (*ctx->ptr) {
        case '\n':
            if (!ctx->skip_newline) {
                return;
            }
            move_to_next_line(ctx);
            continue;
        case '\r':
        case '\t':
        case ' ':
            ++ctx->ptr;
            continue;
        }
        if (!(ctx->ptr[0] == '/' && ctx->ptr[1] == '/')) {
            break;
        }
        ctx->ptr += 2;
        for (;;) {
            char ch = *ctx->ptr;
            if (ch == '\0') {
                return;
            }
            if (ch == '\n') {
                if (!ctx->skip_newline) {
                    return;
                }
                move_to_next_line(ctx);
                break;
            }
            ++ctx->ptr;
        }
    }
}
static void skip_whitespace_including_newlines(struct parse_ctx *ctx) {
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = true;
    skip_whitespace(ctx);
    ctx->skip_newline = prev_skip_newline;
}
static void skip_whitespace_excluding_newlines(struct parse_ctx *ctx) {
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = false;
    skip_whitespace(ctx);
    ctx->skip_newline = prev_skip_newline;
}


static struct expr *parse_expr(struct parse_ctx *ctx) {
    return parse_infix(ctx, PREC_LOWEST + 1); /* disallow '=' operator */
}


static struct expr *parse_unary(struct parse_ctx *ctx, int prim, char *start) {
    skip_whitespace(ctx);
    struct expr *arg = parse_infix(ctx, PREC_HIGHEST);
    struct expr *result = expr_create(ctx, EXPR_PRIM, SLICE_FROM_START);
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = arg;
    return result;
}

static struct expr *parse_string(struct parse_ctx *ctx) {
    char *start = ctx->ptr++; /* skip initial " */
    bool has_escapes = false;
    for (;;) {
        switch (*ctx->ptr) {
        case '"':
            ++ctx->ptr;
            return string_create(ctx, has_escapes, SLICE_FROM_START);
        case '\0':
            PARSE_ERR("unexpected end of input while parsing string literal");
        case '\\':
            ctx->ptr += 2;
            has_escapes = true;
            continue;
        default:
            ++ctx->ptr;
            continue;
        }
    }
}

static struct expr *parse_number(struct parse_ctx *ctx) {
    char *start = ctx->ptr;
    uint64_t value = my_strtoull(ctx->ptr, (const char **)&ctx->ptr, 0);
    if (ctx->ptr > start && *ctx->ptr != '.') {
        if (is_leading_ident_char(*ctx->ptr)) {
            PARSE_ERR("illegal number suffix");
        }
        struct expr *result = expr_create(ctx, EXPR_CONST, SLICE_FROM_START);
        if (value <= INT64_MAX) {
            result->c.tag = &type_int;
            result->c.integer = (int64_t)value;
        } else {
            result->c.tag = &type_uint;
            result->c.uinteger = value;
        }
        return result;
    } else {
        double real = my_strtod(start, (const char **)&ctx->ptr);
        if (ctx->ptr == start) {
            PARSE_ERR("error parsing number");
        }
        if (is_leading_ident_char(*ctx->ptr)) {
            PARSE_ERR("illegal number suffix");
        }
        struct expr *result = expr_create(ctx, EXPR_CONST, SLICE_FROM_START);
        result->c.tag = &type_real;
        result->c.real = real;
        return result;
    }
}

static struct expr *parse_expr_seq_at_indent(struct parse_ctx *ctx, int indent_to_keep) {
    struct expr *result = NULL;
    struct expr *last_seq = NULL;
    for (;;) {
        skip_whitespace(ctx);
        switch (*ctx->ptr) {
        case '\n':
            move_to_next_line(ctx);
            continue;
        case '\0':
        case ')':
        case ',':
            goto out;
        }
        if (ctx->current_indent > indent_to_keep) {
            PARSE_ERR("unexpectedly increased indentation");
        }
        if (ctx->current_indent < indent_to_keep) {
            break;
        }
        struct expr *e = parse_infix(ctx, PREC_LOWEST); /* allow '=' */
        if (!result) {
            result = e;
        } else if (!last_seq) {
            result = last_seq = prim_create_bin(ctx, PRIM_SEQ, result, e);
        } else {
            last_seq = last_seq->prim.arg_exprs[1] = prim_create_bin(ctx, PRIM_SEQ, last_seq->prim.arg_exprs[1], e);
        }
    }
out:
    if (!result) {
        PARSE_ERR("expected block expression");
    }
    return result;
}
static struct expr *parse_expr_seq(struct parse_ctx *ctx) {
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = false;
    skip_whitespace(ctx);
    if (*ctx->ptr != '\n') {
        // if an expression starts on the same line as a "block starter",
        // then expect a single-line expression for this block
        ctx->skip_newline = prev_skip_newline;
        return parse_expr(ctx);
    }
    int initial_indent = ctx->current_indent;
    move_to_next_line(ctx);
    if (ctx->current_indent <= initial_indent) {
        PARSE_ERR("expected indented expression");
    }
    struct expr *result = parse_expr_seq_at_indent(ctx, ctx->current_indent);
    ctx->skip_newline = prev_skip_newline;
    return result;
}
static struct expr *maybe_wrap_in_block(struct parse_ctx *ctx, struct expr *e, char *start) {
    if (e->kind == EXPR_BLOCK) {
        return e; /* avoid redundant block in block */
    }
    if (e->kind != EXPR_DEF && (e->kind != EXPR_PRIM || e->prim.kind != PRIM_SEQ)) {
        return e; /* avoid redundant wrapping of single non-def expression */
    }
    struct expr *result = expr_create(ctx, EXPR_BLOCK, SLICE_FROM_START);
    result->block.body_expr = e;
    return result;
}
static struct expr *parse_expr_seq_and_wrap_in_block(struct parse_ctx *ctx) {
    char *start = ctx->ptr;
    struct expr *body = parse_expr_seq(ctx);
    return maybe_wrap_in_block(ctx, body, start);
}
static struct expr *parse_do(struct parse_ctx *ctx, char *start) {
    struct expr *body = parse_expr_seq(ctx);
    return maybe_wrap_in_block(ctx, body, start);
}
static struct expr *parse_struct(struct parse_ctx *ctx, char *start) {
    struct expr *body = parse_expr_seq(ctx);
    struct expr *result = expr_create(ctx, EXPR_STRUCT, SLICE_FROM_START);
    result->struc.body_expr = body;
    return result;
}


static struct expr_decl *parse_decls(struct parse_ctx *ctx) {
    skip_whitespace(ctx);
    char *start = ctx->ptr;
    struct expr_decl *d = allocate(ctx->arena, sizeof(struct expr_decl));
    expect_ident(ctx, "in declaration");
    d->name_expr = symbol_create(ctx, start);
    skip_whitespace(ctx);
    if (match_char(ctx, ',')) {
        d->next = parse_decls(ctx);
        d->type_expr = d->next->type_expr;
        d->value_expr = d->next->value_expr;
        d->is_static = d->next->is_static;
    }
    else {
        if (match_char(ctx, ':')) {
            skip_whitespace(ctx);
            if (match_keyword(ctx, "static")) {
                d->is_static = true;
                skip_whitespace(ctx);
            }
            d->type_expr = parse_expr(ctx);
        }
        if (match_char(ctx, '=')) {
            skip_whitespace(ctx);
            d->value_expr = parse_expr(ctx);
        }
        if (match_char(ctx, ';')) {
            skip_whitespace(ctx);
            d->next = parse_decls(ctx);
        }
    }
    return d;
}

static struct expr *parse_fun(struct parse_ctx *ctx, bool is_type, char *start) {
    struct expr *return_type_expr = NULL, *body_expr = NULL;
    struct expr_decl *params = NULL;

    skip_whitespace(ctx);
    expect_char(ctx, '(', "after 'fun'");
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = true;
    skip_whitespace(ctx);
    if (ctx->ptr[0] != ')' && !(ctx->ptr[0] == '-' && ctx->ptr[1] == '>')) {
        params = parse_decls(ctx);
        skip_whitespace(ctx);
    }
    if (match_str(ctx, "->")) {
        skip_whitespace(ctx);
        return_type_expr = parse_expr(ctx);
        skip_whitespace(ctx);
    }
    expect_char(ctx, ')', "after parameter list");
    ctx->skip_newline = prev_skip_newline;
    if (!is_type) {
        skip_whitespace(ctx);
        body_expr = parse_expr_seq_and_wrap_in_block(ctx);
    } else {
        for (struct expr_decl *p = params; p; p = p->next) {
            if (!p->type_expr) {
                PARSE_ERR("incomplete parameter types in preceding function type definition");
            }
        }
        if (!return_type_expr) {
            PARSE_ERR("missing return type in preceding function type definition");
        }
    }

    struct expr *result = expr_create(ctx, EXPR_FUN, SLICE_FROM_START);
    result->fun.params = params;
    result->fun.return_type_expr = return_type_expr;
    result->fun.body_expr = body_expr;
    return result;
}

static struct expr *parse_def(struct parse_ctx *ctx, bool is_const, char *start) {
    unsigned int flags = 0;
    if (!is_const) {
        flags |= EXPR_FLAG_DEF_VAR;
    }
    skip_whitespace(ctx);
    if (match_keyword(ctx, "static")) {
        flags |= EXPR_FLAG_DEF_STATIC;
        skip_whitespace(ctx);
    }
    char *sym_start = ctx->ptr;
    expect_ident(ctx, "in declaration");
    struct expr *name_expr = symbol_create(ctx, sym_start);
    skip_whitespace(ctx);
    struct expr *type_expr = NULL;
    if (match_char(ctx, ':')) {
        skip_whitespace(ctx);
        type_expr = parse_expr(ctx);
        skip_whitespace(ctx);
    }
    struct expr *value_expr = NULL;
    if (match_char(ctx, '=')) {
        skip_whitespace(ctx);
        value_expr = parse_expr(ctx);
    }

    struct expr *e = expr_create(ctx, EXPR_DEF, SLICE_FROM_START);
    e->flags = flags;
    e->def.name_expr = name_expr;
    e->def.type_expr = type_expr;
    e->def.value_expr = value_expr;
    return e;
}

static struct expr *parse_if(struct parse_ctx *ctx, bool is_elif, char *start) {
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = true;
    skip_whitespace(ctx);
    struct expr *pred_expr = parse_expr(ctx);
    skip_whitespace(ctx);
    expect_keyword(ctx, "then", "after 'if' condition");
    int then_indent = ctx->current_indent;
    struct expr *then_expr = parse_expr_seq_and_wrap_in_block(ctx);
    char *after_then = ctx->ptr;
    skip_whitespace(ctx);
    ctx->skip_newline = prev_skip_newline;
    if (ctx->current_indent > then_indent) {
        PARSE_ERR("unexpected indentation");
    }
    struct expr *else_expr;
    if (ctx->current_indent == then_indent && match_keyword(ctx, "else")) {
        else_expr = parse_expr_seq_and_wrap_in_block(ctx);
    } else if (ctx->current_indent == then_indent && match_keyword(ctx, "elif")) {
        else_expr = parse_if(ctx, true, after_then);
    } else {
        slice_t text = then_expr->source_text;
        text.ptr += text.len;
        text.len = 0;
        else_expr = unit_create(ctx, text);
    }
    struct expr *result = expr_create(ctx, EXPR_COND, SLICE_FROM_START);
    result->cond.pred_expr = pred_expr;
    result->cond.then_expr = then_expr;
    result->cond.else_expr = else_expr;
    return result;
}

static struct expr *parse_single_arg_prim(struct parse_ctx *ctx, int prim, char *start) {
    char *after_ident = ctx->ptr;
    skip_whitespace(ctx);
    if (!match_char(ctx, '(')) {
        PARSE_ERR("expected '(' after '%.*s'", after_ident - start, start);
    }
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = true;
    skip_whitespace(ctx);
    struct expr *e = parse_expr(ctx);
    if (!match_char(ctx, ')')) {
        PARSE_ERR("expected ')' after argument to '%.*s'", after_ident - start, start);
    }
    ctx->skip_newline = prev_skip_newline;
    struct expr *result = expr_create(ctx, EXPR_PRIM, SLICE_FROM_START);
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = e;
    return result;
}

static struct expr *parse_atom(struct parse_ctx *ctx) {
    char *start = ctx->ptr;
    switch (*start) {
    case '(': {
        ++ctx->ptr;
        bool prev_skip_newline = ctx->skip_newline;
        ctx->skip_newline = true;
        skip_whitespace(ctx);
        if (match_char(ctx, ')')) {
            ctx->skip_newline = prev_skip_newline;
            return unit_create(ctx, SLICE_FROM_START);
        } else {
            struct expr *result = parse_expr(ctx);
            skip_whitespace(ctx);
            expect_char(ctx, ')', "after expression following '('");
            ctx->skip_newline = prev_skip_newline;
            return result;
        }
        break;
    }
    case '+':
        ++ctx->ptr;
        return parse_unary(ctx, PRIM_PLUS, start);
    case '-':
        ++ctx->ptr;
        return parse_unary(ctx, PRIM_NEGATE, start);
    case '~':
        ++ctx->ptr;
        return parse_unary(ctx, PRIM_BITWISE_NOT, start);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number(ctx);
    case '"':
        return parse_string(ctx);
    case 'a':
        if (match_keyword(ctx, "assert")) {
            return parse_single_arg_prim(ctx, PRIM_ASSERT, start);
        }
        break;
    case 'c':
        if (match_keyword(ctx, "const")) {
            return parse_def(ctx, true, start);
        }
        break;
    case 'd':
        if (match_keyword(ctx, "do")) {
            return parse_do(ctx, start);
        }
        break;
    case 'f':
        if (match_keyword(ctx, "false")) {
            return bool_create(ctx, false, SLICE_FROM_START);
        }
        if (match_keyword(ctx, "fun")) {
            return parse_fun(ctx, false, start);
        }
        break;
    case 'F':
        if (match_keyword(ctx, "Fun")) {
            return parse_fun(ctx, true, start);
        }
        break;
    case 'i':
        if (match_keyword(ctx, "if")) {
            return parse_if(ctx, false, start);
        }
        if (match_keyword(ctx, "import")) {
            return parse_single_arg_prim(ctx, PRIM_IMPORT, start);
        }
        break;
    case 'n':
        if (match_keyword(ctx, "not")) {
            return parse_unary(ctx, PRIM_LOGI_NOT, start);
        }
        break;
    case 'p':
        if (match_keyword(ctx, "print")) {
            return parse_single_arg_prim(ctx, PRIM_PRINT, start);
        }
        break;
    case 'q':
        if (match_keyword(ctx, "quote")) {
            return parse_single_arg_prim(ctx, PRIM_QUOTE, start);
        }
        break;
    case 's':
        if (match_keyword(ctx, "struct")) {
            return parse_struct(ctx, start);
        }
        if (match_keyword(ctx, "static")) {
            return parse_single_arg_prim(ctx, PRIM_STATIC, start);
        }
        if (match_keyword(ctx, "splice")) {
            return parse_single_arg_prim(ctx, PRIM_SPLICE, start);
        }
        break;
    case 'S':
        if (match_keyword(ctx, "Self")) {
            return expr_create(ctx, EXPR_SELF, SLICE_FROM_START);
        }
    case 't':
        if (match_keyword(ctx, "true")) {
            return bool_create(ctx, true, SLICE_FROM_START);
        }
        break;
    case 'v':
        if (match_keyword(ctx, "var")) {
            return parse_def(ctx, false, start);
        }
        break;
    }
    
    if (match_ident(ctx)) {
        return symbol_create(ctx, start);
    }
    PARSE_ERR("unexpected input while parsing atom");
}




static struct expr_link *parse_args(struct parse_ctx *ctx) {
    if (match_char(ctx, ')')) {
        return NULL;
    }
    struct expr_link *a = allocate(ctx->arena, sizeof(struct expr_link));
    a->expr = parse_expr(ctx);
    skip_whitespace(ctx);
    if (match_char(ctx, ',')) {
        skip_whitespace(ctx);
        a->next = parse_args(ctx);
    } else {
        expect_char(ctx, ')', "after argument list");
    }
    return a;
}
static struct expr *parse_call(struct parse_ctx *ctx, struct expr *callable_expr) {
    bool prev_skip_newline = ctx->skip_newline;
    ctx->skip_newline = true;
    skip_whitespace(ctx);
    struct expr_link *args = parse_args(ctx);
    ctx->skip_newline = prev_skip_newline;
    char *start = callable_expr->source_text.ptr;
    struct expr *result = expr_create(ctx, EXPR_CALL, SLICE_FROM_START);
    result->call.callable_expr = callable_expr;
    result->call.args = args;
    return result;
}

static void quote_args(struct parse_ctx *ctx, struct expr_link *arg) {
    if (!arg) {
        return;
    }
    struct expr *e = expr_create(ctx, EXPR_PRIM, arg->expr->source_text);
    e->prim.kind = PRIM_QUOTE;
    e->prim.arg_exprs[0] = arg->expr;
    arg->expr = e;
    quote_args(ctx, arg->next);
}
static struct expr *macroify(struct parse_ctx *ctx, struct expr *call_expr) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, call_expr->source_text);
    e->prim.kind = PRIM_SPLICE;
    e->prim.arg_exprs[0] = call_expr;
    quote_args(ctx, call_expr->call.args);
    return e;
}

#define HANDLE_INFIX_PRIM(Precedence, Prim, SkipChars) \
    { \
        if (Precedence < min_precedence) { return result; } \
        ctx->ptr += SkipChars; \
        skip_whitespace(ctx); \
        struct expr *rhs = parse_infix(ctx, Precedence + 1); \
        result = prim_create_bin(ctx, Prim, result, rhs); \
        continue; \
    }

static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence) {
    struct expr *result = parse_atom(ctx);

    while (true) {
        skip_whitespace(ctx);

        switch (*ctx->ptr) {
        case '=':
            if (ctx->ptr[1] == '=') HANDLE_INFIX_PRIM(PREC_EQ, PRIM_EQ, 2)
            else HANDLE_INFIX_PRIM(PREC_LOWEST, PRIM_ASSIGN, 1)

        case 'o':
            if (match_keyword(ctx, "or")) HANDLE_INFIX_PRIM(PREC_LOGI_OR, PRIM_LOGI_OR, 1)
            else return result;

        case 'a':
            if (match_keyword(ctx, "and")) HANDLE_INFIX_PRIM(PREC_LOGI_AND, PRIM_LOGI_AND, 1)
            else return result;

        case '|': HANDLE_INFIX_PRIM(PREC_BW_OR, PRIM_BITWISE_OR, 1)

        case '^': HANDLE_INFIX_PRIM(PREC_BW_XOR, PRIM_BITWISE_XOR, 1)

        case '&': HANDLE_INFIX_PRIM(PREC_BW_AND, PRIM_BITWISE_AND, 1)

        case '!':
            if (ctx->ptr[1] == '=') HANDLE_INFIX_PRIM(PREC_EQ, PRIM_NEQ, 2)
            else {
                if (PREC_HIGHEST < min_precedence) { return result; }
                ++ctx->ptr;
                skip_whitespace(ctx);
                expect_char(ctx, '(', "after postfix '!'");
                result = parse_call(ctx, result);
                result = macroify(ctx, result);
                continue;
            }

        case '<':
            if (ctx->ptr[1] == '=') HANDLE_INFIX_PRIM(PREC_LTGT, PRIM_LTEQ, 2)
            else if (ctx->ptr[1] == '<') HANDLE_INFIX_PRIM(PREC_SHIFT, PRIM_BITWISE_LSH, 2)
            else HANDLE_INFIX_PRIM(PREC_LTGT, PRIM_LT, 1)
        case '>':
            if (ctx->ptr[1] == '=') HANDLE_INFIX_PRIM(PREC_LTGT, PRIM_GTEQ, 2)
            else if (ctx->ptr[1] == '>') HANDLE_INFIX_PRIM(PREC_SHIFT, PRIM_BITWISE_RSH, 2)
            else HANDLE_INFIX_PRIM(PREC_LTGT, PRIM_GT, 1)

        case '+': HANDLE_INFIX_PRIM(PREC_ADDSUB, PRIM_ADD, 1)
        case '-':
            if (ctx->ptr[1] != '>') HANDLE_INFIX_PRIM(PREC_ADDSUB, PRIM_SUB, 1)
            else return result;

        case '*': HANDLE_INFIX_PRIM(PREC_MULDIVMOD, PRIM_MUL, 1)
        case '/': HANDLE_INFIX_PRIM(PREC_MULDIVMOD, PRIM_DIV, 1)
        case '%': HANDLE_INFIX_PRIM(PREC_MULDIVMOD, PRIM_MOD, 1)

        case '.': HANDLE_INFIX_PRIM(PREC_HIGHEST, PRIM_DOT, 1)
        case '(':
            if (PREC_HIGHEST < min_precedence) { return result; }
            ++ctx->ptr;
            result = parse_call(ctx, result);
            continue;

        default: return result;
        }
    }
}

static struct expr *do_parse_module(struct parse_ctx *ctx) {
    char *start = ctx->ptr;
    skip_leading_space(ctx);
    struct expr *body = parse_expr_seq_at_indent(ctx, 0);
    struct expr *result = expr_create(ctx, EXPR_STRUCT, SLICE_FROM_START);
    skip_whitespace(ctx);
    if (*ctx->ptr) {
        PARSE_ERR("unexpected input in module");
    }
    result->struc.body_expr = body;
    return result;
}

struct expr *parse_module(struct module_ctx *mod_ctx, slice_t source_text) {
    struct parse_ctx parse_ctx = {0};
    parse_ctx.arena = &mod_ctx->arena.a;
    parse_ctx.global_ctx = mod_ctx->global_ctx;
    parse_ctx.mod_ctx = mod_ctx;
    parse_ctx.err_ctx = &mod_ctx->err_ctx;

    parse_ctx.line_start = parse_ctx.ptr = source_text.ptr;

    if (setjmp(parse_ctx.error_jmp_buf)) {
        return NULL;
    }

    return do_parse_module(&parse_ctx);
}
