#include <ctype.h>

#define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-o", binary_path, source_path, "src/msk.c", "-g"
#define NOB_IMPLEMENTATION
#include "nob.h"

#include "src/msk.h"

#define CC "cc", "-g"
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

void apply_vecmath(const char** files, u64 len) {
	for (u64 i = 0; i < len; i++) {
		u64 ck = nob_temp_save();

		const char* file = files[i];
		const char* output = nob_temp_sprintf("build/%s", file);

		if (!nob_needs_rebuild1(output, file)) {
			continue;
		}

		Nob_String_Builder src = {0};
		try !nob_read_entire_file(file, &src) or_failf("unable to read file: %s", file);
		nob_sb_append_null(&src);

		char* start = src.items;
		char* curr = src.items;
		char* found = null;

		const char* vecmath = "vecmath" "(";
		u64			next = slen(vecmath);

		while (true) {
			char* found = strstr(curr, vecmath);
			if (found == null) {
				break;
			}

			if (is_define(found - 1, start)) {
				// ignore
				curr = found + next;
				continue;
			}

			found += next;
			Nob_String_View sv = get_inside_p(found);

			nob_log(NOB_INFO, "transpile: " SV_Fmt, SV_Arg(sv));
			curr = found + next;
		};

		nob_temp_rewind(ck);
	}
}

i32 main(i32 argc, char** argv) {
	NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "src/msk.h", "src/msk.c");

	char* program = nob_shift_args(&argc, &argv);
	char* command = argc ? nob_shift_args(&argc, &argv) : "";

	Nob_Cmd cmd = {0};

	try !nob_mkdir_if_not_exists(build_path) or_fail("failed to create build dir");

	apply_vecmath(arr(rt_srcs));

	if (nob_needs_rebuild(rt_program, rt_srcs, NOB_ARRAY_LEN(rt_srcs))) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", rt_program);
		nob_da_append_many(&cmd, rt_srcs, NOB_ARRAY_LEN(rt_srcs));

		try !nob_cmd_run_sync_and_reset(&cmd) or_fail("failed to compile rt");
	}

	if (nob_needs_rebuild(test_program, test_srcs, NOB_ARRAY_LEN(test_srcs))) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", test_program);
		nob_da_append_many(&cmd, test_srcs, NOB_ARRAY_LEN(test_srcs));

		try !nob_cmd_run_sync_and_reset(&cmd) or_fail("failed to compile tests");
	}

	if (!strcmp(command, "run")) {
		nob_cmd_append(&cmd, "./main");
		try !nob_cmd_run_sync(cmd) or_fail("./main returned bad status code");

		return 0;
	}

	if (!strcmp(command, "test")) {
		nob_cmd_append(&cmd, "./test");
		try !nob_cmd_run_sync(cmd) or_fail("TEST FAILED");

		return 0;
	}

	return 0;
}
