#ifndef _KINEMATICSMODEL_H
#define _KINEMATICSMODEL_H

#define RG_MAX_BONE_COUNT 1024 // Max bones in skeleton
#define RG_MAX_IK_LINK_LIST 32 // Max bones in IK list
#define RG_MAX_IKLIST_COUNT 32 // Max IK lists in skeleton

#include "rgtypes.h"
#include "rgmath.h"
#include "rendertypes.h"
#include "animator.h"

typedef struct Bone {
    Uint32 hash;
    mat4   transform;
    mat4   offset;
    vec3   offset_pos;
    Sint32 parent;
    vec3   position;
    Sint32 id;
    quat   rotation;
    vec3   limitation;
    bool   has_limit;
    Uint8  _offset[3];
    char   name[32];
} Bone;

typedef struct IKList {
    Uint32  target;
    Uint32  effector;
    Uint32  iterations;
    Float32 angle_limit;
    Uint32  bones;
    Uint32  list[RG_MAX_IK_LINK_LIST];
} IKList;

typedef struct BoneInfo {
    mat4   offset;
    Sint32 parent;
    vec3   offset_pos;
    bool   has_limit;
    vec3   limitation;
    char   name[32];
} BoneInfo;

typedef struct KinematicsModelCreateInfo {
    Uint32          bone_count;
    Uint32          ik_count;
    BoneInfo*       bones_info;
    IKList*         ik_info;
    R3D_BoneBuffer* buffer_handle;
} KinematicsModelCreateInfo;

namespace Engine {

    class KinematicsModel {
        private:
            R3D_BoneBuffer* handle;
            Animator* animator;
            Uint32 bone_count;
            Uint32 iklist_count;
            mat4 bone_transform[RG_MAX_BONE_COUNT];
            Bone bones[RG_MAX_BONE_COUNT];
            IKList iklist[RG_MAX_IKLIST_COUNT];
            Uint32 bone_namehashtable[RG_MAX_BONE_COUNT];

        public:
            RG_DECLSPEC KinematicsModel(KinematicsModelCreateInfo* info);
            RG_DECLSPEC virtual ~KinematicsModel();

            RG_DECLSPEC void RebuildSkeleton();
            RG_DECLSPEC void RecalculateTransform();
            RG_DECLSPEC void RebuildIK(IKList* ik);

            RG_DECLSPEC Bone* GetBoneByCRCHash(Uint32 hash);
            RG_DECLSPEC Bone* GetBone(Uint32 id);
            RG_DECLSPEC Bone* GetBone(String name);

            RG_INLINE Uint32 GetBoneCount() { return this->bone_count; }
            RG_INLINE Bone* GetBones()      { return this->bones; }
            RG_INLINE mat4* GetTransforms() { return this->bone_transform; }

            RG_INLINE R3D_BoneBuffer* GetBufferHandle() { return this->handle; }

            RG_INLINE IKList* GetIKLists()    { return this->iklist; }
            RG_INLINE Uint32 GetIKListCount() { return this->iklist_count; }

            RG_INLINE Animator* GetAnimator() { return this->animator; }

            RG_DECLSPEC void SolveCCDIK();
            RG_DECLSPEC void SolveCCDIKOne(Uint32 id);
    };

}

#endif