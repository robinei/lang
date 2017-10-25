#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int uint;

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;

static int slice_eq_str(slice_t s, const char *str) {
	int len = strlen(str);
	if (s.len != len) {
		return false;
	}
	return !memcmp(s.ptr, str, len);
}

enum {
	UNOP_PLUS,
	UNOP_NEGATE,
	UNOP_LOGI_NOT,
	UNOP_BITWISE_NOT,
};

enum {
	BINOP_SEQ,

	BINOP_LOGI_OR,

	BINOP_LOGI_AND,

	BINOP_BITWISE_OR,

	BINOP_BITWISE_XOR,

	BINOP_BITWISE_AND,

	BINOP_EQ,
	BINOP_NEQ,

	BINOP_LT,
	BINOP_GT,
	BINOP_LTEQ,
	BINOP_GTEQ,

	BINOP_BITWISE_LSH,
	BINOP_BITWISE_RSH,

	BINOP_ADD,
	BINOP_SUB,

	BINOP_MUL,
	BINOP_DIV,
	BINOP_MOD,
};

enum {
	AST_LIT,
	AST_SYM,
	AST_UNOP,
	AST_BINOP,
	AST_APPLY,
};

struct ast_node;

struct ast_lit {
	slice_t value;
};

struct ast_sym {
	slice_t value;
};

struct ast_unop {
	uint type;
	struct ast_node *arg;
};

struct ast_binop {
	uint type;
	struct ast_node *lhs, *rhs;
};

struct ast_apply {
	struct ast_node *func;
	struct ast_node **args;
	uint arg_count;
};

struct ast_node {
	uint type;
	union {
		struct ast_lit lit;
		struct ast_sym sym;
		struct ast_unop unop;
		struct ast_binop binop;
		struct ast_apply apply;
	} u;
};



#define ERROR_MAX 512

struct parse_ctx {
    char *pos;
    uint line;
    uint col;

    char error[ERROR_MAX];
};

#define ERROR_AT(pos, fmt, ...) \
    do { \
        snprintf(ctx->error, ERROR_MAX, "(line %d:%d) syntax error: " fmt ":\n%.60s...", \
            ctx->line + 1, ctx->col + 1, __VA_ARGS__, pos); \
        return false; \
    } while (false)

#define ERROR(fmt, ...) \
	ERROR_AT(ctx->pos, fmt, __VA_ARGS__)

#define UNEXPECTED(context_desc) \
    do { \
        if (PEEK(0)) ERROR("unexpected input %s", context_desc); \
        ERROR("unexpected end of file %s", context_desc); \
    } while(false)

#define PEEK(n) (*(ctx->pos + (n)))
#define STEP(n) do { ctx->pos += (n); ctx->col += (n); } while(false)

/* skip whitespace and comments, while paying attention to newlines */
#define SKIP() \
    do { \
        switch (PEEK(0)) { \
        case '\r': \
            if (PEEK(1) == '\n') { \
                STEP(1); \
                continue; \
            } \
        case '\n': \
            ++ctx->line; \
            ctx->col = -1; \
        case ' ': \
        case '\t': \
            STEP(1); \
            continue; \
        case '/': \
            if (PEEK(1) == '/') { \
                STEP(2); \
                do { \
                    char ch = PEEK(0); \
                    if (ch == '\r' || ch == '\n' || ch == '\0') break; \
                    STEP(1); \
                } while(true); \
                continue; \
            } \
        default: \
            break; \
        } \
        break; \
    } while(true)

#define EXPECT_SKIP(context_desc) \
    do { \
        char *pos = ctx->pos; \
        SKIP(); \
        if (pos == ctx->pos) ERROR("expected whitespace %s", context_desc); \
    } while(false)

#define EXPECT(str, context_desc) \
    do { \
        uint i = 0; \
        while (str[i]) { \
            if (PEEK(0) != str[i]) ERROR("expected '%s' %s", str, context_desc); \
            STEP(1); \
            ++i; \
        } \
    } while(false)

#define EXPECT_CHAR(ch, context_desc) \
    do { \
        if (PEEK(0) != ch) ERROR("expected '%c' %s", ch, context_desc); \
        STEP(1); \
    } while(false)

