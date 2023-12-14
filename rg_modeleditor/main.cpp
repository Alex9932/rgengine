#include <rgentrypoint.h>
#include <render.h>

#include <imgui/ImGuiFileDialog.h>

using namespace Engine;

class Application : public BaseGame {
	private:

		Bool isModelLoaded = false;
		char currentModel[256];


	public:
		Application() {
			this->isClient = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D | RG_RENDER_FULLSCREEN);
		}

		~Application() {
		}

		void MainUpdate() {
			ImGui::Begin("Model");

			if (isModelLoaded) {
				ImGui::Text("Current model: %s", currentModel);
				// TODO
			}

			if (isModelLoaded) {
				if (ImGui::Button("Save as")) {
					isModelLoaded = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Close")) {
					isModelLoaded = false;
				}
			} else {
				if (ImGui::Button("Load model")) {
					ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".obj,.pm2,.pmd,.pmx", ".");
				}
			}
			ImGui::End();

			if (ImGuiFileDialog::Instance()->Display("Open model")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

					SDL_memcpy(currentModel, filePathName.c_str(), filePathName.length() + 1);
					rgLogInfo(RG_LOG_SYSTEM, "Opened: %s", currentModel);
					isModelLoaded = true;
				}

				ImGuiFileDialog::Instance()->Close();
			}
		}

		void Initialize() {

		}

		void Quit() {

		}
};

int EntryPoint(int argc, String* argv) {

	Application app;
	Initialize(&app);
	Start();

	return 0;

}

rgmain(EntryPoint)