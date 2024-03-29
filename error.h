#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include "alloc.h"
#include "slice.h"

#define ERROR_FILENAME_BUF_SIZE 256
#define ERROR_MSG_BUF_SIZE 1024

enum error_category {
    ERROR_CATEGORY_MESSAGE,
    ERROR_CATEGORY_WARNING,
    ERROR_CATEGORY_ERROR
};

struct error_entry {
    struct error_entry *next;
    enum error_category category;
    uint source_pos;
    char message[0];
};

struct error_ctx {
    struct arena_allocator *arena;
    struct error_entry *first_error;
    struct error_entry *last_error;
    slice_t source_buf;
    slice_t filename;
    char msg_buf[ERROR_MSG_BUF_SIZE];
};

void error_ctx_init(struct error_ctx *ctx, slice_t filename, slice_t source_text, struct arena_allocator *arena);

void error_emit(struct error_ctx *ctx, enum error_category category, uint source_pos, const char *format, ...);

slice_t error_line_text(struct error_ctx *ctx, struct error_entry *entry);
uint error_line_num(struct error_ctx *ctx, struct error_entry *entry);
uint error_col_num(struct error_ctx *ctx, struct error_entry *entry);

void error_fprint(struct error_ctx *ctx, struct error_entry *entry, FILE *fp);

void print_errors(struct error_ctx *err_ctx);

#endif