#define MATCH(str) do_match(ctx, str)
static inline bool do_match(struct parse_ctx *ctx, char *str) {
    uint len = strlen(str);
    if (strncmp(ctx->pos, str, len)) {
        return false;
    }
    STEP(len);
    return true;
}

#define MATCH_CHAR(ch) do_match_char(ctx, ch)
static inline bool do_match_char(struct parse_ctx *ctx, char ch) {
    if (*ctx->pos != ch) {
        return false;
    }
    STEP(1);
    return true;
}

#define LOWER_HEX_LETTER \
    'a': \
    case 'b': \
    case 'c': \
    case 'd': \
    case 'e': \
    case 'f'

#define LOWER_LETTER \
	LOWER_HEX_LETTER: \
    case 'g': \
    case 'h': \
    case 'i': \
    case 'j': \
    case 'k': \
    case 'l': \
    case 'm': \
    case 'n': \
    case 'o': \
    case 'p': \
    case 'q': \
    case 'r': \
    case 's': \
    case 't': \
    case 'u': \
    case 'v': \
    case 'w': \
    case 'x': \
    case 'y': \
    case 'z'

#define UPPER_HEX_LETTER \
    'A': \
    case 'B': \
    case 'C': \
    case 'D': \
    case 'E': \
    case 'F'

#define UPPER_LETTER \
	UPPER_HEX_LETTER: \
    case 'G': \
    case 'H': \
    case 'I': \
    case 'J': \
    case 'K': \
    case 'L': \
    case 'M': \
    case 'N': \
    case 'O': \
    case 'P': \
    case 'Q': \
    case 'R': \
    case 'S': \
    case 'T': \
    case 'U': \
    case 'V': \
    case 'W': \
    case 'X': \
    case 'Y': \
    case 'Z'

#define DIGIT \
    '0': \
    case '1': \
    case '2': \
    case '3': \
    case '4': \
    case '5': \
    case '6': \
    case '7': \
    case '8': \
    case '9'

enum {
	NOT_IDENT,
	IDENT,
	IDENT_MODULE,
	IDENT_IMPORT,
	IDENT_FN,
	IDENT_LET,
	IDENT_IN,
	IDENT_INT,
};

int parse_ident(struct parse_ctx *ctx, slice_t *slice) {
	slice->ptr = &PEEK(0);

	switch (PEEK(0)) {
	case '_':
	case LOWER_LETTER:
	case UPPER_LETTER:
		STEP(1);
		break;
	default:
		slice->len = 0;
		return NOT_IDENT;
	}

	while (true) {
		switch (PEEK(0)) {
		case '_':
		case DIGIT:
		case LOWER_LETTER:
		case UPPER_LETTER:
			STEP(1);
			continue;
		default:
			break;
		}
		break;
	}

	slice->len = (uint)(&PEEK(0) - slice->ptr);
	SKIP();

	switch (slice->ptr[0]) {
	case 'f':
		if (slice_eq_str(*slice, "fn")) { return IDENT_FN; }
		break;
	case 'i':
		if (slice_eq_str(*slice, "in")) { return IDENT_IN; }
		if (slice_eq_str(*slice, "int")) { return IDENT_INT; }
		if (slice_eq_str(*slice, "import")) { return IDENT_MODULE; }
		break;
	case 'l':
		if (slice_eq_str(*slice, "let")) { return IDENT_LET; }
		break;
	case 'm':
		if (slice_eq_str(*slice, "module")) { return IDENT_MODULE; }
		break;
	}

	return IDENT;
}



static bool parse_number(struct parse_ctx *ctx) {
	char *start = &PEEK(0);
	char *end;

	if (PEEK(0) == '0' && PEEK(1) == 'x') {
		STEP(2);
		while (true) {
			switch (PEEK(0)) {
			case DIGIT:
			case LOWER_HEX_LETTER:
			case UPPER_HEX_LETTER:
				STEP(1);
				continue;
			}
			break;
		}
	}
	else {
		while (true) {
			switch (PEEK(0)) {
			case DIGIT:
				STEP(1);
				continue;
			case '.':
				STEP(1);
				while (true) {
					switch (PEEK(0)) {
					case DIGIT:
						STEP(1);
						continue;
					}
					break;
				}
				break;
			}
			break;
		}
	}

	end = &PEEK(0);
	SKIP();

	printf("num: '%.*s'\n", (int)(end - start), start);
	return true;
}

