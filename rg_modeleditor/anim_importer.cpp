#define NOMINMAX
#include "anim_importer.h"
#include <allocator.h>
#include <vector>
#include <set>
#include <kinematicsmodel.h>

#include "assimputil.h"

using namespace Engine;

static void PrintMatrix(mat4* m) {
	rgLogInfo(RG_LOG_SYSTEM, "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
		m->m00, m->m01, m->m02, m->m03,
		m->m10, m->m11, m->m12, m->m13,
		m->m20, m->m21, m->m22, m->m23,
		m->m30, m->m31, m->m32, m->m33
	);
}
#if 0
Animation* LoadAnimation(LoadAnimationInfo* info) {
	if (info->anim_idx >= info->scene->mNumAnimations) {
		rgLogError(RG_LOG_SYSTEM, "Animation index out of range!");
		return NULL;
	}

	aiAnimation* animation = info->scene->mAnimations[info->anim_idx];

	AnimationTrack* tracks = (AnimationTrack*)GetDefaultAllocator()->Allocate(sizeof(AnimationTrack) * animation->mNumChannels);
	Animation* anim = RG_NEW(Animation)(animation->mName.C_Str(), tracks);
	anim->SetFramerate(animation->mTicksPerSecond);
	

	Uint32 lastframe = 0;

	for (Uint32 i = 0; i < animation->mNumChannels; i++) {
		aiNodeAnim* node = animation->mChannels[i];
		String name = node->mNodeName.C_Str();
		Uint32 hash = rgCRC32(name, (Uint32)SDL_strlen(name));

		//AnimationTrack* track = RG_NEW(AnimationTrack)(name);
		// Do not call allocate procedure just use preallocated array
		AnimationTrack* track = new(&tracks[i]) AnimationTrack(name);
		anim->AddBoneAnimationTrack(track);

		// Add interpolation between different keyframes types

		Bone* bone = info->km->GetBoneByCRCHash(hash);
		if (!bone) { continue; } // Skip animation for this bone
		
		for (Uint32 j = 0; j < node->mNumRotationKeys; j++) {
			aiQuatKey* qframe = &node->mRotationKeys[j];
			//aiVectorKey* vframe = &node->mPositionKeys[j];
			
			BoneKeyFrame keyframe = {};

			keyframe.scale.x = 1;
			keyframe.scale.y = 1;
			keyframe.scale.z = 1;
			CopyVector(&keyframe.rotation, qframe->mValue);
			//CopyVector(&keyframe.translation, vframe->mValue);
			//keyframe.translation = keyframe.translation - bone->offset_pos;

			keyframe.timestamp   = qframe->mTime;


			keyframe.interp_x.x = 0.5f;
			keyframe.interp_x.y = 0.5f;
			keyframe.interp_x.z = 0.5f;
			keyframe.interp_x.w = 0.5f;
			keyframe.interp_y = keyframe.interp_x;
			keyframe.interp_z = keyframe.interp_x;
			keyframe.interp_r = keyframe.interp_x;

			track->AddKeyFrame(&keyframe);
			if (lastframe < qframe->mTime) {
				lastframe = qframe->mTime;
			}
		}
	}
	
	anim->Finish(lastframe);

	return anim;
}

#endif

static aiVector3D InterpolatePosition(aiNodeAnim* node, Float64 time) {
	if (node->mNumPositionKeys == 0) return aiVector3D(0, 0, 0);
	if (node->mNumPositionKeys == 1) return node->mPositionKeys[0].mValue;

	// Find surrounding keys
	for (Uint32 i = 0; i < node->mNumPositionKeys - 1; i++) {
		Float64 t0 = node->mPositionKeys[i].mTime;
		Float64 t1 = node->mPositionKeys[i + 1].mTime;
		if (time < t1) {
			Float64 factor = (time - t0) / (t1 - t0);
			return node->mPositionKeys[i].mValue + (node->mPositionKeys[i + 1].mValue - node->mPositionKeys[i].mValue) * (Float32)factor;
		}
	}

	return node->mPositionKeys[node->mNumPositionKeys - 1].mValue;
}

