#ifndef SCAN_H
#define SCAN_H

#define FOR_ALL_TOKENS(X) \
    X(ERR) \
    X(END) \
    X(ASSIGN) \
    X(EQ) \
    X(NEQ) \
    X(LT) \
    X(GT) \
    X(LTEQ) \
    X(GTEQ) \
    X(PLUS) \
    X(MINUS) \
    X(MUL) \
    X(DIV) \
    X(MOD) \
    X(LSH) \
    X(RSH) \
    X(AND) \
    X(AND_BW) \
    X(OR) \
    X(OR_BW) \
    X(XOR_BW) \
    X(NOT) \
    X(NOT_BW) \
    X(COMMA) \
    X(COLON) \
    X(SEMICOLON) \
    X(LPAREN) \
    X(RPAREN) \
    X(LBRACKET) \
    X(RBRACKET) \
    X(KW_MODULE) \
    X(KW_IMPORT) \
    X(KW_STRUCT) \
    X(KW_FN) \
    X(KW_LET) \
    X(KW_IN) \
    X(KW_IF) \
    X(KW_ELIF) \
    X(KW_ELSE) \
    X(KW_CASE) \
    X(KW_OF) \
    X(KW_WHILE) \
    X(KW_INT) \
    X(KW_BOOL) \
    X(KW_TRUE) \
    X(KW_FALSE) \
    X(IDENT) \
    X(BIN) \
    X(OCT) \
    X(DEC) \
    X(HEX)

#define DECL_TOK_ENUM(Tok) TOK_##Tok,
enum {
    FOR_ALL_TOKENS(DECL_TOK_ENUM)
};
#undef DECL_TOK_ENUM

const char *token_strings[];

struct scan_ctx {
    char *cursor;
    int line;
};

int scan_next_token(struct scan_ctx *ctx, char **begin);

#endif