#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include "arena.h"
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
    slice_t location;
    char message[0];
};

struct error_ctx {
    struct arena *arena;
    struct error_entry *first_error;
    struct error_entry *last_error;
    slice_t source_buf;
    char *filename;
    char msg_buf[ERROR_MSG_BUF_SIZE];
};

void error_ctx_init(struct error_ctx *ctx, const char *filename, char *source_text, struct arena *arena);

void error_emit(struct error_ctx *ctx, enum error_category category, slice_t location, const char *format, ...);

slice_t error_line_text(struct error_ctx *ctx, struct error_entry *entry);
uint error_line_num(struct error_ctx *ctx, struct error_entry *entry);
uint error_col_num(struct error_ctx *ctx, struct error_entry *entry);

void error_fprint(struct error_ctx *ctx, struct error_entry *entry, FILE *fp);

void print_errors(struct error_ctx *err_ctx);

#endif
