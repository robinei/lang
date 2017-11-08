/* Generated by re2c 0.14.3 on Thu Nov  9 20:30:34 2017 */
#line 1 "scan.re2c"
#include "scan.h"

#define DECL_TOK_STR(Tok) #Tok,
const char *token_strings[] = {
    FOR_ALL_TOKENS(DECL_TOK_STR)
};

#define TOK(Tok) return TOK_##Tok

int scan_next_token(struct scan_ctx *ctx, char **begin) {
    char *marker = (void *)0;
next_token_start:;
    *begin = ctx->cursor;

#line 18 "scan.c"
{
	char yych;
	unsigned int yyaccept = 0;

	yych = *ctx->cursor;
	switch (yych) {
	case 0x00:	goto yy3;
	case '\t':
	case '\v':
	case '\r':
	case ' ':	goto yy7;
	case '\n':	goto yy8;
	case '!':	goto yy10;
	case '%':	goto yy12;
	case '&':	goto yy14;
	case '(':	goto yy16;
	case ')':	goto yy18;
	case '*':	goto yy20;
	case '+':	goto yy22;
	case ',':	goto yy24;
	case '-':	goto yy26;
	case '/':	goto yy28;
	case '0':	goto yy30;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy32;
	case ':':	goto yy34;
	case ';':	goto yy36;
	case '<':	goto yy38;
	case '=':	goto yy40;
	case '>':	goto yy42;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'd':
	case 'g':
	case 'h':
	case 'j':
	case 'k':
	case 'n':
	case 'q':
	case 'r':
	case 'u':
	case 'x':
	case 'y':
	case 'z':	goto yy44;
	case '[':	goto yy46;
	case ']':	goto yy48;
	case '^':	goto yy50;
	case 'c':	goto yy52;
	case 'e':	goto yy53;
	case 'f':	goto yy54;
	case 'i':	goto yy55;
	case 'l':	goto yy56;
	case 'm':	goto yy57;
	case 'o':	goto yy58;
	case 'p':	goto yy59;
	case 's':	goto yy60;
	case 't':	goto yy61;
	case 'v':	goto yy62;
	case 'w':	goto yy63;
	case '{':	goto yy64;
	case '|':	goto yy66;
	case '}':	goto yy68;
	case '~':	goto yy70;
	default:	goto yy5;
	}
yy2:
#line 29 "scan.re2c"
	{ goto next_token_start; }
#line 121 "scan.c"
yy3:
	++ctx->cursor;
#line 27 "scan.re2c"
	{ TOK(END); }
#line 126 "scan.c"
yy5:
	++ctx->cursor;
#line 26 "scan.re2c"
	{ TOK(ERR); }
#line 131 "scan.c"
yy7:
	yych = *++ctx->cursor;
	goto yy180;
yy8:
	++ctx->cursor;
#line 35 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 142 "scan.c"
yy10:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '=':	goto yy177;
	default:	goto yy11;
	}
yy11:
#line 59 "scan.re2c"
	{ TOK(NOT); }
#line 152 "scan.c"
yy12:
	++ctx->cursor;
#line 51 "scan.re2c"
	{ TOK(MOD); }
#line 157 "scan.c"
yy14:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '&':	goto yy175;
	default:	goto yy15;
	}
yy15:
#line 55 "scan.re2c"
	{ TOK(AND_BW); }
#line 167 "scan.c"
yy16:
	++ctx->cursor;
#line 64 "scan.re2c"
	{ TOK(LPAREN); }
#line 172 "scan.c"
yy18:
	++ctx->cursor;
#line 65 "scan.re2c"
	{ TOK(RPAREN); }
#line 177 "scan.c"
yy20:
	++ctx->cursor;
#line 49 "scan.re2c"
	{ TOK(MUL); }
#line 182 "scan.c"
yy22:
	++ctx->cursor;
#line 47 "scan.re2c"
	{ TOK(PLUS); }
#line 187 "scan.c"
yy24:
	++ctx->cursor;
#line 61 "scan.re2c"
	{ TOK(COMMA); }
