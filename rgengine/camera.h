#ifndef _CAMERA_H
#define _CAMERA_H

#include "entity.h"
#include "rgvector.h"

#undef near
#undef far

namespace Engine {

	class Camera : public Entity {
		public:
			RG_DECLSPEC Camera(World* w, Float32 near, Float32 far, Float32 fov, Float32 aspect);
			RG_DECLSPEC ~Camera();

			RG_DECLSPEC void Update(double dt);

			RG_DECLSPEC void ReaclculateProjection();
			RG_DECLSPEC void RecalculateView();

			RG_INLINE void RecalculateMatrices() {
				ReaclculateProjection();
				RecalculateView();
			}

			RG_INLINE void SetNearPlane(Float32 n) { this->m_near = n;   }
			RG_INLINE void SetFarPlane(Float32 f)  { this->m_far  = f;   }
			RG_INLINE void SetAspect(Float32 a)    { this->m_aspect = a; }
			RG_INLINE void SetFov(Float32 f)       { this->m_fov = f;    }

			RG_INLINE Float32 GetNearPlane() { return m_near;   }
			RG_INLINE Float32 GetFarPlane()  { return m_far;    }
			RG_INLINE Float32 GetAspect()    { return m_aspect; }
			RG_INLINE Float32 GetFov()       { return m_fov;    }

			RG_INLINE mat4* GetProjection() { return &this->m_proj; }
			RG_INLINE mat4* GetView()       { return &this->m_view; }

		private:
			Float32 m_near;
			Float32 m_far;
			Float32 m_fov;
			Float32 m_aspect;
			mat4    m_proj;
			mat4    m_view;

	};

}

#endif