#ifndef _RPOSTPROCESS_H
#define _RPOSTPROCESS_H

union ivec2;
struct RDescriptorSet;
struct RImage;

namespace Engine {
	namespace Render {
		void InitPostProcess(ivec2* size);
		void DestroyPostProcess();
		void ResizePostProcess(ivec2* size);
		void ReloadPostProcess(ivec2* size);
		void DoPostProcess();

		void* GetPostProcessOutputImGuiSet();
		RDescriptorSet* GetPostProcessOutputSet();
		RImage* GetPostProcessOutputImage();
	}
}

#endif