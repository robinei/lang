#ifndef PARSE_H
#define PARSE_H

#include "scan.h"
#include "expr.h"
#include <setjmp.h>

#define ERROR_MAX 512

struct parse_ctx {
    struct scan_ctx scan;
    char *text;
    int token;
    slice_t token_text;
    slice_t prev_token_text;

    char error[ERROR_MAX + 1];
    jmp_buf error_jmp_buf;
};

struct expr *parse_module(struct parse_ctx *ctx);

#endif
