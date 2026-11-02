#define DLL_EXPORT
#include "kinematicsmodel.h"
#include "allocator.h"

namespace Engine {

    static Uint32 HashBoneName(String name) {
        char buf[32];
        SDL_memset(buf, 0, 32);
        SDL_strlcpy(buf, name, 32);
        return rgCRC32(buf, (Uint32)SDL_strlen(buf));
    }

    static mat4 ComposeLocal(Bone* bone) {
        mat4 translate = MAT4_IDENTITY();
        mat4 rotate = MAT4_IDENTITY();
        mat4_fromquat(&rotate, bone->rotation);
        mat4_translate(&translate, bone->position);

        mat4 local_transform = translate * rotate;

        if (bone->scale.x != 1.0f || bone->scale.y != 1.0f || bone->scale.z != 1.0f) {
            mat4 scale = MAT4_IDENTITY();
            scale.m00 = bone->scale.x;
            scale.m11 = bone->scale.y;
            scale.m22 = bone->scale.z;
            local_transform = local_transform * scale;
        }

        return local_transform;
    }

    KinematicsModel::KinematicsModel(KinematicsModelCreateInfo* info) {
        Uint32 count = info->bone_count;
        if (count > RG_MAX_BONE_COUNT) {
            rgLogError(RG_LOG_SYSTEM, "KinematicsModel: too many bones (%u), clamped to %d", count, RG_MAX_BONE_COUNT);
            count = RG_MAX_BONE_COUNT;
        }
        this->bone_count = count;
        this->iklist_count = info->ik_count;
        this->globalInv = info->globalInv;
        for (Uint32 i = 0; i < this->bone_count; i++) {
            Bone* bone = &this->bones[i];
            BoneInfo* binfo = &info->bones_info[i];
            SDL_memset(bone->name, 0, 32);
            SDL_strlcpy(bone->name, binfo->name, 32);

            // Loaders that don't know about rest rotation / scale leave them zeroed
            // TODO: Rewrite loaders
            quat rest_rot   = binfo->offset_rot;
            vec3 rest_scale = binfo->offset_scale;
            if (rest_rot.x == 0 && rest_rot.y == 0 && rest_rot.z == 0 && rest_rot.w == 0) {
                rest_rot.x = 0; rest_rot.y = 0; rest_rot.z = 0; rest_rot.w = 1;
            }
            if (rest_scale.x == 0 && rest_scale.y == 0 && rest_scale.z == 0) {
                rest_scale.x = 1; rest_scale.y = 1; rest_scale.z = 1;
            }

            bone->hash         = rgCRC32(bone->name, SDL_strlen(bone->name));
            bone->parent       = binfo->parent;
            bone->id           = i;
            bone->offset       = binfo->offset;
            bone->offset_pos   = binfo->offset_pos;
            bone->offset_rot   = binfo->offset_rot;
            bone->offset_scale = binfo->offset_scale;

            // Start in the rest pose (was identity -> model collapsed until the first Animator::Update)
            bone->position     = binfo->offset_pos;
            bone->rotation     = rest_rot;
            bone->scale        = rest_scale;
            bone->has_limit    = binfo->has_limit;
            bone->limitation   = binfo->limitation;
            bone->transform    = MAT4_IDENTITY();
            //rgLogInfo(RG_LOG_SYSTEM, "Bone: %d -> %d (%d), %s (%d) %d", i, bone->parent, binfo->parent, bone->name, SDL_strlen(bone->name), bone->hash);
        }
        for (Uint32 i = 0; i < this->iklist_count; i++) {
            this->iklist[i] = info->ik_info[i];
        }

        this->animator = RG_NEW_CLASS(Engine::GetDefaultAllocator(), Animator)(this);
        RebuildSkeleton();
    }

    KinematicsModel::~KinematicsModel() {
        RG_DELETE_CLASS(Engine::GetDefaultAllocator(), Animator, this->animator);
    }

    void KinematicsModel::MakeAnimator() {
		if (!this->animator) {
            this->animator = RG_NEW_CLASS(Engine::GetDefaultAllocator(), Animator)(this);
        }
    }

    // NOTE: requires bones[i].parent < i (parents first).
    void KinematicsModel::RebuildSkeleton() {
        for (Uint32 i = 0; i < this->bone_count; i++) {
            Bone* bone = &this->bones[i];
            mat4 parent_transform = MAT4_IDENTITY();
            if (bone->parent > -1) {
                parent_transform = this->bones[bone->parent].transform;
            }

            mat4 local_transform = ComposeLocal(bone);
            bone->transform = parent_transform * local_transform;
        }
    }

    void KinematicsModel::RecalculateTransform() {
        for (Uint32 i = 0; i < this->bone_count; i++) {
            this->bone_transform[i] = this->globalInv * this->bones[i].transform * this->bones[i].offset;
        }
    }

    Float32 KinematicsModel::ValidateRestPose() {
        mat4 identity = MAT4_IDENTITY();
        Float32 worst = 0.0f;
        Uint32 worst_id = 0;

        for (Uint32 i = 0; i < this->bone_count; i++) {
            mat4 m = this->bones[i].transform * this->bones[i].offset;
            for (Uint32 k = 0; k < 16; k++) {
                Float32 d = SDL_fabsf(m.m[k] - identity.m[k]);
                if (d > worst) { worst = d; worst_id = i; }
            }
        }

        if (this->bone_count > 0) {
            rgLogInfo(RG_LOG_SYSTEM, "KinematicsModel: Rest pose deviation from identity: %f (worst bone %u '%s')",
                worst, worst_id, this->bones[worst_id].name);
        }
        return worst;
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
        return GetBoneByCRCHash(HashBoneName(name));
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
            mat4 parent_transform = MAT4_IDENTITY();
            if (bone->parent > -1) {
                parent_transform = this->bones[bone->parent].transform;
            }
            bone->transform = parent_transform * ComposeLocal(bone);
        }

        Bone* bone = &this->bones[ik->effector];
        mat4 parent_transform = MAT4_IDENTITY();
        if (bone->parent > -1) {
            parent_transform = this->bones[bone->parent].transform;
        }
        bone->transform = parent_transform * ComposeLocal(bone);
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