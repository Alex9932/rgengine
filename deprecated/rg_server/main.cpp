#define GAME_DLL
#include <rgentrypoint.h>

using namespace Engine;

class Application : public BaseGame {
	public:

		Application() {
			this->isClient   = false;
			this->isGraphics = false;
		}

		~Application() {
		}

		void MainUpdate() {
		}

		void Initialize() {
		}

		void Quit() {
		}
		
		String GetName() { return "rgserver"; }
};

static Application* app;

void Module_Initialize() {
	app = new Application();
}

void Module_Destroy() {
	delete app;
}

BaseGame* Module_GetApplication() {
	return app;
}