#line 192 "scan.c"
yy26:
	++ctx->cursor;
#line 48 "scan.re2c"
	{ TOK(MINUS); }
#line 197 "scan.c"
yy28:
	yyaccept = 0;
	yych = *(marker = ++ctx->cursor);
	switch (yych) {
	case '/':	goto yy168;
	default:	goto yy29;
	}
yy29:
#line 50 "scan.re2c"
	{ TOK(DIV); }
#line 208 "scan.c"
yy30:
	yyaccept = 1;
	yych = *(marker = ++ctx->cursor);
	switch (yych) {
	case 'B':
	case 'b':	goto yy161;
	case 'X':
	case 'x':	goto yy157;
	default:	goto yy160;
	}
yy31:
#line 91 "scan.re2c"
	{ TOK(OCT); }
#line 222 "scan.c"
yy32:
	++ctx->cursor;
	yych = *ctx->cursor;
	goto yy156;
yy33:
#line 92 "scan.re2c"
	{ TOK(DEC); }
#line 230 "scan.c"
yy34:
	++ctx->cursor;
#line 62 "scan.re2c"
	{ TOK(COLON); }
#line 235 "scan.c"
yy36:
	++ctx->cursor;
#line 63 "scan.re2c"
	{ TOK(SEMICOLON); }
#line 240 "scan.c"
yy38:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '<':	goto yy151;
	case '=':	goto yy153;
	default:	goto yy39;
	}
yy39:
#line 43 "scan.re2c"
	{ TOK(LT); }
#line 251 "scan.c"
yy40:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '=':	goto yy149;
	default:	goto yy41;
	}
yy41:
#line 40 "scan.re2c"
	{ TOK(ASSIGN); }
#line 261 "scan.c"
yy42:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '=':	goto yy147;
	case '>':	goto yy145;
	default:	goto yy43;
	}
yy43:
#line 44 "scan.re2c"
	{ TOK(GT); }
#line 272 "scan.c"
yy44:
	++ctx->cursor;
	yych = *ctx->cursor;
	goto yy75;
yy45:
#line 89 "scan.re2c"
	{ TOK(IDENT); }
#line 280 "scan.c"
yy46:
	++ctx->cursor;
#line 66 "scan.re2c"
	{ TOK(LBRACKET); }
#line 285 "scan.c"
yy48:
	++ctx->cursor;
#line 67 "scan.re2c"
	{ TOK(RBRACKET); }
#line 290 "scan.c"
yy50:
	++ctx->cursor;
#line 58 "scan.re2c"
	{ TOK(XOR_BW); }
#line 295 "scan.c"
yy52:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy141;
	default:	goto yy75;
	}
yy53:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy131;
	case 'n':	goto yy132;
	default:	goto yy75;
	}
yy54:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy124;
	case 'n':	goto yy125;
	default:	goto yy75;
	}
yy55:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy114;
	case 'm':	goto yy116;
	case 'n':	goto yy117;
	default:	goto yy75;
	}
yy56:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy111;
	default:	goto yy75;
	}
yy57:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy105;
	default:	goto yy75;
	}
yy58:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy103;
	default:	goto yy75;
	}
yy59:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy99;
	default:	goto yy75;
	}
yy60:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy88;
	default:	goto yy75;
	}
yy61:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy84;
	default:	goto yy75;
	}
yy62:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy81;
	default:	goto yy75;
	}
yy63:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'h':	goto yy76;
	default:	goto yy75;
	}
yy64:
	++ctx->cursor;
#line 68 "scan.re2c"
	{ TOK(LBRACE); }
#line 376 "scan.c"
yy66:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '|':	goto yy72;
	default:	goto yy67;
	}
yy67:
#line 57 "scan.re2c"
	{ TOK(OR_BW); }
#line 386 "scan.c"
yy68:
	++ctx->cursor;
#line 69 "scan.re2c"
	{ TOK(RBRACE); }
#line 391 "scan.c"
yy70:
	++ctx->cursor;
#line 60 "scan.re2c"
	{ TOK(NOT_BW); }
