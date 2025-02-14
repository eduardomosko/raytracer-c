#include <ctype.h>
#include <string.h>

#define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-o", binary_path, source_path, "src/msk.c", "-g"
#define NOB_IMPLEMENTATION
#include "nob.h"

#include "src/msk.h"

#define CC "cc", "-I", "./src", "-Ofast", "-flto", "-fopenmp", "-ftree-vectorize"
// #define CC "clang", "-O3"
// #define CC "./tcc", "-L", "../tinycc/", "-I", "../tinycc/include/", "-O3"

#define CFLAGS "-Wall", "-Wextra", "-lm"

#define arr(arr) arr, NOB_ARRAY_LEN(arr)

const char* build_path = "./build";

const char* rt_program = "main";
const char* rt_srcs[] = {"src/main.c", "src/msk.h", "src/msk.c"};

const char* test_program = "test";
const char* test_srcs[] = {"tests/fmt.c", "src/msk.h", "src/msk.c"};

bool is_define(char* str, char* start) {
	bool  before_define = true;
	bool  before_hash = true;
	bool  seen_hash = false;
	bool  before_nl = true;
	char* ref = "\0define";

	// compare with define
	ref += slen(ref + 1);

	// parse back to front
	while (str > start) {
		// until define starts
		if (before_define && isspace(*str)) {
			str -= 1;
			continue;
		}
		before_define = false;
		// define
		if (*ref != '\0') {
			if (*ref == *str) {
				ref -= 1;
				str -= 1;
				continue;
			}
			return false;
		}
		// until hash
		if (before_hash && isspace(*str)) {
			str -= 1;
			continue;
		}
		before_hash = false;

		// #
		if (!seen_hash) {
			if (*str != '#') {
				return false;
			}
			str -= 1;
			seen_hash = true;
			continue;
		}

		// before newline
		if (isspace(*str) && *str != '\n') {
			str -= 1;
			continue;
		}

		if (*str != '\n') {
			return false;
		}

		return true;
	}
	return false;
}

Nob_String_View get_inside_p(const char* to_resolve) {
	u64 count = 1;

	for (u64 i = 0; to_resolve[i] != '\0'; i++) {
		switch (to_resolve[i]) {
			case '(':
				count += 1;
				break;
			case ')':
				count -= 1;
				break;
		}
		if (count == 0) {
			return nob_sv_from_parts(to_resolve, i);
		}
	}

	return (Nob_String_View){0};
}

typedef enum {
	TT_EOF,
	TT_ILLEGAL,

	TT_LITERAL,		// number
	TT_IDENTIFIER,	// name

	TT_LPAREN,	// (
	TT_RPAREN,	// )
	TT_COMMA,	// ,

	TT_PLUS,   // +
	TT_MINUS,  // -
	TT_DIV,	   // /
	TT_MUL,	   // /

	TT_DOT,	 // .
} token_type_t;

const char* token_type_name[] = {
	"TT_EOF",	"TT_ILLEGAL", "TT_LITERAL", "TT_IDENTIFIER", "TT_LPAREN", "TT_RPAREN",
	"TT_COMMA", "TT_PLUS",	  "TT_MINUS",	"TT_DIV",		 "TT_MUL",	  "TT_DOT",
};

typedef struct {
	token_type_t	type;
	Nob_String_View literal;
} token_t;

#define da_t(type_t)      \
	struct {              \
		type_t* items;    \
		u64		count;    \
		u64		capacity; \
	}

typedef struct {
	token_t* items;
	u64		 count;
} token_view_t;

token_t create_token(token_type_t type, const char* data, u64 len) {
	return (token_t){type, nob_sv_from_parts(data, len)};
}

typedef struct {
	Nob_String_View sv;
	u64				position;
	u64				readPosition;
	char			curr;
	const char*		ptr;
} lexer_t;

