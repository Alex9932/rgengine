#include <stdio.h>

// Implement pack/unpack .rfs archives !!!FIRST!!!

int main(int argc, char** argv) {
	printf("rgtools\n\n");
	printf("Usages:\n");
	printf(" -fs       Work with .rfs archive\n");
	printf(" -mdl      Work with models (NOT IMPLEMENTED YET)\n\n");
	printf("Operators:\n");
	printf(" -pack     Pack resources/level to .rfs archive\n");
	printf(" -unpack   Unck resources/level from .rfs archive\n");
	printf(" -pm2      Converts model to pm2 format (NOT IMPLEMENTED YET)\n");
	printf(" -i <f/d>  Used for input data\n");
	printf(" -o <f/d>  Used for output data\n\n");
	printf("Examples:\n");
	printf("rgtools -fs -pack -i ./gamedata -o gamedata.rfs => pack gamedata's content to rfs archive\n");
	printf("rgtools -fs -unpack -i gamedata.rfs -o ./gamedata => unpack content from gamedata.rfs archive\n");
	printf("rgtools -mdl -pm2 -i model.obj -o model.pm2 => convert .obj model to .pm2\n");
	return 0;
}