static bool parse_expr(struct parse_ctx *ctx, int min_precedence);

static bool parse_type(struct parse_ctx *ctx) {
	slice_t type_name;
	switch (parse_ident(ctx, &type_name)) {
	case IDENT_INT:
		printf("type: '%.*s'\n", type_name.len, type_name.ptr);
		return true;
	default:
		ERROR_AT(type_name.ptr, "unexpected input in type expression");
	}
}

static bool parse_fn(struct parse_ctx *ctx) {
	EXPECT_CHAR('(', "after 'fn'");
	SKIP();

	if (MATCH_CHAR(')')) {
		SKIP();
	}
	else {
		while (true) {
			while (true) {
				slice_t param_name;
				switch (parse_ident(ctx, &param_name)) {
				case IDENT:
					printf("param: '%.*s'\n", param_name.len, param_name.ptr);
					break;
				default:
					ERROR_AT(param_name.ptr, "expected parameter name");
				}

				if (MATCH_CHAR(',')) {
					SKIP();
					continue;
				}
				if (MATCH_CHAR(':')) {
					SKIP();
					break;
				}
				UNEXPECTED("in function parameter list");
			}

			if (!parse_type(ctx)) {
				return false;
			}

			if (MATCH_CHAR(';')) {
				SKIP();
				continue;
			}
			if (MATCH_CHAR(')')) {
				SKIP();
				break;
			}
			UNEXPECTED("in function parameter list");
		}
	}

	if (!parse_type(ctx)) {
		return false;
	}

	EXPECT_CHAR(':', "after return type");
	SKIP();

	if (!parse_expr(ctx, 1)) {
		return false;
	}
	return true;
}

static bool parse_atom(struct parse_ctx *ctx) {
	char ch = PEEK(0);
	switch (ch) {
	case '(':
		STEP(1);
		SKIP();
		if (!parse_expr(ctx, 1)) {
			return false;
		}
		EXPECT_CHAR(')', "after expression");
		SKIP();
		return true;
	case '!':
		STEP(1);
		SKIP();
		if (!parse_atom(ctx)) {
			return false;
		}
		return true;
	case '~':
		STEP(1);
		SKIP();
		if (!parse_atom(ctx)) {
			return false;
		}
		return true;
	case '+':
		STEP(1);
		SKIP();
		if (!parse_atom(ctx)) {
			return false;
		}
		return true;
	case '-':
		STEP(1);
		SKIP();
		if (!parse_atom(ctx)) {
			return false;
		}
		return true;
	case '_':
	case LOWER_LETTER:
	case UPPER_LETTER: {
		slice_t ident;
		switch (parse_ident(ctx, &ident)) {
		case IDENT:
			printf("var: '%.*s'\n", ident.len, ident.ptr);
			return true;
		case IDENT_FN:
			printf("fn\n");
			if (!parse_fn(ctx)) {
				return false;
			}
			return true;
		case IDENT_LET:
			printf("fn\n");
			return true;
		default:
			ERROR_AT(ident.ptr, "unexpected identifier in expression");
		}
		return true;
	}
	case DIGIT:
		if (!parse_number(ctx)) {
			return false;
		}
		return true;
	}

	UNEXPECTED("while parsing expression atom");
}

