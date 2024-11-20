/*
 * rgEngine staticobject.cpp
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#include "staticobject.h"

namespace Engine {

	StaticObject::StaticObject(R3D_StaticModel* handle, mat4* transform, AABB* aabb) {
		m_handle    = handle;
		m_transform = *transform;
		m_aabb      = *aabb;
	}

	StaticObject::~StaticObject() {

	}

}