#line 396 "scan.c"
yy72:
	++ctx->cursor;
#line 56 "scan.re2c"
	{ TOK(OR); }
#line 401 "scan.c"
yy74:
	++ctx->cursor;
	yych = *ctx->cursor;
yy75:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy45;
	}
yy76:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy77;
	default:	goto yy75;
	}
yy77:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy78;
	default:	goto yy75;
	}
yy78:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy79;
	default:	goto yy75;
	}
yy79:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy80;
	}
yy80:
#line 82 "scan.re2c"
	{ TOK(KW_WHILE); }
#line 561 "scan.c"
yy81:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy82;
	default:	goto yy75;
	}
yy82:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy83;
	}
yy83:
#line 83 "scan.re2c"
	{ TOK(KW_MUT); }
#line 639 "scan.c"
yy84:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy85;
	default:	goto yy75;
	}
yy85:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy86;
	default:	goto yy75;
	}
yy86:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy87;
	}
yy87:
#line 86 "scan.re2c"
	{ TOK(KW_TRUE); }
#line 723 "scan.c"
yy88:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy90;
	case 'r':	goto yy89;
	default:	goto yy75;
	}
yy89:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy95;
	default:	goto yy75;
	}
yy90:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy91;
	default:	goto yy75;
	}
yy91:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy92;
	default:	goto yy75;
	}
yy92:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy93;
	default:	goto yy75;
	}
yy93:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy94;
	}
yy94:
#line 84 "scan.re2c"
	{ TOK(KW_STATIC); }
#line 826 "scan.c"
yy95:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy96;
	default:	goto yy75;
	}
yy96:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy97;
	default:	goto yy75;
	}
yy97:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy98;
	}
yy98:
#line 73 "scan.re2c"
	{ TOK(KW_STRUCT); }
#line 910 "scan.c"
yy99:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy100;
	default:	goto yy75;
	}
yy100:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy101;
	default:	goto yy75;
	}
yy101:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy102;
	}
yy102:
#line 85 "scan.re2c"
	{ TOK(KW_PURE); }
#line 994 "scan.c"
yy103:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy104;
	}
yy104:
#line 81 "scan.re2c"
	{ TOK(KW_OF); }
#line 1066 "scan.c"
yy105:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy106;
	default:	goto yy75;
	}
yy106:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy107;
	default:	goto yy75;
	}
yy107:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy108;
	default:	goto yy75;
	}
yy108:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy109;
	default:	goto yy75;
	}
yy109:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy110;
	}
yy110:
#line 71 "scan.re2c"
	{ TOK(KW_MODULE); }
#line 1162 "scan.c"
yy111:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy112;
	default:	goto yy75;
	}
yy112:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy113;
	}
yy113:
#line 75 "scan.re2c"
	{ TOK(KW_LET); }
#line 1240 "scan.c"
yy114:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy115;
	}
yy115:
#line 77 "scan.re2c"
	{ TOK(KW_IF); }
#line 1312 "scan.c"
yy116:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'p':	goto yy119;
	default:	goto yy75;
	}
yy117:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy118;
	}
yy118:
#line 76 "scan.re2c"
	{ TOK(KW_IN); }
#line 1390 "scan.c"
yy119:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy120;
	default:	goto yy75;
	}
yy120:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy121;
	default:	goto yy75;
	}
yy121:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy122;
	default:	goto yy75;
	}
yy122:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy123;
	}
yy123:
#line 72 "scan.re2c"
	{ TOK(KW_IMPORT); }
#line 1480 "scan.c"
yy124:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy127;
	default:	goto yy75;
	}
yy125:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy126;
	}
yy126:
#line 74 "scan.re2c"
	{ TOK(KW_FN); }
#line 1558 "scan.c"
yy127:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy128;
	default:	goto yy75;
	}
yy128:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy129;
	default:	goto yy75;
	}
yy129:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy130;
	}
yy130:
#line 87 "scan.re2c"
	{ TOK(KW_FALSE); }
#line 1642 "scan.c"
yy131:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy135;
	case 's':	goto yy136;
	default:	goto yy75;
	}
