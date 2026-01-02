#ifndef _RGBUFFER_H
#define _RGBUFFER_H

#include "rgvector.h"
#include "rendertypes.h"

namespace Engine {
	namespace Render {

		void InitGBuffer(ivec2* wndsize);
		void DestroyGBuffer();
		void ResizeGBuffer(ivec2* wndsize);

		RDescriptorSet* GetGBufferOutputSet();
		RFramebuffer* GetGBufferFramebuffer();
		RRenderpass* GetGBufferRenderpass();
		RPipeline* GetGBufferPipeline();

		RImage* GetGBufferImage(Uint32 i);
		RImage* GetGBufferDepth();

	}
}

#endif
