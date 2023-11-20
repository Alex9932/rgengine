#ifndef _ENTRYPOINT_H
#define _ENTRYPOINT_H

#include "engine.h"

#undef main
#define rgmain(fptr)                               \
int main(int argc, char** argv) {                  \
	Engine::ProcessArguments(argc, (String*)argv); \
	return fptr(argc, (String*)argv);              \
}

#endif