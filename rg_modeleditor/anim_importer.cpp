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

template<typename K>
static Float32 FindSegment(const K* keys, Uint32 count, Float64 time, Uint32* i0, Uint32* i1) {
	if (time <= keys[0].mTime) { *i0 = *i1 = 0; return 0.0f; }
	if (time >= keys[count - 1].mTime) { *i0 = *i1 = count - 1; return 0.0f; }

	Uint32 lo = 0;
	Uint32 hi = count - 1;
	while (hi - lo > 1) {
		Uint32 mid = (lo + hi) / 2;
		if (keys[mid].mTime <= time) { lo = mid; }
		else { hi = mid; }
	}

	*i0 = lo;
	*i1 = hi;

	Float64 dt = keys[hi].mTime - keys[lo].mTime;
	if (dt <= 1e-9) { return 0.0f; }
	return (Float32)((time - keys[lo].mTime) / dt);
}


static aiVector3D InterpolatePosition(aiNodeAnim* node, Float64 time, const aiVector3D& rest) {
	if (node->mNumPositionKeys == 0) return rest;
	if (node->mNumPositionKeys == 1) return node->mPositionKeys[0].mValue;

	Uint32 i0, i1;
	Float32 f = FindSegment(node->mPositionKeys, node->mNumPositionKeys, time, &i0, &i1);
	const aiVector3D& a = node->mPositionKeys[i0].mValue;
	const aiVector3D& b = node->mPositionKeys[i1].mValue;
	return a + (b - a) * f;
}

static aiQuaternion InterpolateRotation(aiNodeAnim* node, Float64 time, const aiQuaternion& rest) {
	if (node->mNumRotationKeys == 0) return rest;
	if (node->mNumRotationKeys == 1) return node->mRotationKeys[0].mValue;

	Uint32 i0, i1;
	Float32 f = FindSegment(node->mRotationKeys, node->mNumRotationKeys, time, &i0, &i1);
	aiQuaternion q;
	aiQuaternion::Interpolate(q, node->mRotationKeys[i0].mValue, node->mRotationKeys[i1].mValue, f);
	q.Normalize();
	return q;
}

static aiVector3D InterpolateScaling(aiNodeAnim* node, Float64 time, const aiVector3D& rest) {
	if (node->mNumScalingKeys == 0) return rest;
	if (node->mNumScalingKeys == 1) return node->mScalingKeys[0].mValue;

	Uint32 i0, i1;
	Float32 f = FindSegment(node->mScalingKeys, node->mNumScalingKeys, time, &i0, &i1);
	const aiVector3D& a = node->mScalingKeys[i0].mValue;
	const aiVector3D& b = node->mScalingKeys[i1].mValue;
	return a + (b - a) * f;
}

static Uint32 BoneNameHash(String name) {
	char buf[32];
	SDL_memset(buf, 0, 32);
	SDL_strlcpy(buf, name, 32);
	return rgCRC32(buf, (Uint32)SDL_strlen(buf));
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

		const aiVector3D   rest_pos(bone->offset_pos.x, bone->offset_pos.y, bone->offset_pos.z);
		const aiQuaternion rest_rot(bone->offset_rot.w, bone->offset_rot.x, bone->offset_rot.y, bone->offset_rot.z); // (w, x, y, z)
		const aiVector3D   rest_scl(bone->offset_scale.x, bone->offset_scale.y, bone->offset_scale.z);

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
			CopyVector(&keyframe.translation, InterpolatePosition(node, time, rest_pos));
			CopyVector(&keyframe.rotation, InterpolateRotation(node, time, rest_rot));
			CopyVector(&keyframe.scale, InterpolateScaling(node, time, rest_scl));

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