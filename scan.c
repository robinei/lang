/* Generated by re2c 1.0.3 on Tue Feb  6 21:30:14 2018 */
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
	yych = *ctx->cursor;
	switch (yych) {
	case 0x00:	goto yy2;
	case '\t':
	case '\v':
	case '\r':
	case ' ':	goto yy6;
	case '\n':	goto yy9;
	case '!':	goto yy11;
	case '%':	goto yy13;
	case '&':	goto yy15;
	case '(':	goto yy17;
	case ')':	goto yy19;
	case '*':	goto yy21;
	case '+':	goto yy23;
	case ',':	goto yy25;
	case '-':	goto yy27;
	case '/':	goto yy29;
	case '0':	goto yy31;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy33;
	case ':':	goto yy36;
	case ';':	goto yy38;
	case '<':	goto yy40;
	case '=':	goto yy42;
	case '>':	goto yy44;
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
	case 'z':	goto yy46;
	case '[':	goto yy49;
	case ']':	goto yy51;
	case '^':	goto yy53;
	case 'c':	goto yy55;
	case 'e':	goto yy56;
	case 'f':	goto yy57;
	case 'i':	goto yy58;
	case 'l':	goto yy59;
	case 'm':	goto yy60;
	case 'o':	goto yy61;
	case 'p':	goto yy62;
	case 's':	goto yy63;
	case 't':	goto yy64;
	case 'v':	goto yy65;
	case 'w':	goto yy66;
	case '{':	goto yy67;
	case '|':	goto yy69;
	case '}':	goto yy71;
	case '~':	goto yy73;
	default:	goto yy4;
	}
yy2:
	++ctx->cursor;
#line 27 "scan.re2c"
	{ TOK(END); }
#line 120 "scan.c"
yy4:
	++ctx->cursor;
#line 26 "scan.re2c"
	{ TOK(ERR); }
#line 125 "scan.c"
yy6:
	yych = *++ctx->cursor;
	switch (yych) {
	case '\t':
	case '\v':
	case '\r':
	case ' ':	goto yy6;
	default:	goto yy8;
	}
yy8:
#line 29 "scan.re2c"
	{ goto next_token_start; }
#line 138 "scan.c"
yy9:
	++ctx->cursor;
#line 35 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 146 "scan.c"
yy11:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy75;
	default:	goto yy12;
	}
yy12:
#line 59 "scan.re2c"
	{ TOK(NOT); }
#line 156 "scan.c"
yy13:
	++ctx->cursor;
#line 51 "scan.re2c"
	{ TOK(MOD); }
#line 161 "scan.c"
yy15:
	yych = *++ctx->cursor;
	switch (yych) {
	case '&':	goto yy77;
	default:	goto yy16;
	}
yy16:
#line 55 "scan.re2c"
	{ TOK(AND_BW); }
#line 171 "scan.c"
yy17:
	++ctx->cursor;
#line 64 "scan.re2c"
	{ TOK(LPAREN); }
#line 176 "scan.c"
yy19:
	++ctx->cursor;
#line 65 "scan.re2c"
	{ TOK(RPAREN); }
#line 181 "scan.c"
yy21:
	++ctx->cursor;
#line 49 "scan.re2c"
	{ TOK(MUL); }
#line 186 "scan.c"
yy23:
	++ctx->cursor;
#line 47 "scan.re2c"
	{ TOK(PLUS); }
#line 191 "scan.c"
yy25:
	++ctx->cursor;
#line 61 "scan.re2c"
	{ TOK(COMMA); }
#line 196 "scan.c"
yy27:
	++ctx->cursor;
#line 48 "scan.re2c"
	{ TOK(MINUS); }
#line 201 "scan.c"
yy29:
	yych = *++ctx->cursor;
	switch (yych) {
	case '/':	goto yy79;
	default:	goto yy30;
	}
yy30:
#line 50 "scan.re2c"
	{ TOK(DIV); }
