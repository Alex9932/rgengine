#include <engine.h>
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

int EntryPoint(int argc, String* argv) {
	Application app;
	Initialize(&app);
	Start();
	return 0;
}

rgmain(EntryPoint)