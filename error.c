#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void error_ctx_init(struct error_ctx *ctx, slice_t filename, slice_t source_text, struct arena_allocator *arena) {
    memset(ctx, 0, sizeof(struct error_ctx));
    ctx->arena = arena;
    ctx->source_buf = source_text;
    ctx->filename = filename;
}

void error_emit(struct error_ctx *ctx, enum error_category category, slice_t location, const char *format, ...) {
    va_list args;
    int len;
    struct error_entry *entry;

    va_start(args, format);
    len = vsnprintf(ctx->msg_buf, ERROR_MSG_BUF_SIZE, format, args);
    va_end(args);
    if (len >= ERROR_MSG_BUF_SIZE) {
        len = ERROR_MSG_BUF_SIZE - 1;
    }
    ctx->msg_buf[len] = '\0';

    entry = allocate(&ctx->arena->a, sizeof(struct error_entry) + len + 1);
    entry->next = NULL;
    entry->category = category;
    entry->location = location;
    memcpy(entry->message, ctx->msg_buf, len + 1);

    if (ctx->last_error) {
        ctx->last_error->next = entry;
        ctx->last_error = entry;
    }
    else {
        ctx->first_error = ctx->last_error = entry;
    }
}

slice_t error_line_text(struct error_ctx *ctx, struct error_entry *entry) {
    char *buf_start = ctx->source_buf.ptr;
    char *buf_end = ctx->source_buf.ptr + ctx->source_buf.len;
    char *start = entry->location.ptr;
    char *end = entry->location.ptr;
    slice_t slice;

    while (start > buf_start && start[-1] != '\n') {
        --start;
    }
    while (end < buf_end && *end != '\r' && *end != '\n') {
        ++end;
    }

    slice.ptr = start;
    slice.len = slice.cap = end - start;
    return slice;
}

uint error_line_num(struct error_ctx *ctx, struct error_entry *entry) {
    uint line = 0;
    char *buf_start = ctx->source_buf.ptr;
    char *ch = entry->location.ptr - 1;
    for (; ch >= buf_start; --ch) {
        if (*ch == '\n') {
            ++line;
        }
    }
    return line;
}

uint error_col_num(struct error_ctx *ctx, struct error_entry *entry) {
    uint col = 0;
    char *buf_start = ctx->source_buf.ptr;
    char *ch = entry->location.ptr - 1;
    for (; ch >= buf_start && *ch != '\n'; --ch) {
        ++col;
    }
    return col;
}

static void print_slice(FILE *fp, slice_t slice) {
    fprintf(fp, "%.*s", slice.len, slice.ptr);
}

static void print_repeated(FILE *fp, int ch, uint times) {
    uint i;
    for (i = 0; i < times; ++i) {
        fputc(ch, fp);
    }
}

void error_fprint(struct error_ctx *ctx, struct error_entry *entry, FILE *fp) {
    uint line_num = error_line_num(ctx, entry);
    uint col_num = error_col_num(ctx, entry);
    slice_t line = error_line_text(ctx, entry);
    uint err_len = line.len - col_num;
    if (entry->location.len < err_len) {
        err_len = entry->location.len;
    }

    fprintf(fp, "(%.*s:%u:%u) ", ctx->filename.len, ctx->filename.ptr, line_num + 1, col_num + 1);
    switch (entry->category) {
    case ERROR_CATEGORY_MESSAGE:
        fprintf(fp, "MESSAGE: ");
        break;
    case ERROR_CATEGORY_WARNING:
        fprintf(fp, "WARNING: ");
        break;
    case ERROR_CATEGORY_ERROR:
        fprintf(fp, "ERROR: ");
        break;
    }
    fprintf(fp, "%s\n", entry->message);

    print_slice(fp, line);
    fputc('\n', fp);
    print_repeated(fp, ' ', col_num);
    print_repeated(fp, '^', err_len ? err_len : 1);
    fputc('\n', fp);
}

void print_errors(struct error_ctx *err_ctx) {
    struct error_entry *entry;
    for (entry = err_ctx->first_error; entry; entry = entry->next) {
        error_fprint(err_ctx, entry, stdout);
    }
}