#line 211 "scan.c"
yy31:
	yych = *(marker = ++ctx->cursor);
	switch (yych) {
	case 'B':
	case 'b':	goto yy83;
	case 'X':
	case 'x':	goto yy85;
	default:	goto yy82;
	}
yy32:
#line 91 "scan.re2c"
	{ TOK(OCT); }
#line 224 "scan.c"
yy33:
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
	case '9':	goto yy33;
	default:	goto yy35;
	}
yy35:
#line 92 "scan.re2c"
	{ TOK(DEC); }
#line 243 "scan.c"
yy36:
	++ctx->cursor;
#line 62 "scan.re2c"
	{ TOK(COLON); }
#line 248 "scan.c"
yy38:
	++ctx->cursor;
#line 63 "scan.re2c"
	{ TOK(SEMICOLON); }
#line 253 "scan.c"
yy40:
	yych = *++ctx->cursor;
	switch (yych) {
	case '<':	goto yy86;
	case '=':	goto yy88;
	default:	goto yy41;
	}
yy41:
#line 43 "scan.re2c"
	{ TOK(LT); }
#line 264 "scan.c"
yy42:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy90;
	default:	goto yy43;
	}
yy43:
#line 40 "scan.re2c"
	{ TOK(ASSIGN); }
#line 274 "scan.c"
yy44:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy92;
	case '>':	goto yy94;
	default:	goto yy45;
	}
yy45:
#line 44 "scan.re2c"
	{ TOK(GT); }
#line 285 "scan.c"
yy46:
	yych = *++ctx->cursor;
yy47:
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
	case 'z':	goto yy46;
	default:	goto yy48;
	}
yy48:
#line 89 "scan.re2c"
	{ TOK(IDENT); }
#line 358 "scan.c"
yy49:
	++ctx->cursor;
#line 66 "scan.re2c"
	{ TOK(LBRACKET); }
#line 363 "scan.c"
yy51:
	++ctx->cursor;
#line 67 "scan.re2c"
	{ TOK(RBRACKET); }
#line 368 "scan.c"
yy53:
	++ctx->cursor;
#line 58 "scan.re2c"
	{ TOK(XOR_BW); }
#line 373 "scan.c"
yy55:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy96;
	default:	goto yy47;
	}
yy56:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy97;
	case 'n':	goto yy98;
	default:	goto yy47;
	}
yy57:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy99;
	case 'u':	goto yy100;
	default:	goto yy47;
	}
yy58:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy101;
	case 'm':	goto yy103;
	case 'n':	goto yy104;
	default:	goto yy47;
	}
yy59:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy106;
	default:	goto yy47;
	}
yy60:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy107;
	default:	goto yy47;
	}
yy61:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy108;
	default:	goto yy47;
	}
yy62:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy110;
	default:	goto yy47;
	}
yy63:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy111;
	default:	goto yy47;
	}
yy64:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy112;
	default:	goto yy47;
	}
yy65:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy113;
	default:	goto yy47;
	}
yy66:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'h':	goto yy114;
	default:	goto yy47;
	}
yy67:
	++ctx->cursor;
#line 68 "scan.re2c"
	{ TOK(LBRACE); }
#line 454 "scan.c"
yy69:
	yych = *++ctx->cursor;
	switch (yych) {
	case '|':	goto yy115;
	default:	goto yy70;
	}
yy70:
#line 57 "scan.re2c"
	{ TOK(OR_BW); }
#line 464 "scan.c"
yy71:
	++ctx->cursor;
#line 69 "scan.re2c"
	{ TOK(RBRACE); }
#line 469 "scan.c"
yy73:
	++ctx->cursor;
#line 60 "scan.re2c"
	{ TOK(NOT_BW); }
#line 474 "scan.c"
yy75:
	++ctx->cursor;
#line 42 "scan.re2c"
	{ TOK(NEQ); }