void lexer_next_char(lexer_t* lx) {
	lx->ptr = 0;
	lx->curr = 0;
	if (lx->readPosition < lx->sv.count) {
		lx->ptr = &lx->sv.data[lx->readPosition];
		lx->curr = *lx->ptr;
	}
	lx->position = lx->readPosition;
	lx->readPosition++;
}

token_t lexer_read_identifier(lexer_t* lx) {
	const u64	position = lx->position;
	const char* start = lx->ptr;

	while (isalpha(lx->curr) || isdigit(lx->curr) || lx->curr == '.' || lx->curr == '_') {
		lexer_next_char(lx);
	}

	return create_token(TT_IDENTIFIER, start, lx->position - position);
}

token_t lexer_read_literal(lexer_t* lx) {
	const u64	position = lx->position;
	const char* start = lx->ptr;
	bool		has_dot = false;

	while (isdigit(lx->curr) || lx->curr == '.') {
		if (lx->curr == '.') {
			if (has_dot) {
				return create_token(TT_ILLEGAL, start, lx->position - position);
			}
			has_dot = true;
		}

		lexer_next_char(lx);
	}

	if (isalpha(lx->curr)) {
		lexer_next_char(lx);
	}

	return create_token(TT_LITERAL, start, lx->position - position);
}

token_t lexer_next_token(lexer_t* lx) {
	token_t token;

	while (isspace(lx->curr)) {
		lexer_next_char(lx);
	}

	switch (lx->curr) {
		case '+': {
			token = create_token(TT_PLUS, lx->ptr, 1);
		}; break;
		case '-': {
			token = create_token(TT_MINUS, lx->ptr, 1);
		}; break;
		case '*': {
			token = create_token(TT_MUL, lx->ptr, 1);
		}; break;
		case '/': {
			token = create_token(TT_DIV, lx->ptr, 1);
		}; break;
		case '(': {
			token = create_token(TT_LPAREN, lx->ptr, 1);
		}; break;
		case ')': {
			token = create_token(TT_RPAREN, lx->ptr, 1);
		}; break;
		case ',': {
			token = create_token(TT_COMMA, lx->ptr, 1);
		}; break;
		case '\0': {
			token = create_token(TT_EOF, lx->ptr, 1);
		}; break;
		default: {
			if (isalpha(lx->curr) || isdigit(lx->curr) || lx->curr == '.' || lx->curr == '_') {
				return lexer_read_identifier(lx);
			}
		}; break;
	}

	lexer_next_char(lx);
	return token;
}

