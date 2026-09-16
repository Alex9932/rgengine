#ifndef _RGBUFFERDRAW_H
#define _RGBUFFERDRAW_H

struct R3D_StaticModel;
union mat4;

namespace Engine {
	namespace Render {

		void InitGBufferDraw();
		void DestroyGBufferDraw();


		void BeginGBufferPass(mat4* proj, mat4* view);
		void EndGBufferPass();

		void DrawGBufferStatic(R3D_StaticModel* mdl, mat4* transform);

	}
}

#endif