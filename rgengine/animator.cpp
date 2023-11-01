#define DLL_EXPORT
#include "animator.h"
#include "kinematicsmodel.h"

namespace Engine {

    Animator::Animator(KinematicsModel* model) {
        this->model = model;
        this->current_animation = NULL;
    }

    Animator::~Animator() { }

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
            //AnimationTrack* track = this->current_animation->GetBoneAnimationTrack(bone->name);
            if (track == NULL) { // AnimationTrack is doen't exist for this bone
                bone->position = bone->offset_pos;
                bone->rotation.x = 0;
                bone->rotation.y = 0;
                bone->rotation.z = 0;
                bone->rotation.w = 1;
                continue;
            }

            //    for (Uint32 i = 0; i < this->current_animation->GetAnimationTrackCount(); i++) {
            //        AnimationTrack* track = this->current_animation->GetBoneAnimationTracks()[i];
            //        Bone* bone = this->model->GetBoneByCRCHash(track->GetNameHash());
            //        if(bone == NULL) {
            //            continue;
            //        }

            FindBoneKeyFrames(this->current_animation->GetTime(), track, &f1_id, &f2_id);

            frame1 = track->GetKeyFrame(f1_id);
            frame2 = track->GetKeyFrame(f2_id);

            //if(i == 2) {
            //    rgLogInfo(RG_LOG_SYSTEM, "Animation: [%lf] %d-%d (%d-%d)\nBone: %f %f %f %f  %f %f %f", this->current_animation->GetTime(), frame1->timestamp, frame2->timestamp, f1_id, f2_id,
            //        frame2->rotation.x, frame2->rotation.y, frame2->rotation.z, frame2->rotation.w, frame2->translation.x, frame2->translation.y, frame2->translation.z);
            //}

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

            //if(i == 2) {
            //    rgLogInfo(RG_LOG_SYSTEM, "Animation: [%lf] %d-%d (%d-%d)  d: %d, %lf %lf", this->current_animation->GetTime(), frame1->timestamp, frame2->timestamp, f1_id, f2_id, delta, anim_dt, Df);
            //}

            //if(anim_dt < 0) { anim_dt = 0; }
            //if(anim_dt > 1) { anim_dt = 1; }

            // TODO: Use Bezier curves
            // Temporary used linear interpolation

            bone->position = bone->offset_pos + frame1->translation.lerp(frame2->translation, (float)anim_dt);
            bone->rotation = frame1->rotation.slerp(frame2->rotation, (float)anim_dt);

            bone->position.x = bone->position.x;
            bone->position.y = bone->position.y;
            bone->position.z = bone->position.z;
            bone->rotation.x = bone->rotation.x;
            bone->rotation.y = bone->rotation.y;
            bone->rotation.z = bone->rotation.z;
            bone->rotation.w = bone->rotation.w;


            //            rgLogInfo(RG_LOG_SYSTEM, "Bone: %d %s, DT: %f", i, bone->name, anim_dt);
            //            rgLogInfo(RG_LOG_SYSTEM, "1Rotation: %f %f %f %f", frame1->rotation.x, frame1->rotation.y, frame1->rotation.z, frame1->rotation.w);
            //            rgLogInfo(RG_LOG_SYSTEM, "1Translation: %f %f %f", frame1->translation.x, frame1->translation.y, frame1->translation.z);
            //            rgLogInfo(RG_LOG_SYSTEM, "2Rotation: %f %f %f %f", frame2->rotation.x, frame2->rotation.y, frame2->rotation.z, frame2->rotation.w);
            //            rgLogInfo(RG_LOG_SYSTEM, "2Translation: %f %f %f", frame2->translation.x, frame2->translation.y, frame2->translation.z);
            //
            //            rgLogInfo(RG_LOG_SYSTEM, "Rotation: %f %f %f %f", bone->rotation.x, bone->rotation.y, bone->rotation.z, bone->rotation.w);
            //            rgLogInfo(RG_LOG_SYSTEM, "Translation: %f %f %f", bone->position.x, bone->position.y, bone->position.z);

        }

    }

    void Animator::PlayAnimation(Animation* animation) {
        this->current_animation = animation;
        if (this->current_animation) {
            this->current_animation->Reset();
        }
    }

    void OverrideAnimation(Animation* animation, Uint32 time) {

    }

}