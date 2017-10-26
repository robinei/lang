#ifndef LEXER_H
#define LEXER_H

#define LEXER_ALL_TOKENS(X) \
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
    X(KW_MODULE) \
    X(KW_IMPORT) \
    X(KW_STRUCT) \
    X(KW_FN) \
    X(KW_LET) \
    X(KW_IN) \
    X(KW_IF) \
    X(KW_ELIF) \
    X(KW_ELSE) \
    X(KW_WHILE) \
    X(KW_INT) \
    X(IDENT) \
    X(BIN) \
    X(OCT) \
    X(DEC) \
    X(HEX)

#define LEXER_DECL_TOK_ENUM(Tok) TOK_##Tok,
enum {
    LEXER_ALL_TOKENS(LEXER_DECL_TOK_ENUM)
};
#undef LEXER_DECL_TOK_ENUM

const char *lexer_token_strings[];

struct lexer_ctx {
    char *cursor;
    int line;
};

int lexer_next_token(struct lexer_ctx *ctx, char **begin);

#endif