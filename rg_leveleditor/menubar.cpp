/*
 * rgEngine/leveleditor menubar.cpp
 *
 *  Created on: Oct 29, 2024
 *      Author: alex9932
 */

#include "menubar.h"
#include "dockerglobal.h"

#include <imgui/imgui.h>
#include <engine.h>

void MenubarDraw() {
	////////////////////////////
	// Menubar                //
	////////////////////////////

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load")) { /* Engine::Quit(); */ }
			if (ImGui::MenuItem("Save")) { /* Engine::Quit(); */ }
			if (ImGui::MenuItem("Exit")) { Engine::RequestShutdown(); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Option")) {
			if (ImGui::MenuItem("Split geometry", "", docker_splitgeom)) { docker_splitgeom = !docker_splitgeom; }
			if (ImGui::MenuItem("Lock FPS", "", docker_fps_lock)) { docker_fps_lock = !docker_fps_lock; }
			if (ImGui::MenuItem("Toggle renderer stats", "", docker_isStats)) { docker_isStats = !docker_isStats; }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Docker")) {
#if 0
			// Disabling fullscreen would allow the window to be moved to the front of other windows,
			// which we can't undo at the moment without finer window depth/z control.
			ImGui::MenuItem("Fullscreen", NULL, &docker_opt_fullscreen);
			ImGui::MenuItem("Padding", NULL, &docker_opt_padding);
			ImGui::Separator();
#endif
			if (ImGui::MenuItem("Flag: NoSplit", "", (docker_dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) { docker_dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
			if (ImGui::MenuItem("Flag: NoResize", "", (docker_dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { docker_dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (docker_dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { docker_dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (docker_dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { docker_dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (docker_dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, docker_opt_fullscreen)) { docker_dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}