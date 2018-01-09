#ifndef PARSE_H
#define PARSE_H

#include "scan.h"
#include "expr.h"
#include "error.h"
#include <setjmp.h>

struct parse_ctx {
    struct scan_ctx scan_ctx;
    struct error_ctx *err_ctx;

    int token;
    slice_t token_text;
    slice_t prev_token_text;

    int sync_after_error;
    jmp_buf error_jmp_buf;
};

struct expr *parse_module(struct parse_ctx *ctx);

#endif
