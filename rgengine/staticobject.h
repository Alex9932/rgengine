/*
 * rgEngine staticobject.h
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#ifndef _STATICOBJECT_H
#define _STATICOBJECT_H

#include "rgmath.h"
#include "uuid.h"

struct R3D_StaticModel;

namespace Engine {

	class StaticObject {

		public:
			StaticObject(R3D_StaticModel* handle, mat4* transform, AABB* aabb);
			~StaticObject();

			RG_INLINE mat4* GetMatrix() { return &m_transform; }
			RG_INLINE AABB* GetAABB()   { return &m_aabb; }

			RG_INLINE RGUUID GetID()    { return m_id; }

			RG_INLINE R3D_StaticModel* GetModelHandle() { return m_handle; }

		private:
			AABB             m_aabb;
			mat4             m_transform;
			RGUUID           m_id;

			R3D_StaticModel* m_handle;

			// TODO: static physics bodies
	};

}

#endif