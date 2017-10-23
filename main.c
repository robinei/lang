#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int uint;

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;



enum {
	OPERATOR_ADD,
	OPERATOR_SUB,
	OPERATOR_MUL,
	OPERATOR_DIV
};

enum {
	AST_LITERAL,
	AST_SYMBOL,
	AST_BINOP,
	AST_APPLY
};

struct ast_node;

struct ast_literal {
	slice_t value;
};

struct ast_symbol {
	slice_t value;
};

struct ast_binop {
	int op;
	struct ast_node *lhs, *rhs;
};

struct ast_apply {
	struct ast_node *func;
	struct ast_node **args;
	uint arg_count;
};

struct ast_node {
	int type;
	union {
		struct ast_literal literal;
		struct ast_symbol symbol;
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

#define ERROR(fmt, ...) \
    do { \
        snprintf(ctx->error, ERROR_MAX, "(line %d:%d) syntax error: " fmt ":\n%.60s...", \
            ctx->line + 1, ctx->col + 1, __VA_ARGS__, ctx->pos); \
        return false; \
    } while (false)

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

#define EXPECT_IDENT(slice, context_desc) \
    do { \
        (slice).ptr = &PEEK(0); \
        switch (PEEK(0)) { \
        case '_': \
        case LOWER_LETTER: \
        case UPPER_LETTER: \
            STEP(1); \
            break; \
        default: \
            ERROR("expected valid identifier %s", context_desc); \
        } \
        while (true) { \
            switch (PEEK(0)) { \
                case '_': \
                case DIGIT: \
                case LOWER_LETTER: \
                case UPPER_LETTER: \
                    STEP(1); \
                    continue; \
                default: \
                    break; \
            } \
            break; \
        } \
        (slice).len = (uint)(&PEEK(0) - (slice).ptr); \
    } while(false)


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
	printf("num: '%.*s'\n", (int)(end - start), start);
	return true;
}

static bool parse_expr(struct parse_ctx *ctx, int min_precedence);

static bool parse_atom(struct parse_ctx *ctx) {
	if (MATCH_CHAR('(')) {
		SKIP();
		if (!parse_expr(ctx, 1)) {
			return false;
		}
		EXPECT_CHAR(')', "after expression");
		SKIP();
		return true;
	}

	switch (PEEK(0)) {
	case '_':
	case LOWER_LETTER:
	case UPPER_LETTER: {
		slice_t var_name;
		EXPECT_IDENT(var_name, "in expression");
		printf("var: '%.*s'\n", var_name.len, var_name.ptr);
		SKIP();
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
		case '+': precedence = 1; op = OPERATOR_ADD; break;
		case '-': precedence = 1; op = OPERATOR_SUB; break;
		case '*': precedence = 2; op = OPERATOR_MUL; break;
		case '/': precedence = 2; op = OPERATOR_DIV; break;
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

static bool parse_type(struct parse_ctx *ctx) {
	slice_t type_name;
	EXPECT_IDENT(type_name, "for type");
	printf("type: '%.*s'\n", type_name.len, type_name.ptr);
	return true;
}

static bool parse_func(struct parse_ctx *ctx) {
	EXPECT_SKIP("after 'func'");

	slice_t func_name;
	EXPECT_IDENT(func_name, "as name in function declaration");
	printf("func: '%.*s'\n", func_name.len, func_name.ptr);

	SKIP();
	EXPECT_CHAR('(', "after function name");
	SKIP();

	if (MATCH_CHAR(')')) {
		SKIP();
	}
	else {
		while (true) {
			slice_t param_name;
			EXPECT_IDENT(param_name, "as parameter name in function declaration");
			printf("param: '%.*s'\n", param_name.len, param_name.ptr);

			SKIP();
			EXPECT_CHAR(':', "after parameter name");
			SKIP();

			if (!parse_type(ctx)) {
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
			UNEXPECTED("in function parameter list");
		}
	}

	if (MATCH("end")) {
		SKIP();
	}
	else {
		if (!parse_expr(ctx, 1)) {
			return false;
		}
		EXPECT("end", "after function body");
		SKIP();
	}

	return true;
}

static bool parse_module(struct parse_ctx *ctx) {
    SKIP();
    EXPECT("module", "at top of file");
    EXPECT_SKIP("after 'module'");
	slice_t module_name;
    EXPECT_IDENT(module_name, "as name in module declaration");
	printf("module: '%.*s'\n", module_name.len, module_name.ptr);
    SKIP();
    EXPECT_CHAR(';', "after module name");
    SKIP();

    if (MATCH("import")) {
        EXPECT_SKIP("after 'import'");

        while (true) {
            slice_t import_name;
            EXPECT_IDENT(import_name, "in import list");
			printf("import: '%.*s'\n", import_name.len, import_name.ptr);
            
            SKIP();
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

	if (MATCH("type")) {
		EXPECT_SKIP("after 'type'");

		while (true) {
			slice_t type_name;
			EXPECT_IDENT(type_name, "in type declaration");
			printf("type name: '%.*s'\n", type_name.len, type_name.ptr);

			SKIP();
			EXPECT_CHAR('=', "after type name");
			SKIP();

			if (!parse_type(ctx)) {
				return false;
			}

			if (MATCH_CHAR(',')) {
				SKIP();
				continue;
			}
			if (MATCH_CHAR(';')) {
				SKIP();
				break;
			}
			UNEXPECTED("in type declaration");
		}
	}

    while (true) {
        if (MATCH("end")) {
            break;
        }
        if (MATCH("func")) {
			if (!parse_func(ctx)) {
				return false;
			}
			continue;
        }
        UNEXPECTED("while parsing module");
    }

    return true;
}


int main(int argc, char *argv[]) {
	char *text =

		"module Test;\n"
		"import IO, String;\n"

		"type\n"
		"  TFoo = int,\n"
		"  TVar = int;\n"

		"func Foo(x: int, y: int)\n"
		"  Foo(asdf, (12), 0x32, 1.123+9*(3+10))\n"
		"end\n"

		"end\n";

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