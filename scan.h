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
    X(AND_BW) \
    X(OR_BW) \
    X(XOR_BW) \
    X(NOT) \
    X(NOT_BW) \
    X(COMMA) \
    X(PERIOD) \
    X(COLON) \
    X(SEMICOLON) \
    X(LPAREN) \
    X(RPAREN) \
    X(LBRACKET) \
    X(RBRACKET) \
    X(LBRACE) \
    X(RBRACE) \
    X(RARROW) \
    X(LARROW) \
    X(KW_BEGIN) \
    X(KW_END) \
    X(KW_MODULE) \
    X(KW_IMPORT) \
    X(KW_STRUCT) \
    X(KW_SELF) \
    X(KW_FUN) \
    X(KW_DEF) \
    X(KW_PUB) \
    X(KW_CONST) \
    X(KW_VAR) \
    X(KW_LET) \
    X(KW_IN) \
    X(KW_IF) \
    X(KW_THEN) \
    X(KW_ELIF) \
    X(KW_ELSE) \
    X(KW_CASE) \
    X(KW_OF) \
    X(KW_DO) \
    X(KW_FOR) \
    X(KW_WHILE) \
    X(KW_STATIC) \
    X(KW_PURE) \
    X(KW_TRUE) \
    X(KW_FALSE) \
    X(KW_AND) \
    X(KW_OR) \
    X(KW_NOT) \
    X(IDENT) \
    X(BIN) \
    X(OCT) \
    X(DEC) \
    X(HEX)

#define DECL_TOK_ENUM(Tok) TOK_##Tok,
enum token_kind { FOR_ALL_TOKENS(DECL_TOK_ENUM) };
#undef DECL_TOK_ENUM

extern const char *token_strings[];

struct scan_ctx {
    char *cursor;
    int line;
};

int scan_next_token(struct scan_ctx *ctx, char **begin);

#endif