static aiQuaternion InterpolateRotation(aiNodeAnim* node, Float64 time) {
	if (node->mNumRotationKeys == 0) return aiQuaternion(1, 0, 0, 0);
	if (node->mNumRotationKeys == 1) return node->mRotationKeys[0].mValue;

	// Find surrounding keys
	for (Uint32 i = 0; i < node->mNumRotationKeys - 1; i++) {
		Float64 t0 = node->mRotationKeys[i].mTime;
		Float64 t1 = node->mRotationKeys[i + 1].mTime;
		if (time < t1) {
			Float64 factor = (time - t0) / (t1 - t0);
			aiQuaternion q;
			aiQuaternion::Interpolate(q, node->mRotationKeys[i].mValue, node->mRotationKeys[i + 1].mValue, factor);
			q.Normalize();
			return q;
			//return node->mRotationKeys[i].mValue + (node->mRotationKeys[i + 1].mValue - node->mRotationKeys[i].mValue) * (Float32)factor;
		}
	}

	return node->mRotationKeys[node->mNumRotationKeys - 1].mValue;
}

static aiVector3D InterpolateScaling(aiNodeAnim* node, Float64 time) {
	if (node->mNumScalingKeys == 0) return aiVector3D(0, 0, 0);
	if (node->mNumScalingKeys == 1) return node->mScalingKeys[0].mValue;

	// Find surrounding keys
	for (Uint32 i = 0; i < node->mNumScalingKeys - 1; i++) {
		Float64 t0 = node->mScalingKeys[i].mTime;
		Float64 t1 = node->mScalingKeys[i + 1].mTime;
		if (time < t1) {
			Float64 factor = (time - t0) / (t1 - t0);
			return node->mScalingKeys[i].mValue + (node->mScalingKeys[i + 1].mValue - node->mScalingKeys[i].mValue) * (Float32)factor;
		}
	}

	return node->mScalingKeys[node->mNumScalingKeys - 1].mValue;
}

Animation* LoadAnimation(LoadAnimationInfo* info) {
	if (info->anim_idx >= info->scene->mNumAnimations) {
		rgLogError(RG_LOG_SYSTEM, "Animation index out of range!");
		return NULL;
	}

	aiAnimation* animation = info->scene->mAnimations[info->anim_idx];

	AnimationTrack* tracks = (AnimationTrack*)GetDefaultAllocator()->Allocate(sizeof(AnimationTrack) * animation->mNumChannels);
	Animation* anim = RG_NEW(Animation)(animation->mName.C_Str(), tracks);

	// Use 30 fps by default
	Float64 ticks_per_second = animation->mTicksPerSecond != 0.0 ? animation->mTicksPerSecond : 30.0;
	anim->SetFramerate(ticks_per_second);

	Float32 lastframe = 0;


	for (Uint32 i = 0; i < animation->mNumChannels; i++) {
		aiNodeAnim* node = animation->mChannels[i];
		String name = node->mNodeName.C_Str();
		Uint32 hash = rgCRC32(name, (Uint32)SDL_strlen(name));

		// Do not call allocate procedure just use preallocated array
		AnimationTrack* track = new(&tracks[i]) AnimationTrack(name);
		anim->AddBoneAnimationTrack(track);

		Bone* bone = info->km->GetBoneByCRCHash(hash);
		if (!bone) { continue; } // Skip animation for this bone

		std::set<Float64> keyframe_times;
		for (Uint32 j = 0; j < node->mNumPositionKeys; j++) {
			keyframe_times.insert(node->mPositionKeys[j].mTime);
		}
		for (Uint32 j = 0; j < node->mNumRotationKeys; j++) {
			keyframe_times.insert(node->mRotationKeys[j].mTime);
		}
		for (Uint32 j = 0; j < node->mNumScalingKeys; j++) {
			keyframe_times.insert(node->mScalingKeys[j].mTime);
		}

		for (Float64 time : keyframe_times) {

			BoneKeyFrame keyframe = {};
			CopyVector(&keyframe.translation, InterpolatePosition(node, time));
			CopyVector(&keyframe.rotation, InterpolateRotation(node, time));
			CopyVector(&keyframe.scale, InterpolateScaling(node, time));

			keyframe.translation = keyframe.translation - bone->offset_pos; // if needed

			keyframe.timestamp = (Float32)time;
			keyframe.interp_x  = { 0.5f, 0.5f, 0.5f, 0.5f };
			keyframe.interp_y  = keyframe.interp_x;
			keyframe.interp_z  = keyframe.interp_x;
			keyframe.interp_r  = keyframe.interp_x;

			track->AddKeyFrame(&keyframe);
			if (lastframe < time) {
				lastframe = time;
			}

		}

	}

	anim->Finish(lastframe);

	return anim;
}