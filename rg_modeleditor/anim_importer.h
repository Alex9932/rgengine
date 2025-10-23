#ifndef _ANIM_IMPORTER_H
#define _ANIM_IMPORTER_H

#include <animation.h>
#include <assimp/scene.h>

namespace Engine {
	struct KinematicsModel;
}

typedef struct LoadAnimationInfo {
	const aiScene* scene;
	Uint32 anim_idx;
	Engine::KinematicsModel* km;
} LoadAnimationInfo;

Engine::Animation* LoadAnimation(LoadAnimationInfo* info);

#endif