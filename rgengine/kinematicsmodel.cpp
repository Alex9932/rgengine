#define DLL_EXPORT
#include "kinematicsmodel.h"
#include "allocator.h"

namespace Engine {

    KinematicsModel::KinematicsModel(KinematicsModelCreateInfo* info) {
        this->bone_count = info->bone_count;
        this->iklist_count = info->ik_count;
        for (Uint32 i = 0; i < this->bone_count; i++) {
            Bone* bone = &this->bones[i];
            BoneInfo* binfo = &info->bones_info[i];
            SDL_memset(bone->name, 0, 32);
            SDL_strlcpy(bone->name, binfo->name, 32);

            bone->hash       = rgCRC32(bone->name, SDL_strlen(bone->name));
            bone->parent     = binfo->parent;
            bone->id         = i;
            bone->position   = { 0, 0, 0 };
            bone->rotation.x = 0;
            bone->rotation.y = 0;
            bone->rotation.z = 0;
            bone->rotation.w = 1;
            bone->offset     = binfo->offset;
            bone->offset_pos = binfo->offset_pos;
            bone->has_limit  = binfo->has_limit;
            bone->limitation = binfo->limitation;
            bone->transform  = MAT4_IDENTITY();
            //rgLogInfo(RG_LOG_SYSTEM, "Bone: %d -> %d (%d), %s (%d) %d", i, bone->parent, binfo->parent, bone->name, SDL_strlen(bone->name), bone->hash);
        }
        for (Uint32 i = 0; i < this->iklist_count; i++) {
            this->iklist[i] = info->ik_info[i];
        }

        this->handle = info->buffer_handle;
        this->animator = RG_NEW_CLASS(Engine::GetDefaultAllocator(), Animator)(this);
        RebuildSkeleton();
    }

    KinematicsModel::~KinematicsModel() {
        RG_DELETE_CLASS(Engine::GetDefaultAllocator(), Animator, this->animator);
    }

    void KinematicsModel::RebuildSkeleton() {
        for (Uint32 i = 0; i < this->bone_count; i++) {
            Bone* bone = &this->bones[i];
            mat4 parent_transform = MAT4_IDENTITY();
            if (bone->parent != -1) {
                parent_transform = this->bones[bone->parent].transform;
            }

            mat4 translate;
            mat4 rotate = MAT4_IDENTITY();
            mat4_fromquat(&rotate, bone->rotation);
            mat4_translate(&translate, bone->position);

            mat4 local_transform = translate * rotate;
            bone->transform = parent_transform * local_transform;
        }
    }

    void KinematicsModel::RecalculateTransform() {
        //rgLogInfo(RG_LOG_SYSTEM, " ~ ~ ~ ~ ~ ~ ~");
        for (Uint32 i = 0; i < this->bone_count; i++) {
            this->bone_transform[i] = this->bones[i].transform * this->bones[i].offset;
            //rgLogInfo(RG_LOG_SYSTEM, "Bone: %d %s", i, this->bones[i].name);
            //PM(this->bone_transform[i]);
            //PM(this->bones[i].transform);
            //PM(this->bones[i].offset);
        }
    }

    Bone* KinematicsModel::GetBone(Uint32 id) {
        if (id > 1023) { return NULL; }
        return &this->bones[id];
    }

    Bone* KinematicsModel::GetBoneByCRCHash(Uint32 hash) {
        for (Uint32 i = 0; i < this->bone_count; i++) {
            if (this->bones[i].hash == hash) {
                return &this->bones[i];
            }
        }
        return NULL;
    }

    Bone* KinematicsModel::GetBone(String name) {
        Uint32 hash = rgCRC32(name, SDL_strlen(name));
        return GetBoneByCRCHash(hash);
    }

    void KinematicsModel::SolveCCDIK() {
        for (Uint32 i = 0; i < this->iklist_count; ++i) {
            this->SolveCCDIKOne(i);
        }
    }

