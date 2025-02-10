#define NOB_IMPLEMENTATION
#include "nob.h"

#include "src/msk.h"

//#define CC "cc"
//#define CC "clang", "-O3"
#define CC "./tcc", "-L", "../tinycc/", "-I", "../tinycc/include/", "-O3"
#define CFLAGS "-Wall", "-Wextra"

const char* program = "main";
const char* main_c = "src/main.c";

const char*	 paths[] = {"src/msk.h", "src/msk.c"};
const size_t paths_count = sizeof(paths) / sizeof(*paths);

const char*	 test_program = "test";
const char*	 test_paths[] = {"tests/fmt.c"};
const size_t test_paths_count = sizeof(test_paths) / sizeof(*test_paths);

int main(int argc, char** argv) {
	NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "src/msk.h");

	Nob_Cmd cmd = {0};

	if (nob_needs_rebuild1(program, main_c) || nob_needs_rebuild(program, paths, paths_count)) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", program, main_c);
		for (size_t i = 0; i < paths_count; i++) {
			nob_cmd_append(&cmd, paths[i]);
		}
		try !nob_cmd_run_sync_and_reset(&cmd) or_errorf("failed to compile %s", program);
	}

	if (nob_needs_rebuild(test_program, paths, paths_count) || nob_needs_rebuild(test_program, test_paths, test_paths_count) ||
		nob_needs_rebuild1(test_program, "tests/ft_test.h")) {
		nob_cmd_append(&cmd, CC, CFLAGS, "-o", test_program);
		for (size_t i = 0; i < paths_count; i++) {
			nob_cmd_append(&cmd, paths[i]);
		}
		for (size_t i = 0; i < test_paths_count; i++) {
			nob_cmd_append(&cmd, test_paths[i]);
		}
		try !nob_cmd_run_sync_and_reset(&cmd) or_errorf("failed to compile %s", program);
	}

	const char* subcmd = argc > 1 ? argv[1] : "";

	if (!strcmp(subcmd, "run")) {
		nob_cmd_append(&cmd, "./main");
		try !nob_cmd_run_sync(cmd) or_error("./main returned bad status code");
		return 0;
	}

	if (!strcmp(subcmd, "test")) {
		nob_cmd_append(&cmd, "./test");
		try !nob_cmd_run_sync(cmd) or_error("TEST FAILED");
		return 0;
	}

	return 0;
}
