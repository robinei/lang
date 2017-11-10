#ifndef PRIM_H
#define PRIM_H

#define FOR_ALL_PRIMS(X) \
    X(PLUS) \
    X(NEGATE) \
    X(LOGI_NOT) \
    X(BITWISE_NOT) \
    X(SEQ) \
    X(LOGI_OR) \
    X(LOGI_AND) \
    X(BITWISE_OR) \
    X(BITWISE_XOR) \
    X(BITWISE_AND) \
    X(EQ) \
    X(NEQ) \
    X(LT) \
    X(GT) \
    X(LTEQ) \
    X(GTEQ) \
    X(BITWISE_LSH) \
    X(BITWISE_RSH) \
    X(ADD) \
    X(SUB) \
    X(MUL) \
    X(DIV) \
    X(MOD)

#define DECL_PRIM_ENUM(name) PRIM_##name,
enum {
    FOR_ALL_PRIMS(DECL_PRIM_ENUM)
};
#undef DECL_PRIM_ENUM

extern const char *prim_names[];

struct expr;
struct peval_ctx;

struct expr *eval_prim(struct peval_ctx *ctx, struct expr *e);

#endif
