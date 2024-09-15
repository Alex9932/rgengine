#define DLL_EXPORT
#include "animator.h"
#include "kinematicsmodel.h"
#include "allocator.h"

#include "engine.h"

namespace Engine {

    Animator::Animator(KinematicsModel* model) {
        this->model             = model;
        this->current_animation = NULL;
        this->bones_state       = (Bone*)GetDefaultAllocator()->Allocate(sizeof(Bone) * RG_MAX_BONE_COUNT);
        this->animationChanged  = false;
        this->transition_time   = 0.0f;
        this->timestamp         = 0.0;
        SDL_memset(this->_offset, 0, 3);
    }

    Animator::~Animator() {
        GetDefaultAllocator()->Deallocate(this->bones_state);
    }

    static void FindBoneKeyFrames(Uint32 cur_frame, AnimationTrack* track, Uint32* f1, Uint32* f2) {
        Uint32 first = 0;
        Uint32 second = track->GetKeyframeCount() - 1;
        for (Uint32 i = 0; i < track->GetKeyframeCount(); i++) {
            if (track->GetKeyFrame(i)->timestamp > cur_frame) {
                second = i;
                break;
            }
            first = i;
        }

        *f1 = first;
        *f2 = second;
    }

    static Float64 Bezier(vec4* points, Float64 dt) {
        float t = 0.5f;
        float s = 0.5f;
        for (int i = 0; i < 15; i++) {
            float zero = (3 * s * s * t * points->x) + (3 * s * t * t * points->z) + (t * t * t) - dt;
            if (SDL_fabsf(zero) < 0.00001f) { break; }
            if (zero > 0) { t -= 1 / (4 * SDL_powf(2, i)); }
            else { t += 1 / (4 * SDL_powf(2, i)); }
            s = 1 - t;
        }
        return (3 * s * s * t * points->y) + (3 * s * t * t * points->w) + (t * t * t);
    }

    void Animator::Update(double dt) {
        Uint32 bone_count = this->model->GetBoneCount();

        // No animation
        if (!this->current_animation) {
            // Apply T-pose
            for (Uint32 i = 0; i < bone_count; i++) {
                Bone* bone = this->model->GetBone(i);
                bone->position = bone->offset_pos;
                bone->rotation.x = 0;
                bone->rotation.y = 0;
                bone->rotation.z = 0;
                bone->rotation.w = 1;
            }
            return;
        }

        this->current_animation->Update(dt);
        // Animate

        BoneKeyFrame* frame1 = NULL;
        BoneKeyFrame* frame2 = NULL;
        Uint32 f1_id = 0;
        Uint32 f2_id = 0;

        for (Uint32 i = 0; i < bone_count; i++) {
            Bone* bone = this->model->GetBone(i);
            AnimationTrack* track = this->current_animation->GetBoneAnimationTrack(bone->hash);
            if (track == NULL) { // AnimationTrack is doen't exist for this bone
                bone->position = bone->offset_pos;
                bone->rotation.x = 0;
                bone->rotation.y = 0;
                bone->rotation.z = 0;
                bone->rotation.w = 1;
                continue;
            }

            FindBoneKeyFrames(this->current_animation->GetTime(), track, &f1_id, &f2_id);

            frame1 = track->GetKeyFrame(f1_id);
            frame2 = track->GetKeyFrame(f2_id);

            if (frame1 == frame2) {
                bone->position = bone->offset_pos + frame2->translation;
                bone->rotation = frame2->rotation;
                continue;
            }

            Uint32 delta = frame2->timestamp - frame1->timestamp;
            double Df = this->current_animation->GetTime() - frame1->timestamp;
            double anim_dt = 0;
            if (delta != 0) {
                anim_dt = (Df) / (double)delta;
            }

            Float64 x = Bezier(&frame1->interp_x, anim_dt);
            Float64 y = Bezier(&frame1->interp_y, anim_dt);
            Float64 z = Bezier(&frame1->interp_z, anim_dt);
            Float64 r = Bezier(&frame1->interp_r, anim_dt);

            vec3 anim_pos = bone->offset_pos + frame1->translation.lerp(frame2->translation, (float)anim_dt);
            quat anim_rot = frame1->rotation.slerp(frame2->rotation, (float)r);

            // Reset flag
            if (animationChanged && GetUptime() >= timestamp + transition_time) {
                animationChanged = false;
            }

            // Interpolate with prew state if needed
            if (animationChanged) {
                Float32 mix_dt = 1.0 - (Float32)(GetUptime() - timestamp) / transition_time;
                anim_pos = anim_pos.lerp(this->bones_state[i].position, mix_dt);
                anim_rot = anim_rot.slerp(this->bones_state[i].rotation, mix_dt);
            }

            bone->position = anim_pos;
            bone->rotation = anim_rot;

            //rgLogInfo(RG_LOG_SYSTEM, "Bone: %d %s, DT: %f", i, bone->name, anim_dt);
            //rgLogInfo(RG_LOG_SYSTEM, "1Rotation: %f %f %f %f", frame1->rotation.x, frame1->rotation.y, frame1->rotation.z, frame1->rotation.w);
            //rgLogInfo(RG_LOG_SYSTEM, "1Translation: %f %f %f", frame1->translation.x, frame1->translation.y, frame1->translation.z);
            //rgLogInfo(RG_LOG_SYSTEM, "2Rotation: %f %f %f %f", frame2->rotation.x, frame2->rotation.y, frame2->rotation.z, frame2->rotation.w);
            //rgLogInfo(RG_LOG_SYSTEM, "2Translation: %f %f %f", frame2->translation.x, frame2->translation.y, frame2->translation.z);
            //
            //rgLogInfo(RG_LOG_SYSTEM, "Rotation: %f %f %f %f", bone->rotation.x, bone->rotation.y, bone->rotation.z, bone->rotation.w);
            //rgLogInfo(RG_LOG_SYSTEM, "Translation: %f %f %f", bone->position.x, bone->position.y, bone->position.z);

        }

    }

    void Animator::PlayAnimation(Animation* animation, Float32 transition) {

        // Save current bone state
#if 0
        Uint32 bone_count = this->model->GetBoneCount();
        for (Uint32 i = 0; i < bone_count; i++) {
            this->bones_state[i] = *this->model->GetBone(i);
        }
#else
        // Copy bones memory
        SDL_memcpy(this->bones_state, this->model->GetBones(), sizeof(Bone) * RG_MAX_BONE_COUNT);
#endif

        this->animationChanged = true;
        this->transition_time  = transition;
        this->timestamp        = GetUptime();

        this->current_animation = animation;
        if (this->current_animation) {
            this->current_animation->Reset();
        }
    }

}