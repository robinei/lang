/* Generated by re2c 1.1.1 on Sun Mar 31 00:16:27 2019 */
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
	case '.':	goto yy29;
	case '/':	goto yy31;
	case '0':	goto yy33;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy35;
	case ':':	goto yy38;
	case ';':	goto yy40;
	case '<':	goto yy42;
	case '=':	goto yy44;
	case '>':	goto yy46;
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
	case 'g':
	case 'h':
	case 'j':
	case 'k':
	case 'q':
	case 'r':
	case 'u':
	case 'x':
	case 'y':
	case 'z':	goto yy48;
	case '[':	goto yy51;
	case ']':	goto yy53;
	case '^':	goto yy55;
	case 'a':	goto yy57;
	case 'b':	goto yy58;
	case 'c':	goto yy59;
	case 'd':	goto yy60;
	case 'e':	goto yy61;
	case 'f':	goto yy62;
	case 'i':	goto yy63;
	case 'l':	goto yy64;
	case 'm':	goto yy65;
	case 'n':	goto yy66;
	case 'o':	goto yy67;
	case 'p':	goto yy68;
	case 's':	goto yy69;
	case 't':	goto yy70;
	case 'v':	goto yy71;
	case 'w':	goto yy72;
	case '{':	goto yy73;
	case '|':	goto yy75;
	case '}':	goto yy77;
	case '~':	goto yy79;
	default:	goto yy4;
	}
yy2:
	++ctx->cursor;
#line 27 "scan.re2c"
	{ TOK(END); }
#line 121 "scan.c"
yy4:
	++ctx->cursor;
#line 26 "scan.re2c"
	{ TOK(ERR); }
#line 126 "scan.c"
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
#line 139 "scan.c"
yy9:
	++ctx->cursor;
#line 35 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 147 "scan.c"
yy11:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy81;
	default:	goto yy12;
	}
yy12:
#line 57 "scan.re2c"
	{ TOK(NOT); }
#line 157 "scan.c"
yy13:
	++ctx->cursor;
#line 51 "scan.re2c"
	{ TOK(MOD); }
#line 162 "scan.c"
yy15:
	++ctx->cursor;
#line 54 "scan.re2c"
	{ TOK(AND_BW); }
#line 167 "scan.c"
yy17:
	++ctx->cursor;
#line 63 "scan.re2c"
	{ TOK(LPAREN); }
#line 172 "scan.c"
yy19:
	++ctx->cursor;
#line 64 "scan.re2c"
	{ TOK(RPAREN); }
#line 177 "scan.c"
yy21:
	++ctx->cursor;
#line 49 "scan.re2c"
	{ TOK(MUL); }
#line 182 "scan.c"
yy23:
	++ctx->cursor;
#line 47 "scan.re2c"
	{ TOK(PLUS); }
#line 187 "scan.c"
yy25:
	++ctx->cursor;
#line 59 "scan.re2c"
	{ TOK(COMMA); }
#line 192 "scan.c"
yy27:
	yych = *++ctx->cursor;
	switch (yych) {
	case '>':	goto yy83;
	default:	goto yy28;
	}
yy28:
#line 48 "scan.re2c"
	{ TOK(MINUS); }
#line 202 "scan.c"
yy29:
	++ctx->cursor;
#line 60 "scan.re2c"
	{ TOK(PERIOD); }
#line 207 "scan.c"
yy31:
	yych = *++ctx->cursor;
	switch (yych) {
	case '/':	goto yy85;
	default:	goto yy32;
	}
yy32:
#line 50 "scan.re2c"
	{ TOK(DIV); }
#line 217 "scan.c"
yy33:
	yych = *(marker = ++ctx->cursor);
	switch (yych) {
	case 'B':
	case 'b':	goto yy89;
	case 'X':
	case 'x':	goto yy91;
	default:	goto yy88;
	}
yy34:
#line 103 "scan.re2c"
	{ TOK(OCT); }