#line 479 "scan.c"
yy77:
	++ctx->cursor;
#line 54 "scan.re2c"
	{ TOK(AND); }
#line 484 "scan.c"
yy79:
	yych = *++ctx->cursor;
	switch (yych) {
	case '\n':	goto yy117;
	default:	goto yy79;
	}
yy81:
	yych = *++ctx->cursor;
yy82:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':	goto yy81;
	default:	goto yy32;
	}
yy83:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy119;
	default:	goto yy84;
	}
yy84:
	ctx->cursor = marker;
	goto yy32;
yy85:
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
	case 'f':	goto yy122;
	default:	goto yy84;
	}
yy86:
	++ctx->cursor;
#line 52 "scan.re2c"
	{ TOK(LSH); }
#line 546 "scan.c"
yy88:
	++ctx->cursor;
#line 45 "scan.re2c"
	{ TOK(LTEQ); }
#line 551 "scan.c"
yy90:
	++ctx->cursor;
#line 41 "scan.re2c"
	{ TOK(EQ); }
#line 556 "scan.c"
yy92:
	++ctx->cursor;
#line 46 "scan.re2c"
	{ TOK(GTEQ); }
#line 561 "scan.c"
yy94:
	++ctx->cursor;
#line 53 "scan.re2c"
	{ TOK(RSH); }
#line 566 "scan.c"
yy96:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy125;
	default:	goto yy47;
	}
yy97:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy126;
	case 's':	goto yy127;
	default:	goto yy47;
	}
yy98:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy128;
	default:	goto yy47;
	}
yy99:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy130;
	default:	goto yy47;
	}
yy100:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy131;
	default:	goto yy47;
	}
yy101:
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
	case 'z':	goto yy46;
	default:	goto yy102;
	}
yy102:
#line 77 "scan.re2c"
	{ TOK(KW_IF); }
#line 669 "scan.c"
yy103:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'p':	goto yy133;
	default:	goto yy47;
	}
yy104:
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
	case 'z':	goto yy46;
	default:	goto yy105;
	}
yy105:
#line 76 "scan.re2c"
	{ TOK(KW_IN); }
#line 747 "scan.c"
yy106:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy134;
	default:	goto yy47;
	}
yy107:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy136;
	default:	goto yy47;
	}
yy108:
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
	case 'z':	goto yy46;
	default:	goto yy109;
	}
yy109:
#line 81 "scan.re2c"
	{ TOK(KW_OF); }
#line 831 "scan.c"
yy110:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy137;
	default:	goto yy47;
	}
yy111:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy138;
	case 'r':	goto yy139;
	default:	goto yy47;
	}
yy112:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy140;
	default:	goto yy47;
	}
yy113:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy141;
	default:	goto yy47;
	}
yy114:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy143;
	default:	goto yy47;
	}
yy115:
	++ctx->cursor;
#line 56 "scan.re2c"
	{ TOK(OR); }
#line 867 "scan.c"
yy117:
	++ctx->cursor;
#line 31 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 875 "scan.c"
yy119:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy119;
	default:	goto yy121;
	}
yy121:
#line 90 "scan.re2c"
	{ TOK(BIN); }
#line 886 "scan.c"
yy122:
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
	case 'f':	goto yy122;
	default:	goto yy124;
	}
yy124:
#line 93 "scan.re2c"
	{ TOK(HEX); }
#line 917 "scan.c"
yy125:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy144;
	default:	goto yy47;
	}
yy126:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy146;
	default:	goto yy47;
	}
yy127:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy148;
	default:	goto yy47;
	}
yy128:
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
	case 'z':	goto yy46;
	default:	goto yy129;
	}
yy129:
#line 70 "scan.re2c"
	{ TOK(KW_END); }
#line 1007 "scan.c"
yy130:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy150;
	default:	goto yy47;
	}
yy131:
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
	case 'z':	goto yy46;
	default:	goto yy132;
	}
