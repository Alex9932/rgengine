/*
 * rgEngine popup.cpp
 *
 *  Created on: Sep 20, 2024
 *      Author: alex9932
 */

#include "popup.h"
#include <imgui/imgui.h>

#include <window.h>

static char    popupTitle[128];
static char    popupContent[128];
static Uint32  popupMode       = POPUP_MODE_INFO;
static Uint32  popupButtons    = POPUP_BTN_NOBTN;

static Uint32  popupBtnPressed = POPUP_BTNID_NOBTN;
static Float32 popupProgress   = 0.0f;
static char    popupInputBuffer[256];

static void*   popupUserdata   = NULL;

// 0 - no window
static PopupID popupCurrentIdx = 0;
static PopupID popupNextIdx    = 0;

static PopupID popupLastWnd    = 0;

void PopupShow(PopupID wndidx, String title, String text, Uint32 mode, Uint32 btns, void* data) {
	if (popupCurrentIdx != 0) { return; }
	popupCurrentIdx  = wndidx;
	popupMode        = mode;
	popupButtons     = btns;
	popupUserdata    = data;
	popupBtnPressed  = POPUP_BTNID_NOBTN;
	popupProgress    = 0.0f;
	SDL_memset(popupInputBuffer, 0, 256);
	SDL_snprintf(popupTitle, 128, title);
	SDL_snprintf(popupContent, 128, text);
}

void PopupHide() {
	popupLastWnd = popupCurrentIdx;
	popupCurrentIdx = 0;
}

void PopupDraw() {
	if (popupCurrentIdx == 0) { return; }

	ivec2 popupsize = { 260, 100 };

	ivec2 wndsize = {};
	Engine::GetWindowSize(&wndsize);

	Sint32 xpos = (wndsize.x / 2) - (popupsize.x / 2);
	Sint32 ypos = (wndsize.y / 2) - (popupsize.y / 2);

	ImGui::SetNextWindowPos({ (Float32)xpos,  (Float32)ypos });
	ImGui::SetNextWindowSize({ (Float32)popupsize.x, (Float32)popupsize.y });

	ImGui::Begin(popupTitle);

	// center content
	if (popupMode == POPUP_MODE_INFO) {
		ImGui::Text(popupContent);
	}
	else if (popupMode == POPUP_MODE_PROGRESSBAR) {
		ImGui::Text(popupContent);
		ImGui::ProgressBar(popupProgress);
	}
	else if (popupMode == POPUP_MODE_TEXTINPUT) {
		ImGui::Text(popupContent);
		ImGui::InputText("##txtinput", popupInputBuffer, 256);
	}
	else {
		ImGui::Text("Invalid popup_info");
	}

	// down buttons
	if (popupButtons == POPUP_BTN_OK_CANCEL) {
		if (ImGui::Button("Ok")) {
			popupBtnPressed = POPUP_BTNID_OK;
			popupLastWnd = popupCurrentIdx;
			popupCurrentIdx = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			popupBtnPressed = POPUP_BTNID_CANCEL;
			popupLastWnd = popupCurrentIdx;
			popupCurrentIdx = 0;
		}
	}

	ImGui::End();
}

PopupID PopupShown()                    { return popupCurrentIdx; }
Uint32  PopupGetBtnPressed()            { return popupBtnPressed; }
String  PopupGetInputBuffer()           { return popupInputBuffer; }
Float32 PopupGetProgress()              { return popupProgress; }
void*   PopupGetUserdata()              { return popupUserdata; }
void    PopupSetInputBuffer(String str) { SDL_snprintf(popupInputBuffer, 256, "%s", str); }
void    PopupSetProgress(Float32 p)     { popupProgress = p; }

PopupID PopupNextID() {
	popupNextIdx++;
	return popupNextIdx;
}

PopupID PopupClosed() { return popupLastWnd; }
void PopupFree() { popupLastWnd = 0; }