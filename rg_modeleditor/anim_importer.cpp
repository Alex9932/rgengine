#include "anim_importer.h"
#include <allocator.h>
#include <vector>

using namespace Engine;

struct LoadSkeletonState {
	BoneInfo bones[1024];
	const aiNode* nodes[1024]; // nodes by bone id
	aiMatrix4x4 transforms[1024];
	Uint32 bone_counter = 0;
};
#if 0
static void ReadSkeletonHierarchy(LoadSkeletonState* state, const aiNode* node, Sint32 parent) {

	BoneInfo info = {};
	SDL_snprintf(info.name, 32, "%s", node->mName.C_Str());
	info.has_limit = false;
	info.parent = parent;

	aiMatrix4x4 t = node->mTransformation;
	aiVector3D t_scale;
	aiVector3D t_rot;
	aiVector3D t_pos;
	t.Decompose(t_scale, t_rot, t_pos);
	state->transforms[parent + 1] = t; // save current matrix

	mat4 parent_m = MAT4_IDENTITY();
	vec3 pos = *(vec3*)&t_pos[0];
	if (parent != -1) {

		t = state->transforms[parent] * node->mTransformation;
	}
	state->transforms[parent + 1] = t;


	info.offset_pos = *(vec3*)&pos[0];
	//mat4 m = MAT4_IDENTITY();
	//mat4_translate(&m, info.offset_pos);
	//mat4_inverse(&info.offset, m);

	mat4_inverse(&info.offset, *(mat4*)t[0]);
	//info.offset = *(mat4*)t[0];

	state->bones[state->bone_counter] = info;

	state->bone_counter++;
	parent++;

	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		ReadSkeletonHierarchy(state, node->mChildren[i], parent);
	}
}
#endif

static inline void CopyVector(vec3* dst, const aiVector3D& src) {
	dst->x = src.x;
	dst->y = src.y;
	dst->z = src.z;
}

static inline void CopyMatrix(mat4* dst, const aiMatrix4x4& src) {
	dst->m00 = src.a1; dst->m01 = src.a2; dst->m02 = src.a3; dst->m03 = src.a4;
	dst->m10 = src.b1; dst->m11 = src.b2; dst->m12 = src.b3; dst->m13 = src.b4;
	dst->m20 = src.c1; dst->m21 = src.c2; dst->m22 = src.c3; dst->m23 = src.c4;
	dst->m30 = src.d1; dst->m31 = src.d2; dst->m32 = src.d3; dst->m33 = src.d4;
}

static void ReadSkeletonHierarchy(LoadSkeletonState* state, const aiNode* node) {
	state->nodes[state->bone_counter] = node;
	state->bone_counter++;
	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		ReadSkeletonHierarchy(state, node->mChildren[i]);
	}
}

static Sint32 FindBone(const aiNode** nodes, const aiNode* node, Uint32 max_bones) {
	if (!node) { return -1; }
	for (Uint32 i = 0; i < max_bones; i++) {
		if (nodes[i] == node) { return i; }
	}
	return -1;
}

KinematicsModel* LoadSkeleton(LoadSkeletonInfo* info) {
	LoadSkeletonState state = {};

	ReadSkeletonHierarchy(&state, info->scene->mRootNode);

	for (Uint32 i = 0; i < state.bone_counter; i++) {
		const aiNode* bone = state.nodes[i];

		aiMatrix4x4 local = bone->mTransformation;
		aiVector3D l_scale;
		aiVector3D l_rot;
		aiVector3D l_pos;
		local.Decompose(l_scale, l_rot, l_pos);

		aiVector3D pos = l_pos;

		aiMatrix4x4 parent(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
		if (bone->mParent) {
			const aiNode* pbone = bone->mParent;
			Sint32 pidx = FindBone(state.nodes, pbone, state.bone_counter);
			parent = state.transforms[pidx];

			aiVector3D p_scale;
			aiVector3D p_rot;
			aiVector3D p_pos;
			parent.Decompose(p_scale, p_rot, p_pos);

			pos = l_pos - p_pos;

		}

		state.transforms[i] = parent * local;

		mat4 offset;
		//mat4 offset = *(mat4*)state.transforms[i][0];
		CopyMatrix(&offset, state.transforms[i]);


		BoneInfo info = {};
		rgLogInfo(RG_LOG_SYSTEM, "[%d] Bone: %s", i, info.name);

		SDL_snprintf(info.name, 32, "%s", bone->mName.C_Str());
		info.has_limit = false;
		info.parent = FindBone(state.nodes, bone->mParent, state.bone_counter);

		//info.offset_pos = *(vec3*)&pos[0];
		CopyVector(&info.offset_pos, pos);
		mat4_inverse(&info.offset, offset);

		state.bones[i] = info;
	}


	KinematicsModelCreateInfo mk_info = {};

	mk_info.bone_count = state.bone_counter;
	mk_info.bones_info = state.bones;

	mk_info.ik_count = 0;
	mk_info.ik_info  = NULL;
	mk_info.buffer_handle = NULL;

	return RG_NEW(KinematicsModel)(&mk_info);
}

Animation* LoadAnimation(LoadAnimationInfo* info) {
	if (info->anim_idx >= info->scene->mNumAnimations) {
		rgLogError(RG_LOG_SYSTEM, "Animation index out of range!");
		return NULL;
	}

	aiAnimation* animation = info->scene->mAnimations[info->anim_idx];
	//Float64 duration = animation->mDuration;

	Animation* anim = RG_NEW(Animation)(animation->mName.C_Str());
	anim->SetFramerate(animation->mTicksPerSecond);

	Uint32 lastframe = 0;

	for (Uint32 i = 0; i < animation->mNumChannels; i++) {
		aiNodeAnim* node = animation->mChannels[i];
		String name = node->mNodeName.C_Str();
		Uint32 hash = rgCRC32(name, (Uint32)SDL_strlen(name));

		AnimationTrack* track = RG_NEW(AnimationTrack)(name);
		anim->AddBoneAnimationTrack(track);

		// Add interpolation between different keyframes types

		
		for (Uint32 j = 0; j < node->mNumRotationKeys; j++) {
			aiQuatKey* qframe = &node->mRotationKeys[j];
			aiVectorKey* vframe = &node->mPositionKeys[j];
			
			BoneKeyFrame keyframe = {};

			keyframe.scale.x = 1;
			keyframe.scale.y = 1;
			keyframe.scale.z = 1;
			//keyframe.translation = {0, 0, 0};
			//keyframe.translation.x = vframe->mValue.x;
			//keyframe.translation.y = vframe->mValue.y;
			//keyframe.translation.z = vframe->mValue.z;
			keyframe.rotation.x  = qframe->mValue.x;
			keyframe.rotation.y  = qframe->mValue.y;
			keyframe.rotation.z  = qframe->mValue.z;
			keyframe.rotation.w  = qframe->mValue.w;
			keyframe.timestamp   = qframe->mTime * animation->mTicksPerSecond;


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
	
	//anim->Finish(lastframe);

	return anim;
}