#line 230 "scan.c"
yy35:
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
	case '9':	goto yy35;
	default:	goto yy37;
	}
yy37:
#line 104 "scan.re2c"
	{ TOK(DEC); }
#line 249 "scan.c"
yy38:
	++ctx->cursor;
#line 61 "scan.re2c"
	{ TOK(COLON); }
#line 254 "scan.c"
yy40:
	++ctx->cursor;
#line 62 "scan.re2c"
	{ TOK(SEMICOLON); }
#line 259 "scan.c"
yy42:
	yych = *++ctx->cursor;
	switch (yych) {
	case '-':	goto yy92;
	case '<':	goto yy94;
	case '=':	goto yy96;
	default:	goto yy43;
	}
yy43:
#line 43 "scan.re2c"
	{ TOK(LT); }
#line 271 "scan.c"
yy44:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy98;
	default:	goto yy45;
	}
yy45:
#line 40 "scan.re2c"
	{ TOK(ASSIGN); }
#line 281 "scan.c"
yy46:
	yych = *++ctx->cursor;
	switch (yych) {
	case '=':	goto yy100;
	case '>':	goto yy102;
	default:	goto yy47;
	}
yy47:
#line 44 "scan.re2c"
	{ TOK(GT); }
#line 292 "scan.c"
yy48:
	yych = *++ctx->cursor;
yy49:
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
	case 'z':	goto yy48;
	default:	goto yy50;
	}
yy50:
#line 101 "scan.re2c"
	{ TOK(IDENT); }
#line 365 "scan.c"
yy51:
	++ctx->cursor;
#line 65 "scan.re2c"
	{ TOK(LBRACKET); }
#line 370 "scan.c"
yy53:
	++ctx->cursor;
#line 66 "scan.re2c"
	{ TOK(RBRACKET); }
#line 375 "scan.c"
yy55:
	++ctx->cursor;
#line 56 "scan.re2c"
	{ TOK(XOR_BW); }
#line 380 "scan.c"
yy57:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy104;
	default:	goto yy49;
	}
yy58:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy105;
	default:	goto yy49;
	}
yy59:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy106;
	case 'o':	goto yy107;
	default:	goto yy49;
	}
yy60:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy108;
	case 'o':	goto yy109;
	default:	goto yy49;
	}
yy61:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy111;
	case 'n':	goto yy112;
	default:	goto yy49;
	}
yy62:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy113;
	case 'o':	goto yy114;
	case 'u':	goto yy115;
	default:	goto yy49;
	}
yy63:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy116;
	case 'm':	goto yy118;
	case 'n':	goto yy119;
	default:	goto yy49;
	}
yy64:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy121;
	default:	goto yy49;
	}
yy65:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy122;
	default:	goto yy49;
	}
yy66:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy123;
	default:	goto yy49;
	}
yy67:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy124;
	case 'r':	goto yy126;
	default:	goto yy49;
	}
yy68:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy128;
	default:	goto yy49;
	}
yy69:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy129;
	case 't':	goto yy130;
	default:	goto yy49;
	}
yy70:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'h':	goto yy131;
	case 'r':	goto yy132;
	default:	goto yy49;
	}
yy71:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy133;
	default:	goto yy49;
	}
yy72:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'h':	goto yy134;
	default:	goto yy49;
	}
yy73:
	++ctx->cursor;
#line 67 "scan.re2c"
	{ TOK(LBRACE); }
#line 491 "scan.c"
yy75:
	++ctx->cursor;
#line 55 "scan.re2c"
	{ TOK(OR_BW); }
#line 496 "scan.c"
yy77:
	++ctx->cursor;
#line 68 "scan.re2c"
	{ TOK(RBRACE); }
#line 501 "scan.c"
yy79:
	++ctx->cursor;
#line 58 "scan.re2c"
	{ TOK(NOT_BW); }
