/*
 * rgEngine/leveleditor docker.cpp
 *
 *  Created on: Oct 29, 2024
 *      Author: alex9932
 */

#include "docker.h"
#include "dockerglobal.h"

#include <imgui/imgui.h>

Bool docker_opt_fullscreen = true;
Bool docker_opt_padding    = false;
Bool docker_isStats        = false;

ImGuiDockNodeFlags docker_dockspace_flags = ImGuiDockNodeFlags_None;


void DockerBegin() {
	////////////////////////////
	// Docker                 //
	////////////////////////////

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	if (docker_opt_fullscreen) {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoNavFocus;
	} else {
		docker_dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	if (docker_dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	if (!docker_opt_padding) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	}

	ImGui::Begin("DockSpace Demo", NULL, window_flags);

	if (!docker_opt_padding)   { ImGui::PopStyleVar(); }
	if (docker_opt_fullscreen) { ImGui::PopStyleVar(2); }

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), docker_dockspace_flags);
	}
}

void DockerEnd() {
	////////////////////////////
	// Docker end             //
	////////////////////////////
	ImGui::End();
}