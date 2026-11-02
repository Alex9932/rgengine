#define DLL_EXPORT
#include "animator.h"
#include "kinematicsmodel.h"
#include "allocator.h"

#include "engine.h"

#define RG_ANIMATION_CURVE_FROM_NEXT_KEY 0
#define RG_ANIMATION_FORCE_LINEAR_INTERPOLATION 0

namespace Engine {

    Animator::Animator(KinematicsModel* model) {
        this->model             = model;
        this->current_animation = NULL;
        this->bones_state       = (Bone*)GetDefaultAllocator()->Allocate(sizeof(Bone) * RG_MAX_BONE_COUNT);
        SDL_memset(this->bones_state, 0, sizeof(Bone) * RG_MAX_BONE_COUNT);
        this->animationChanged  = false;
        this->transition_time   = 0.0f;
        this->timestamp         = 0.0;
        SDL_memset(this->_offset, 0, 3);
    }

    Animator::~Animator() {
        GetDefaultAllocator()->Deallocate(this->bones_state);
    }

    static void FindBoneKeyFrames(Float32 cur_frame, AnimationTrack* track, Uint32* f1, Uint32* f2) {
#if 1
        Uint32 count = track->GetKeyframeCount();
        Uint32 lo = 0;
        Uint32 hi = count;
        while (lo < hi) {
            Uint32 mid = (lo + hi) / 2;
            if (track->GetKeyFrame(mid)->timestamp > cur_frame) { hi = mid; }
            else { lo = mid + 1; }
        }

        if (lo == 0)          { *f1 = 0;         *f2 = 0; }         // before the first key
        else if (lo == count) { *f1 = count - 1; *f2 = count - 1; } // after the last key
        else                  { *f1 = lo - 1;    *f2 = lo; }
#else
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
#endif
    }

    // TODO: Add LUT
    static Float64 Bezier(vec4* points, Float64 dt) {
        // Control points on the diagonal (x1 == y1, x2 == y2) are exactly linear
        // (this is what the Assimp importer writes) -> skip the solver.
        if (points->x == points->y && points->z == points->w) { return dt; }

        if (dt < 0.0) { dt = 0.0; }
        else if (dt > 1.0) { dt = 1.0; }

        Float64 t = 0.5;
        Float64 s = 0.5;
        Float64 z = 0.0; // zero
        Float64 q = 0.0;
        for (Uint32 i = 0; i < 15; i++) {
            z = (3 * s * s * t * points->x) + (3 * s * t * t * points->z) + (t * t * t) - dt;
            if (SDL_fabs(z) < 0.00001) { break; }
            //q = 1 / (4 * SDL_pow(2, i));
            q = 1.0 / (Float64)(4 << i);
            if (z > 0) { t -= q; }
            else { t += q; }
            s = 1 - t;
        }
        return ((3 * s * s * t * points->y) + (3 * s * t * t * points->w) + (t * t * t));
    }

#if 1

    static quat SlerpShortest(quat a, quat b, Float64 t) {
        Float32 d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        if (d < 0.0f) {
            b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
        }
        return a.slerp(b, t);
    }

    static vec3 KeyScale(Bone* bone, BoneKeyFrame* f) {
        if (f->scale.x == 0.0f && f->scale.y == 0.0f && f->scale.z == 0.0f) {
            return bone->offset_scale;
        }
        return f->scale;
    }

    static void SampleBone(Bone* bone, AnimationTrack* track, Float64 time, vec3* pos, quat* rot, vec3* scl) {
        Uint32 f1_id = 0;
        Uint32 f2_id = 0;
        FindBoneKeyFrames((Float32)time, track, &f1_id, &f2_id);

        BoneKeyFrame* frame1 = track->GetKeyFrame(f1_id);
        BoneKeyFrame* frame2 = track->GetKeyFrame(f2_id);

        if (f1_id == f2_id) {
            *pos = bone->offset_pos + frame2->translation;
            *rot = frame2->rotation;
            *scl = KeyScale(bone, frame2);
            return;
        }

        Float64 delta = frame2->timestamp - frame1->timestamp;
        Float64 Df = time - frame1->timestamp;
        Float64 anim_dt = 0;
        if (delta != 0) {
            anim_dt = Df / delta;
        }

        vec3 s1 = KeyScale(bone, frame1);
        vec3 s2 = KeyScale(bone, frame2);
        *scl = s1.lerp(s2, (Float32)anim_dt);

#if RG_ANIMATION_FORCE_LINEAR_INTERPOLATION
        *pos = bone->offset_pos + frame1->translation.lerp(frame2->translation, (Float32)anim_dt);
        *rot = SlerpShortest(frame1->rotation, frame2->rotation, anim_dt);
#else
#if RG_ANIMATION_CURVE_FROM_NEXT_KEY
        BoneKeyFrame* curve = frame2;
#else
        BoneKeyFrame* curve = frame1;
#endif
        Float64 x = Bezier(&curve->interp_x, anim_dt);
        Float64 y = Bezier(&curve->interp_y, anim_dt);
        Float64 z = Bezier(&curve->interp_z, anim_dt);
        Float64 r = Bezier(&curve->interp_r, anim_dt);
        *pos = bone->offset_pos + frame1->translation.lerp(frame2->translation, { (Float32)x, (Float32)y, (Float32)z });
        *rot = SlerpShortest(frame1->rotation, frame2->rotation, r);
#endif

    }