Nob_String_Builder transpile_tokens(token_view_t tokens) {
	da_t(char*) to_free = {0};

	// eval parenthesis
	for (u64 i = 0; i < tokens.count; i++) {
		token_t token = tokens.items[i];
		nob_log(NOB_INFO, "p: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));

		// find open
		if (token.type != TT_LPAREN) {
			continue;
		}
		u64 len = 0;
		u64 count = 1;

		// find close
		while (count != 0) {
			len++;
			try i + len > tokens.count or_fail("bad parsing");
			token = tokens.items[i + len];
			if (token.type == TT_LPAREN) {
				count++;
			} else if (token.type == TT_RPAREN) {
				count--;
			}
			nob_log(NOB_INFO, "rp: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));
		}

		Nob_String_Builder substr = transpile_tokens((token_view_t){tokens.items + i + 1, len});
		nob_da_append(&to_free, substr.items);

		// substitute tokens that have been condensed
		memmove(&tokens.items[i + 1], &tokens.items[i + len + 1], (tokens.count - (i + len + 1)) * sizeof(token_t));
		tokens.count = tokens.count - len + 1;
		tokens.items[i] = create_token(TT_LITERAL, substr.items, substr.count);
	}

	// eval first priority
	for (u64 i = 1; i + 1 < tokens.count;) {
		token_t token = tokens.items[i];

		nob_log(NOB_INFO, "2p: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));

		// find * and /
		if (token.type != TT_MUL && token.type != TT_DIV) {
			i++;
			continue;
		}

		token_t lhs = tokens.items[i - 1];
		token_t rhs = tokens.items[i + 1];

		nob_log(NOB_INFO, "found 2p: %s(" SV_Fmt ") %s(" SV_Fmt ") %s(" SV_Fmt ")", token_type_name[lhs.type],
				SV_Arg(lhs.literal), token_type_name[token.type], SV_Arg(token.literal), token_type_name[rhs.type],
				SV_Arg(rhs.literal));

		Nob_String_Builder sb = {0};
		if (token.type == TT_MUL) {
			nob_sb_append_cstr(&sb, "vec3_mul(");
		} else {
			nob_sb_append_cstr(&sb, "vec3_div(");
		}
		nob_sb_append_buf(&sb, lhs.literal.data, lhs.literal.count);
		nob_sb_append_cstr(&sb, ", ");
		nob_sb_append_buf(&sb, rhs.literal.data, rhs.literal.count);
		nob_sb_append_cstr(&sb, ")");
		nob_da_append(&to_free, sb.items);

		// substitute tokens that have been condensed
		memmove(&tokens.items[i], &tokens.items[i + 2], (tokens.count - (i + 1)) * sizeof(token_t));
		tokens.count = tokens.count - 2;
		tokens.items[i - 1] = create_token(TT_LITERAL, sb.items, sb.count);
	}

	// eval second priority
	for (u64 i = 1; i + 1 < tokens.count;) {
		token_t token = tokens.items[i];

		nob_log(NOB_INFO, "3p: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));

		// find + and -
		if (token.type != TT_PLUS && token.type != TT_MINUS) {
			i++;
			continue;
		}

		token_t lhs = tokens.items[i - 1];
		token_t rhs = tokens.items[i + 1];

		nob_log(NOB_INFO, "found 2p: %s(" SV_Fmt ") %s(" SV_Fmt ") %s(" SV_Fmt ")", token_type_name[lhs.type],
				SV_Arg(lhs.literal), token_type_name[token.type], SV_Arg(token.literal), token_type_name[rhs.type],
				SV_Arg(rhs.literal));

		Nob_String_Builder sb = {0};
		if (token.type == TT_PLUS) {
			nob_sb_append_cstr(&sb, "vec3_add(");
		} else {
			nob_sb_append_cstr(&sb, "vec3_sub(");
		}
		nob_sb_append_buf(&sb, lhs.literal.data, lhs.literal.count);
		nob_sb_append_cstr(&sb, ", ");
		nob_sb_append_buf(&sb, rhs.literal.data, rhs.literal.count);
		nob_sb_append_cstr(&sb, ")");
		nob_da_append(&to_free, sb.items);

		// substitute tokens that have been condensed
		memmove(&tokens.items[i], &tokens.items[i + 2], (tokens.count - (i + 1)) * sizeof(token_t));
		tokens.count = tokens.count - 2;
		tokens.items[i - 1] = create_token(TT_LITERAL, sb.items, sb.count);
	}

	nob_log(NOB_INFO, "count: %d", tokens.count);
	for (u64 i = 0; i < tokens.count; i++) {
		token_t token = tokens.items[i];
		nob_log(NOB_INFO, "here: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));
	}

	token_t token = tokens.items[0];
	assert(token.type == TT_LITERAL);

	Nob_String_Builder result = {0};
	nob_da_append_many(&result, token.literal.data, token.literal.count);
	nob_log(NOB_INFO, "res: \"" SV_Fmt "\"", result.count, result.items);

	for (u64 i = 0; i < to_free.count; i++) {
		NOB_FREE(to_free.items[i]);
	}

	return result;
}

Nob_String_Builder transpile_math(Nob_String_View sv) {
	lexer_t lx = {.sv = sv};
	da_t(token_t) tokens = {0};

	lexer_next_char(&lx);

	while (lx.curr) {
		token_t token = lexer_next_token(&lx);
		nob_da_append(&tokens, token);

		nob_log(NOB_INFO, "tok: %s - \"" SV_Fmt "\"", token_type_name[token.type], SV_Arg(token.literal));
	}

	Nob_String_Builder res = transpile_tokens((token_view_t){tokens.items, tokens.count});

	nob_da_free(tokens);
	return res;
}

void apply_vecmath(const char** files, u64 len) {
	for (u64 i = 0; i < len; i++) {
		const char* file = files[i];
		const char* output = nob_temp_sprintf("%s/%s", build_path, file);

		if (!nob_needs_rebuild1(output, file)) {
			continue;
		}

		Nob_String_Builder src = {0};
		try !nob_read_entire_file(file, &src) or_failf("unable to read file: %s", file);
		nob_sb_append_null(&src);

		bool  found_anything = false;
		char* start = src.items;
		char* curr = src.items;
		char* found = null;

		const char* vecmath = "vecmath" "(";
		u64			next = slen(vecmath);

		Nob_String_Builder out = {0};

		while (true) {
			char* found = strstr(curr, vecmath);
			if (found == null) {
				break;
			}

			if (is_define(found - 1, start)) {
				// ignore
				nob_sb_append_buf(&out, curr, (found + next) - curr);
				curr = found + next;
				continue;
			}

			found_anything = true;

			// write to output
			nob_sb_append_buf(&out, curr, found - curr);
			found += next;

			Nob_String_View sv = get_inside_p(found);
			nob_log(NOB_INFO, "transpile: " SV_Fmt, SV_Arg(sv));

			Nob_String_Builder transpiled = transpile_math(sv);
			nob_sb_append_buf(&out, transpiled.items, transpiled.count);
			nob_sb_free(transpiled);

			curr = found + sv.count + 1;
		};

		if (found_anything) {
			char* end = src.items + src.count;
			nob_sb_append_buf(&out, curr, end - curr);
			nob_sb_append_cstr(&out, "\n");

			nob_write_entire_file(output, out.items, out.count -2);

			files[i] = output;
		}

		nob_sb_free(src);
		nob_sb_free(out);
	}
}


i32 main(i32 argc, char** argv) {
	NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "src/msk.h", "src/msk.c");

	char* program = nob_shift_args(&argc, &argv);
	char* command = argc ? nob_shift_args(&argc, &argv) : "";

	Nob_Cmd cmd = {0};

	try !nob_mkdir_if_not_exists(build_path) or_fail("failed to create build dir");
	try !nob_mkdir_if_not_exists("build/src") or_fail("failed to create build/src dir");
	try !nob_mkdir_if_not_exists("build/tests") or_fail("failed to create build/tests dir");

	apply_vecmath(arr(rt_srcs));
	const char *rt_output = nob_temp_sprintf("%s/%s", build_path, rt_program);

	if (nob_needs_rebuild(rt_output, rt_srcs, NOB_ARRAY_LEN(rt_srcs))) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", rt_output);
		nob_da_append_many(&cmd, rt_srcs, NOB_ARRAY_LEN(rt_srcs));

		try !nob_cmd_run_sync_and_reset(&cmd) or_fail("failed to compile rt");
	}

	apply_vecmath(arr(test_srcs));
	const char *test_output = nob_temp_sprintf("%s/%s", build_path, test_program);

	if (nob_needs_rebuild(test_output, test_srcs, NOB_ARRAY_LEN(test_srcs))) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", test_output);
		nob_da_append_many(&cmd, test_srcs, NOB_ARRAY_LEN(test_srcs));

		try !nob_cmd_run_sync_and_reset(&cmd) or_fail("failed to compile tests");
	}

	if (!strcmp(command, "run")) {
		nob_cmd_append(&cmd, rt_output);
		try !nob_cmd_run_sync(cmd) or_fail("./main returned bad status code");

		return 0;
	}

	if (!strcmp(command, "test")) {
		nob_cmd_append(&cmd, test_output);
		try !nob_cmd_run_sync(cmd) or_fail("TEST FAILED");

		return 0;
	}

	nob_temp_reset();
	return 0;
}