    void KinematicsModel::RebuildIK(IKList* ik) {
#if 1
        for (Sint32 i = ik->bones - 1; i >= 0; i--) {
            Bone* bone = &this->bones[ik->list[i]];
            mat4 parent_transform = this->bones[bone->parent].transform;
            mat4 translate;
            mat4 rotate = MAT4_IDENTITY();
            mat4_fromquat(&rotate, bone->rotation);
            mat4_translate(&translate, bone->position);
            mat4 local_transform = translate * rotate;
            bone->transform = parent_transform * local_transform;
            //        rgLogInfo(RG_LOG_SYSTEM, "IK Bone: %d -> %d %s", ik->list[i], bone->parent, bone->name);
        }

        Bone* bone = &this->bones[ik->effector];
        mat4 parent_transform = this->bones[bone->parent].transform;
        mat4 translate;
        mat4 rotate = MAT4_IDENTITY();
        mat4_fromquat(&rotate, bone->rotation);
        mat4_translate(&translate, bone->position);
        mat4 local_transform = translate * rotate;
        bone->transform = parent_transform * local_transform;
        //    rgLogInfo(RG_LOG_SYSTEM, "IK Bone: %d -> %d %s", ik->effector, bone->parent, bone->name);

        //    exit(1);
#else
        this->RebuildSkeleton();
#endif
    }

    void KinematicsModel::SolveCCDIKOne(Uint32 id) {
        IKList* ik = &this->iklist[id];
        Uint32 effector = ik->effector;
        Uint32 target = ik->target;

        vec3 effector_pos;
        vec3 target_pos;

        mat4 target_mat = this->bones[target].transform;
        target_pos.x = target_mat.m03;
        target_pos.y = target_mat.m13;
        target_pos.z = target_mat.m23;

        for (Uint32 i = 0; i < ik->iterations; i++) {
            bool rotated = false;

            for (Uint32 i = 0; i < ik->bones; i++) {
                Uint32 link = ik->list[i];

                mat4 m = this->bones[link].transform;
                vec3 p;
                quat q;
                vec3 s;
                mat4_decompose(&p, &q, &s, m);
                quat inv = q.conjugate();

                mat4 eff_mat = this->bones[effector].transform;
                effector_pos.x = eff_mat.m03;
                effector_pos.y = eff_mat.m13;
                effector_pos.z = eff_mat.m23;

                vec3 eff_vec = { effector_pos.x - p.x, effector_pos.y - p.y, effector_pos.z - p.z };
                vec3 eff2_vec = vec3_mulquat(eff_vec, inv);
                eff_vec = eff2_vec.normalize_safe();

                vec3 tar_vec = { target_pos.x - p.x, target_pos.y - p.y, target_pos.z - p.z };
                vec3 tar2_vec = vec3_mulquat(tar_vec, inv);
                tar_vec = tar2_vec.normalize_safe();

                float dot = eff_vec.dot(tar_vec);

                if (dot > 1.0) { dot = 1.0; }
                else if (dot < -1.0) { dot = -1.0; }

                float angle = SDL_acosf(dot);
                if (angle < 1e-5) { continue; }
                if (angle > ik->angle_limit) { angle = ik->angle_limit; }

                vec3 c_vec = eff_vec.cross(tar_vec);
                vec3 cn_vec = c_vec.normalize_safe();
                vec4 axis_angle = { cn_vec.x, cn_vec.y, cn_vec.z, angle };
                quat _q = quat_axisAngle(axis_angle);
                quat _r = this->bones[link].rotation;
                quat rotation = _r * _q;

                Bone* bone = &this->bones[link];
                if (bone->has_limit) {
                    float c = rotation.w;
                    if (c > 1.0) c = 1.0;
                    float c2 = SDL_sqrtf(1 - c * c);
                    rotation.x = bone->limitation.x * c2;
                    rotation.y = bone->limitation.y * c2;
                    rotation.z = bone->limitation.z * c2;
                    rotation.w = c;
                }

                bone->rotation = rotation;
                this->RebuildIK(ik);
                rotated = true;
            }

            if (!rotated) { break; }
        }
    }

}