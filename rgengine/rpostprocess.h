#ifndef _RPOSTPROCESS_H
#define _RPOSTPROCESS_H

union ivec2;

namespace Engine {
	namespace Render {
		void InitPostProcess(ivec2* size);
		void DestroyPostProcess();
		void ResizePostProcess(ivec2* size);
		void DoPostProcess();
		void* GetPostProcessOutputSet();
	}
}

#endif