#line 506 "scan.c"
yy81:
	++ctx->cursor;
#line 42 "scan.re2c"
	{ TOK(NEQ); }
#line 511 "scan.c"
yy83:
	++ctx->cursor;
#line 69 "scan.re2c"
	{ TOK(RARROW); }
#line 516 "scan.c"
yy85:
	yych = *++ctx->cursor;
	switch (yych) {
	case '\n':	goto yy135;
	default:	goto yy85;
	}
yy87:
	yych = *++ctx->cursor;
yy88:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':	goto yy87;
	default:	goto yy34;
	}
yy89:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy137;
	default:	goto yy90;
	}
yy90:
	ctx->cursor = marker;
	goto yy34;
yy91:
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
	case 'f':	goto yy140;
	default:	goto yy90;
	}
yy92:
	++ctx->cursor;
#line 70 "scan.re2c"
	{ TOK(LARROW); }
#line 578 "scan.c"
yy94:
	++ctx->cursor;
#line 52 "scan.re2c"
	{ TOK(LSH); }
#line 583 "scan.c"
yy96:
	++ctx->cursor;
#line 45 "scan.re2c"
	{ TOK(LTEQ); }
#line 588 "scan.c"
yy98:
	++ctx->cursor;
#line 41 "scan.re2c"
	{ TOK(EQ); }
#line 593 "scan.c"
yy100:
	++ctx->cursor;
#line 46 "scan.re2c"
	{ TOK(GTEQ); }
#line 598 "scan.c"
yy102:
	++ctx->cursor;
#line 53 "scan.re2c"
	{ TOK(RSH); }
#line 603 "scan.c"
yy104:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy143;
	default:	goto yy49;
	}
yy105:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'g':	goto yy145;
	default:	goto yy49;
	}
yy106:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy146;
	default:	goto yy49;
	}
yy107:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy147;
	default:	goto yy49;
	}
yy108:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy148;
	default:	goto yy49;
	}
yy109:
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
	case 'z':	goto yy48;
	default:	goto yy110;
	}
yy110:
#line 90 "scan.re2c"
	{ TOK(KW_DO); }
#line 705 "scan.c"
yy111:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy150;
	case 's':	goto yy151;
	default:	goto yy49;
	}
yy112:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy152;
	default:	goto yy49;
	}
yy113:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy154;
	default:	goto yy49;
	}
yy114:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy155;
	default:	goto yy49;
	}
yy115:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy157;
	default:	goto yy49;
	}
yy116:
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
	case 'z':	goto yy48;
	default:	goto yy117;
	}
yy117:
#line 84 "scan.re2c"
	{ TOK(KW_IF); }
#line 808 "scan.c"
yy118:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'p':	goto yy159;
	default:	goto yy49;
	}
yy119:
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
	case 'z':	goto yy48;
	default:	goto yy120;
	}
yy120:
#line 83 "scan.re2c"
	{ TOK(KW_IN); }
#line 886 "scan.c"
yy121:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy160;
	default:	goto yy49;
	}
yy122:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'd':	goto yy162;
	default:	goto yy49;
	}
yy123:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy163;
	default:	goto yy49;
	}
yy124:
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
	case 'z':	goto yy48;
	default:	goto yy125;
	}
yy125:
#line 89 "scan.re2c"
	{ TOK(KW_OF); }
#line 976 "scan.c"
yy126:
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
	case 'z':	goto yy48;
	default:	goto yy127;
	}
yy127:
#line 98 "scan.re2c"
	{ TOK(KW_OR); }
#line 1048 "scan.c"
yy128:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'b':	goto yy165;
	case 'r':	goto yy167;
	default:	goto yy49;
	}
yy129:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy168;
	default:	goto yy49;
	}
yy130:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'a':	goto yy169;
	case 'r':	goto yy170;
	default:	goto yy49;
	}
yy131:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy171;
	default:	goto yy49;
	}
yy132:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy172;
	default:	goto yy49;
	}
