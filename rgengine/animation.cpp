#define DLL_EXPORT
#include "animation.h"
#include "rgstring.h"

#include <algorithm>

namespace Engine {

    static bool KF_COMPARATOR(BoneKeyFrame f1, BoneKeyFrame f2) {
        return (f1.timestamp < f2.timestamp);
    }

    void AnimationTrack::SortByTimestamp() {
        for (Uint32 i = 0; i < this->keyframes.size(); i++) {
            std::sort(this->keyframes.begin(), this->keyframes.end(), KF_COMPARATOR);
        }
    }


    Animation::Animation() { }
    Animation::~Animation() { }

    AnimationTrack* Animation::GetBoneAnimationTrack(Uint32 crc_hash) {
        for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
            if (this->bone_tracks[i]->GetNameHash() == crc_hash) {
                return this->bone_tracks[i];
            }
        }

        return NULL;
    }

    AnimationTrack* Animation::GetBoneAnimationTrack(String name) {
        AnimationTrack* track = NULL;
        for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
            track = this->bone_tracks[i];
            if (Engine::rg_streql(name, track->GetName())) {
                return track;
            }
        }

        return NULL;
    }

    void Animation::AddBoneAnimationTrack(AnimationTrack* track) {
        this->bone_tracks.push_back(track);
    }

    void Animation::Update(double dt) {
        this->anim_time += dt * anim_fps * anim_speed;
        if (this->repeat && this->anim_time > this->lastframe) {
            this->anim_time = 0;
        }
    }

    void Animation::Finish(Uint32 frame) {
        this->lastframe = frame;
        // Sort keyframes by timestamp
        for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
            this->bone_tracks[i]->SortByTimestamp();
        }
    }

    void Animation::Reset() {
        this->anim_time = 0;
    }

}