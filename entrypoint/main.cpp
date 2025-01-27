#include <rgentrypoint.h>

//#define RG_RELEASE

using namespace Engine;

#ifdef RG_RELEASE

int EntryPoint(int argc, String* argv) {
	Initialize();
	Start();
	return 0;
}
rgmain(EntryPoint)

#else

int main(int argc, char** argv) {
	int _argc = 7;
	String _argv[] = {
		argv[0], // exec path
		//"-debug",
		"-fsjson",
		"fsgame.json",
		"-render",
		"rgrenderdx11.dll",
		"-game",
		//"rg_3da.dll"
		"rg_leveleditor.dll"
	};

	ProcessArguments(_argc, _argv);
	Initialize();
	Start();
	return 0;
}

#endif