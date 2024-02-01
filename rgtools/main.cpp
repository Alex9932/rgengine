#include <stdio.h>
#include <rgtypes.h>
#include <rgstring.h>

#include <chrono>
#include <ctime>

// Tools
#include "rfs.h"

// Implement pack/unpack .rfs archives !!!FIRST!!!

#ifdef RG_PLATFORM_WINDOWS
#define RG_TOOL_VERSION "1.1 (Windows version)"
#else
#define RG_TOOL_VERSION "1.1 (Linux version)"
#endif

#define RG_TOOL_NONE 0x00
#define RG_TOOL_RFS  0x01

#define RG_MODE_NONE   0x00
#define RG_MODE_PACK   0x01
#define RG_MODE_UNPACK 0x02

static char    tool   = RG_TOOL_NONE;
static char    mode   = RG_MODE_NONE;

static String  input  = NULL;
static String  output = NULL;

static Float64 dotime = 0;

static void printHelp() {
	printf("rgtools\n\n");
	printf("Usages:\n");
	printf(" -fs                 -> Work with .rfs archive\n");
	printf(" -mdl                -> Work with models (NOT IMPLEMENTED YET)\n\n");
	printf("Operators:\n");
	printf(" -i  ( --input     ) -> Set input file / dir.\n");
	printf(" -o  ( --output    ) -> Set output file / dir.\n");
	printf(" -p  ( --pack      ) -> Set pack mode.\n");
	printf(" -u  ( --unpack    ) -> Set unpack mode.\n\n");
	printf(" -pm2                -> Converts model to pm2 format (NOT IMPLEMENTED YET)\n");
	printf("Examples:\n");
	printf("rgtools -fs --pack -i ./gamedata -o gamedata.rfs => pack gamedata's content to rfs archive\n");
	printf("rgtools -fs --unpack -i gamedata.rfs -o ./gamedata => unpack content from gamedata.rfs archive\n");
	printf("rgtools -mdl -pm2 -i model.obj -o model.pm2 => convert .obj model to .pm2\n");
}

static void printSum() {
	printf("Done!\n");
	printf("Input: %s\n", input);
	printf("Output: %s\n", output);
	printf("Time: %lfs.\n", dotime);
}

static int parseArgs(int argc, String argv[]) {
	printf("Parsing arguments...\n");

	for (int i = 0; i < argc; i++) {
		String arg = argv[i];

		// Tool
		if (Engine::rg_streql(arg, "-fs")) {
			tool = RG_TOOL_RFS;
		}

		// Mode
		if (Engine::rg_streql(arg, "-p") || Engine::rg_streql(arg, "--pack")) {
			mode = RG_MODE_PACK;
		}

		if (Engine::rg_streql(arg, "-u") || Engine::rg_streql(arg, "--unpack")) {
			mode = RG_MODE_UNPACK;
		}

		// IO
		if (Engine::rg_streql(arg, "-i") || Engine::rg_streql(arg, "--input")) {
			if (i + 1 >= argc) { return -1; }
			input = argv[i + 1];
		}
		if (Engine::rg_streql(arg, "-o") || Engine::rg_streql(arg, "--output")) {
			if (i + 1 >= argc) { return -1; }
			output = argv[i + 1];
		}
	}

	return 0;
}

#undef main
int main(int argc, char** argv) {
	printf("rg_tool v%s\n", RG_TOOL_VERSION);

	if (parseArgs(argc, (String*)argv) != 0 || tool == RG_TOOL_NONE || mode == RG_MODE_NONE || input == NULL || output == NULL) {
		printf("Invalid arguments!\n");
		printHelp();
	} else {

		auto start = std::chrono::system_clock::now();

		switch (tool) {
			case RG_TOOL_RFS: {
				if (mode == RG_MODE_PACK) {
					rfs_pack(input, output);
				} else if (mode == RG_MODE_UNPACK) {
					rfs_unpack(input, output);
				} else {
					printf("Invalid mode!\n");
				}
				break;
			}
			default: {
				printf("Invalid tool!\n");
				break;
			}
		}

		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
		dotime = (double)elapsed_seconds.count();
		printSum();
	}

	return 0;
}