yy132:
#line 74 "scan.re2c"
	{ TOK(KW_FUN); }
#line 1085 "scan.c"
yy133:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy151;
	default:	goto yy47;
	}
yy134:
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
	case 'z':	goto yy46;
	default:	goto yy135;
	}
yy135:
#line 75 "scan.re2c"
	{ TOK(KW_LET); }
#line 1163 "scan.c"
yy136:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy152;
	default:	goto yy47;
	}
yy137:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy153;
	default:	goto yy47;
	}
yy138:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy155;
	default:	goto yy47;
	}
yy139:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy156;
	default:	goto yy47;
	}
yy140:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy157;
	default:	goto yy47;
	}
yy141:
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
	case 'z':	goto yy46;
	default:	goto yy142;
	}
yy142:
#line 83 "scan.re2c"
	{ TOK(KW_MUT); }
#line 1265 "scan.c"
yy143:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy159;
	default:	goto yy47;
	}
yy144:
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
	case 'z':	goto yy46;
	default:	goto yy145;
	}
yy145:
#line 80 "scan.re2c"
	{ TOK(KW_CASE); }
#line 1343 "scan.c"
yy146:
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
	case 'z':	goto yy46;
	default:	goto yy147;
	}
yy147:
#line 78 "scan.re2c"
	{ TOK(KW_ELIF); }
#line 1415 "scan.c"
yy148:
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
	case 'z':	goto yy46;
	default:	goto yy149;
	}
yy149:
#line 79 "scan.re2c"
	{ TOK(KW_ELSE); }
#line 1487 "scan.c"
yy150:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy160;
	default:	goto yy47;
	}
yy151:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy162;
	default:	goto yy47;
	}
yy152:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy163;
	default:	goto yy47;
	}
yy153:
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
	case 'z':	goto yy46;
	default:	goto yy154;
	}
yy154:
#line 85 "scan.re2c"
	{ TOK(KW_PURE); }
#line 1577 "scan.c"
yy155:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy164;
	default:	goto yy47;
	}
yy156:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy165;
	default:	goto yy47;
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
	case 'z':	goto yy46;
	default:	goto yy158;
	}
yy158:
#line 86 "scan.re2c"
	{ TOK(KW_TRUE); }
#line 1661 "scan.c"
yy159:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy166;
	default:	goto yy47;
	}
yy160:
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
	case 'z':	goto yy46;
	default:	goto yy161;
	}
yy161:
#line 87 "scan.re2c"
	{ TOK(KW_FALSE); }
#line 1739 "scan.c"
yy162:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy168;
	default:	goto yy47;
	}
yy163:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy170;
	default:	goto yy47;
	}
yy164:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy172;
	default:	goto yy47;
	}
yy165:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy174;
	default:	goto yy47;
	}
yy166:
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
	case 'z':	goto yy46;
	default:	goto yy167;
	}
yy167:
#line 82 "scan.re2c"
	{ TOK(KW_WHILE); }
#line 1835 "scan.c"
yy168:
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
	case 'z':	goto yy46;
	default:	goto yy169;
	}
yy169:
#line 72 "scan.re2c"
	{ TOK(KW_IMPORT); }
#line 1907 "scan.c"
yy170:
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
	case 'z':	goto yy46;
	default:	goto yy171;
	}
yy171:
#line 71 "scan.re2c"
	{ TOK(KW_MODULE); }
#line 1979 "scan.c"
yy172:
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
	case 'z':	goto yy46;
	default:	goto yy173;
	}
yy173:
#line 84 "scan.re2c"
	{ TOK(KW_STATIC); }
#line 2051 "scan.c"
yy174:
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
	case 'z':	goto yy46;
	default:	goto yy175;
	}
yy175:
#line 73 "scan.re2c"
	{ TOK(KW_STRUCT); }
#line 2123 "scan.c"
}
#line 94 "scan.re2c"

}
