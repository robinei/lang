#ifndef PARSE_H
#define PARSE_H

#include "scan.h"
#include "mod.h"
#include <setjmp.h>

struct parse_ctx {
    struct arena *arena;
    struct scan_ctx scan_ctx;
    struct module_ctx *mod_ctx;
    struct error_ctx *err_ctx;

    enum token_kind token;
    slice_t token_text;
    slice_t prev_token_text;

    jmp_buf error_jmp_buf;
};

struct expr *parse_module(struct module_ctx *mod_ctx, char *source_text);

#endif
