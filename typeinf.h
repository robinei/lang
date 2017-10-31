#ifndef TYPEINF_H
#define TYPEINF_H

#include "type.h"
#include "slice.h"

struct typeinf_ctx {
    int i;
};

struct type *typeinf(struct typeinf_ctx *ctx, struct expr *e);

#endif