yy133:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy173;
	default:	goto yy49;
	}
yy134:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy175;
	default:	goto yy49;
	}
yy135:
	++ctx->cursor;
#line 31 "scan.re2c"
	{
        ++ctx->line;
        goto next_token_start;
    }
#line 1100 "scan.c"
yy137:
	yych = *++ctx->cursor;
	switch (yych) {
	case '0':
	case '1':	goto yy137;
	default:	goto yy139;
	}
yy139:
#line 102 "scan.re2c"
	{ TOK(BIN); }
#line 1111 "scan.c"
yy140:
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
	case 'f':	goto yy140;
	default:	goto yy142;
	}
yy142:
#line 105 "scan.re2c"
	{ TOK(HEX); }
#line 1142 "scan.c"
yy143:
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
	case 'z':	goto yy48;
	default:	goto yy144;
	}
yy144:
#line 97 "scan.re2c"
	{ TOK(KW_AND); }
#line 1214 "scan.c"
yy145:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy176;
	default:	goto yy49;
	}
yy146:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy177;
	default:	goto yy49;
	}
yy147:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy179;
	default:	goto yy49;
	}
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
	case 'z':	goto yy48;
	default:	goto yy149;
	}
yy149:
#line 78 "scan.re2c"
	{ TOK(KW_DEF); }
#line 1304 "scan.c"
yy150:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy180;
	default:	goto yy49;
	}
yy151:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy182;
	default:	goto yy49;
	}
yy152:
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
	case 'z':	goto yy48;
	default:	goto yy153;
	}
yy153:
#line 72 "scan.re2c"
	{ TOK(KW_END); }
#line 1388 "scan.c"
yy154:
	yych = *++ctx->cursor;
	switch (yych) {
	case 's':	goto yy184;
	default:	goto yy49;
	}
yy155:
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
	case 'z':	goto yy48;
	default:	goto yy156;
	}
yy156:
#line 91 "scan.re2c"
	{ TOK(KW_FOR); }
#line 1466 "scan.c"
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
	case 'z':	goto yy48;
	default:	goto yy158;
	}
yy158:
#line 77 "scan.re2c"
	{ TOK(KW_FUN); }
#line 1538 "scan.c"
yy159:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'o':	goto yy185;
	default:	goto yy49;
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
	case 'z':	goto yy48;
	default:	goto yy161;
	}
yy161:
#line 82 "scan.re2c"
	{ TOK(KW_LET); }
#line 1616 "scan.c"
yy162:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy186;
	default:	goto yy49;
	}
yy163:
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
	case 'z':	goto yy48;
	default:	goto yy164;
	}
yy164:
#line 99 "scan.re2c"
	{ TOK(KW_NOT); }
#line 1694 "scan.c"
yy165:
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
	case 'z':	goto yy48;
	default:	goto yy166;
	}
yy166:
#line 79 "scan.re2c"
	{ TOK(KW_PUB); }
#line 1766 "scan.c"
yy167:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy187;
	default:	goto yy49;
	}
yy168:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'f':	goto yy189;
	default:	goto yy49;
	}
yy169:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy191;
	default:	goto yy49;
	}
yy170:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'u':	goto yy192;
	default:	goto yy49;
	}
yy171:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy193;
	default:	goto yy49;
	}
yy172:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy195;
	default:	goto yy49;
	}
yy173:
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
	case 'z':	goto yy48;
	default:	goto yy174;
	}
yy174:
#line 81 "scan.re2c"
	{ TOK(KW_VAR); }
#line 1874 "scan.c"
yy175:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy197;
	default:	goto yy49;
	}
yy176:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'n':	goto yy198;
	default:	goto yy49;
	}
yy177:
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
	case 'z':	goto yy48;
	default:	goto yy178;
	}
yy178:
#line 88 "scan.re2c"
	{ TOK(KW_CASE); }