static bool parse_expr(struct parse_ctx *ctx, int min_precedence) {
	if (!parse_atom(ctx)) {
		return false;
	}

	if (MATCH_CHAR('(')) { /* function call */
		SKIP();
		printf("funcall\n");

		if (MATCH_CHAR(')')) {
			SKIP();
		}
		else {
			while (true) {
				if (!parse_expr(ctx, 1)) {
					return false;
				}
				if (MATCH_CHAR(',')) {
					SKIP();
					continue;
				}
				if (MATCH_CHAR(')')) {
					SKIP();
					break;
				}
				UNEXPECTED("in function argument list");
			}
		}
	}

	while (true) {
		int op = -1, len = 1, precedence, next_min_precedence;
		bool left_assoc = true;
		char *pos = ctx->pos;
		
		switch (PEEK(0)) {
		case ';':
			if (PEEK(1) == ';') {
				STEP(2);
				SKIP();
				return true;
			}
			precedence = 1; op = BINOP_SEQ;
			break;
		case '|':
			if (PEEK(1) == '|') { precedence = 2; op = BINOP_LOGI_OR; len = 2; }
			else { precedence = 4; op = BINOP_BITWISE_OR; }
			break;
		case '&':
			if (PEEK(1) == '&') { precedence = 3; op = BINOP_LOGI_AND; len = 2; }
			else { precedence = 6; op = BINOP_BITWISE_AND; }
			break;
		case '^': precedence = 5; op = BINOP_BITWISE_XOR; break;
		case '=':
			if (PEEK(1) == '=') { precedence = 7; op = BINOP_EQ; len = 2; }
			break;
		case '!':
			if (PEEK(1) == '=') { precedence = 7; op = BINOP_NEQ; len = 2; }
			break;
		case '<':
			if (PEEK(1) == '=') { precedence = 8; op = BINOP_LTEQ; len = 2; }
			if (PEEK(1) == '<') { precedence = 9; op = BINOP_BITWISE_LSH; len = 2; }
			else { precedence = 8; op = BINOP_LT; }
			break;
		case '>':
			if (PEEK(1) == '=') { precedence = 8; op = BINOP_GTEQ; len = 2; }
			if (PEEK(1) == '>') { precedence = 9; op = BINOP_BITWISE_RSH; len = 2; }
			else { precedence = 8; op = BINOP_GT; }
			break;
		case '+': precedence = 10; op = BINOP_ADD; break;
		case '-': precedence = 10; op = BINOP_SUB; break;
		case '*': precedence = 11; op = BINOP_MUL; break;
		case '/': precedence = 11; op = BINOP_DIV; break;
		case '%': precedence = 11; op = BINOP_MOD; break;
		}

		if (op < 0 || precedence < min_precedence) {
			break;
		}

		STEP(len);
		SKIP();

		next_min_precedence = precedence + (left_assoc ? 1 : 0);
		if (!parse_expr(ctx, next_min_precedence)) {
			return false;
		}

		printf("op: '%.*s'\n", len, pos);
	}

	return true;
}

static bool parse_module(struct parse_ctx *ctx) {
    SKIP();
    EXPECT("module", "at top of file");
    EXPECT_SKIP("after 'module'");
	slice_t module_name;
	switch (parse_ident(ctx, &module_name)) {
	case IDENT:
		printf("module: '%.*s'\n", module_name.len, module_name.ptr);
		break;
	default:
		ERROR_AT(module_name.ptr, "expected module name");
	}
    EXPECT_CHAR(';', "after module name");
    SKIP();

    if (MATCH("import")) {
        EXPECT_SKIP("after 'import'");

        while (true) {
            slice_t import_name;
			switch (parse_ident(ctx, &import_name)) {
			case IDENT:
				printf("import: '%.*s'\n", import_name.len, import_name.ptr);
				break;
			default:
				ERROR_AT(import_name.ptr, "expected import name");
			}
            
            if (MATCH_CHAR(',')) {
                SKIP();
                continue;
            }
            if (MATCH_CHAR(';')) {
                SKIP();
                break;
            }
            UNEXPECTED("in import declaration");
        }
    }

    while (PEEK(0)) {
		slice_t ident;
		switch (parse_ident(ctx, &ident)) {
		case IDENT:
			printf("def top-level: '%.*s'\n", ident.len, ident.ptr);
			break;
		default:
			ERROR_AT(ident.ptr, "expected identifier at top level");
		}
		EXPECT_CHAR('=', "after identifier at top level");
		SKIP();

		if (!parse_expr(ctx, 1)) {
			return false;
		}
    }

    return true;
}


int main(int argc, char *argv[]) {
	char *text =

		"module Test;\n"
		"import IO, String;\n"

		"Foo = fn(x: int; y, z: int) int:\n"
		"  print(1); 1 + 2;;\n"

		"Bar = fn() int: 1;;"
		;

    struct parse_ctx ctx = {0,};
    ctx.pos = text;

    if (!parse_module(&ctx)) {
        printf("\nERROR %s\n", ctx.error);
    } else {
        printf("\nOK\n");
    }

	fgetc(stdin);
    return 0;
}