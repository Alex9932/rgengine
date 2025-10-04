#ifndef _ANIM_IMPORTER_H
#define _ANIM_IMPORTER_H

#include <animation.h>
#include <kinematicsmodel.h>
#include <assimp/scene.h>

typedef struct LoadSkeletonInfo {
	const aiScene* scene;
} LoadSkeletonInfo;

typedef struct LoadAnimationInfo {
	const aiScene* scene;
	Uint32 anim_idx;
} LoadAnimationInfo;

Engine::KinematicsModel* LoadSkeleton(LoadSkeletonInfo* info);
Engine::Animation* LoadAnimation(LoadAnimationInfo* info);

#endif