    void Animator::Update(Float64 dt) {
        Uint32 bone_count = this->model->GetBoneCount();

        // No animation
        if (!this->current_animation) {
            // Apply T-pose
            for (Uint32 i = 0; i < bone_count; i++) {
                Bone* bone = this->model->GetBone(i);
                bone->position = bone->offset_pos;
                bone->rotation = bone->offset_rot;
                bone->scale    = { 1, 1, 1 };// bone->offset_scale;
            }
            return;
        }

        this->current_animation->Update(dt);
        Float64 anim_time = this->current_animation->GetTime();

        // Cross-fade state is evaluated once per update, not per bone
        bool    blending = false;
        Float32 mix_dt = 0.0f;
        if (this->animationChanged) {

        }

        for (Uint32 i = 0; i < bone_count; i++) {
            Bone* bone = this->model->GetBone(i);
            // Default: rest pose
            vec3 anim_pos = bone->offset_pos;
            quat anim_rot = bone->offset_rot;
            vec3 anim_scl = bone->offset_scale;

            AnimationTrack* track = this->current_animation->GetBoneAnimationTrack(bone->hash);
            if (track != NULL && track->GetKeyframeCount() > 0) {
                SampleBone(bone, track, anim_time, &anim_pos, &anim_rot, &anim_scl);
            }

            // Cross-fade applies to EVERY bone (it used to be skipped for bones without a track
            // or sitting on a single key, which made them pop)
            if (blending) {
                anim_pos = anim_pos.lerp(this->bones_state[i].position, mix_dt);
                anim_rot = SlerpShortest(anim_rot, this->bones_state[i].rotation, mix_dt);
                anim_scl = anim_scl.lerp(this->bones_state[i].scale, mix_dt);
            }

            bone->position = anim_pos;
            bone->rotation = anim_rot;
            bone->scale    = { 1, 1, 1 };// anim_scl;
        }
    }
#else
    void Animator::Update(Float64 dt) {
        Uint32 bone_count = this->model->GetBoneCount();

        // No animation
        if (!this->current_animation) {
            // Apply T-pose
            for (Uint32 i = 0; i < bone_count; i++) {
                Bone* bone = this->model->GetBone(i);
                bone->position = bone->offset_pos;
                bone->rotation = bone->offset_rot;
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
                bone->rotation = bone->offset_rot;
                continue;
            }

            FindBoneKeyFrames((Float32)this->current_animation->GetTime(), track, &f1_id, &f2_id);

            frame1 = track->GetKeyFrame(f1_id);
            frame2 = track->GetKeyFrame(f2_id);

            if (frame1 == frame2) {
                bone->position = bone->offset_pos + frame2->translation;
                bone->rotation = frame2->rotation;
                continue;
            }

            Float64 delta = frame2->timestamp - frame1->timestamp;
            Float64 Df = this->current_animation->GetTime() - frame1->timestamp;
            Float64 anim_dt = 0;
            if (delta != 0) {
                anim_dt = Df / delta;
            }

#if RG_ANIMATION_FORCE_LINEAR_INTERPOLATION
            vec3 anim_pos = bone->offset_pos + frame1->translation.lerp(frame2->translation, (float)anim_dt);
            //vec3 anim_pos = frame1->translation.lerp(frame2->translation, (float)anim_dt);
            quat anim_rot = frame1->rotation.slerp(frame2->rotation, anim_dt);
#else
            // TODO: FIX THIS (not smooth animation)
            Float64 x = Bezier(&frame1->interp_x, anim_dt);
            Float64 y = Bezier(&frame1->interp_y, anim_dt);
            Float64 z = Bezier(&frame1->interp_z, anim_dt);
            Float64 r = Bezier(&frame1->interp_r, anim_dt);
            vec3 anim_pos = bone->offset_pos + frame1->translation.lerp(frame2->translation, { (Float32)x, (Float32)y, (Float32)z });
            //vec3 anim_pos = frame1->translation.lerp(frame2->translation, (float)anim_dt);
            quat anim_rot = frame1->rotation.slerp(frame2->rotation, r);
#endif


            // Reset flag
            if (animationChanged && GetUptime() >= timestamp + transition_time) {
                animationChanged = false;
            }

            // Interpolate with prew state if needed
            if (animationChanged) {
                Float32 mix_dt = 1.0f - (Float32)(GetUptime() - timestamp) / transition_time;
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
#endif

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