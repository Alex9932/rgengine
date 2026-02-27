#ifndef _RIMGUI_H
#define _RIMGUI_H

#include "rgtypes.h"

typedef void (*RenderImGuiCallback)();

namespace Engine {
	namespace Render {

		void InitRImGui();
		void DestroyRImGui();
		void ResizeRImGui();
		void UpdateImGui();
		void DrawImGui(Uint32 drawOutput);

		RG_DECLSPEC void RegisterImGuiDrawCallback(RenderImGuiCallback cb);
		RG_DECLSPEC void FreeImGuiDrawCallback(RenderImGuiCallback cb);

		// ImGui window
		RG_DECLSPEC void DrawRendererStats();
		RG_DECLSPEC void DrawProfilerStats();

	}
}

#endif