yy132:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy133;
	default:	goto yy75;
	}
yy133:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy134;
	}
yy134:
#line 70 "scan.re2c"
	{ TOK(KW_END); }
#line 1727 "scan.c"
yy135:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy139;
	default:	goto yy75;
	}
yy136:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy137;
	default:	goto yy75;
	}
yy137:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy138;
	}
yy138:
#line 79 "scan.re2c"
	{ TOK(KW_ELSE); }
#line 1811 "scan.c"
yy139:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy140;
	}
yy140:
#line 78 "scan.re2c"
	{ TOK(KW_ELIF); }
#line 1883 "scan.c"
yy141:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy142;
	default:	goto yy75;
	}
yy142:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy143;
	default:	goto yy75;
	}
yy143:
	++ctx->cursor;
	switch ((yych = *ctx->cursor)) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '_':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':	goto yy74;
	default:	goto yy144;
	}
yy144:
#line 80 "scan.re2c"
	{ TOK(KW_CASE); }
#line 1967 "scan.c"
yy145:
	++ctx->cursor;
#line 53 "scan.re2c"
	{ TOK(RSH); }
#line 1972 "scan.c"
yy147:
	++ctx->cursor;
#line 46 "scan.re2c"
	{ TOK(GTEQ); }
#line 1977 "scan.c"
yy149:
	++ctx->cursor;
#line 41 "scan.re2c"
	{ TOK(EQ); }
#line 1982 "scan.c"
yy151:
	++ctx->cursor;
#line 52 "scan.re2c"
	{ TOK(LSH); }
#line 1987 "scan.c"
yy153:
	++ctx->cursor;
#line 45 "scan.re2c"
	{ TOK(LTEQ); }
#line 1992 "scan.c"
yy155:
	++ctx->cursor;
	yych = *ctx->cursor;
yy156:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy155;
	default:	goto yy33;
	}
yy157:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':	goto yy165;
	default:	goto yy158;
	}
yy158:
	ctx->cursor = marker;
	switch (yyaccept) {
	case 0: 	goto yy29;
	case 1: 	goto yy31;
	default:	goto yy172;
	}
yy159:
	++ctx->cursor;
	yych = *ctx->cursor;
yy160:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':	goto yy159;
	default:	goto yy31;
	}
yy161:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy162;
	default:	goto yy158;
	}
yy162:
	++ctx->cursor;
	yych = *ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy162;
	default:	goto yy164;
	}
yy164:
#line 90 "scan.re2c"
	{ TOK(BIN); }
#line 2077 "scan.c"
yy165:
	++ctx->cursor;
	yych = *ctx->cursor;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':	goto yy165;
	default:	goto yy167;
	}
yy167:
#line 93 "scan.re2c"
	{ TOK(HEX); }
#line 2109 "scan.c"
yy168:
	++ctx->cursor;
	yych = *ctx->cursor;
	switch (yych) {
	case 0x00:	goto yy170;
	case '\n':	goto yy173;
	default:	goto yy168;
	}
yy170:
	yyaccept = 2;
	marker = ++ctx->cursor;
	yych = *ctx->cursor;
	switch (yych) {
	case 0x00:	goto yy170;
	case '\n':	goto yy173;
	default:	goto yy168;
	}
yy172:
#line 30 "scan.re2c"
	{ TOK(END); }
#line 2130 "scan.c"
yy173:
	++ctx->cursor;
#line 31 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 2138 "scan.c"
yy175:
	++ctx->cursor;
#line 54 "scan.re2c"
	{ TOK(AND); }
#line 2143 "scan.c"
yy177:
	++ctx->cursor;
#line 42 "scan.re2c"
	{ TOK(NEQ); }
#line 2148 "scan.c"
yy179:
	++ctx->cursor;
	yych = *ctx->cursor;
yy180:
	switch (yych) {
	case '\t':
	case '\v':
	case '\r':
	case ' ':	goto yy179;
	default:	goto yy2;
	}
}
#line 94 "scan.re2c"

}