#line 1958 "scan.c"
yy179:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy200;
	default:	goto yy49;
	}
yy180:
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
	case 'z':	goto yy48;
	default:	goto yy181;
	}
yy181:
#line 86 "scan.re2c"
	{ TOK(KW_ELIF); }
#line 2036 "scan.c"
yy182:
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
	case 'z':	goto yy48;
	default:	goto yy183;
	}
yy183:
#line 87 "scan.re2c"
	{ TOK(KW_ELSE); }
#line 2108 "scan.c"
yy184:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy202;
	default:	goto yy49;
	}
yy185:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'r':	goto yy204;
	default:	goto yy49;
	}
yy186:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'l':	goto yy205;
	default:	goto yy49;
	}
yy187:
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
	case 'z':	goto yy48;
	default:	goto yy188;
	}
yy188:
#line 94 "scan.re2c"
	{ TOK(KW_PURE); }
#line 2198 "scan.c"
yy189:
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
	case 'z':	goto yy48;
	default:	goto yy190;
	}
yy190:
#line 76 "scan.re2c"
	{ TOK(KW_SELF); }
#line 2270 "scan.c"
yy191:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'i':	goto yy206;
	default:	goto yy49;
	}
yy192:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy207;
	default:	goto yy49;
	}
yy193:
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
	case 'z':	goto yy48;
	default:	goto yy194;
	}
yy194:
#line 85 "scan.re2c"
	{ TOK(KW_THEN); }
#line 2354 "scan.c"
yy195:
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
	case 'z':	goto yy48;
	default:	goto yy196;
	}
yy196:
#line 95 "scan.re2c"
	{ TOK(KW_TRUE); }
#line 2426 "scan.c"
yy197:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy208;
	default:	goto yy49;
	}
yy198:
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
	case 'z':	goto yy48;
	default:	goto yy199;
	}
yy199:
#line 71 "scan.re2c"
	{ TOK(KW_BEGIN); }
#line 2504 "scan.c"
yy200:
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
	case 'z':	goto yy48;
	default:	goto yy201;
	}
yy201:
#line 80 "scan.re2c"
	{ TOK(KW_CONST); }
#line 2576 "scan.c"
yy202:
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
	case 'z':	goto yy48;
	default:	goto yy203;
	}
yy203:
#line 96 "scan.re2c"
	{ TOK(KW_FALSE); }
#line 2648 "scan.c"
yy204:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy210;
	default:	goto yy49;
	}
yy205:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'e':	goto yy212;
	default:	goto yy49;
	}
yy206:
	yych = *++ctx->cursor;
	switch (yych) {
	case 'c':	goto yy214;
	default:	goto yy49;
	}
yy207:
	yych = *++ctx->cursor;
	switch (yych) {
	case 't':	goto yy216;
	default:	goto yy49;
	}
yy208:
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
	case 'z':	goto yy48;
	default:	goto yy209;
	}
yy209:
#line 92 "scan.re2c"
	{ TOK(KW_WHILE); }
#line 2744 "scan.c"
yy210:
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
	case 'z':	goto yy48;
	default:	goto yy211;
	}
yy211:
#line 74 "scan.re2c"
	{ TOK(KW_IMPORT); }
#line 2816 "scan.c"
yy212:
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
	case 'z':	goto yy48;
	default:	goto yy213;
	}
yy213:
#line 73 "scan.re2c"
	{ TOK(KW_MODULE); }
#line 2888 "scan.c"
yy214:
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
	case 'z':	goto yy48;
	default:	goto yy215;
	}
yy215:
#line 93 "scan.re2c"
	{ TOK(KW_STATIC); }
#line 2960 "scan.c"
yy216:
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
	case 'z':	goto yy48;
	default:	goto yy217;
	}
yy217:
#line 75 "scan.re2c"
	{ TOK(KW_STRUCT); }
#line 3032 "scan.c"
}
#line 106 "scan.re2c"

}
