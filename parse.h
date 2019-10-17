#ifndef PARSE_H
#define PARSE_H

#include "mod.h"

struct expr *parse_module(struct module_ctx *mod_ctx, slice_t source_text);

#endif
