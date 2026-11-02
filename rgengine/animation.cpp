#define DLL_EXPORT
#include "animation.h"
#include "rgstring.h"
#include "allocator.h"

#include <algorithm>

namespace Engine {

    static bool KF_COMPARATOR(BoneKeyFrame f1, BoneKeyFrame f2) {
        return (f1.timestamp < f2.timestamp);
    }

    void AnimationTrack::SortByTimestamp() {
        std::sort(this->keyframes.begin(), this->keyframes.end(), KF_COMPARATOR);
    }


    Animation::Animation(String name, AnimationTrack* tracksBasePointer) {
        SDL_memset(m_name, 0, 64);
        SDL_strlcpy(m_name, name, 64);
		this->base_pointer = tracksBasePointer;
    }

    Animation::~Animation() {
        if(this->base_pointer) {
            for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
                this->bone_tracks[i]->~AnimationTrack();
            }
			GetDefaultAllocator()->Deallocate(this->base_pointer);
        } else {
		    for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
			    RG_DELETE(AnimationTrack, this->bone_tracks[i]);
		    }
        }
		this->bone_tracks.clear();
        this->track_map.clear();
    }

    AnimationTrack* Animation::GetBoneAnimationTrack(Uint32 crc_hash) {
#if 1
        auto it = this->track_map.find(crc_hash);
        if (it == this->track_map.end()) { return NULL; }
        return it->second;
#else
        for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
            if (this->bone_tracks[i]->GetNameHash() == crc_hash) {
                return this->bone_tracks[i];
            }
        }
#endif
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
        this->track_map.emplace(track->GetNameHash(), track); // keeps the first track for a hash, like the old linear search
    }

    void Animation::Update(double dt) {
        this->anim_time += dt * anim_fps * anim_speed;
        if (this->repeat && this->anim_time > this->lastframe) {
            this->anim_time = SDL_fmod(this->anim_time, (double)this->lastframe);
        }
    }

    void Animation::Finish(Float32 frame) {
        this->lastframe = frame;
        // Animator binary-searches the keyframes, so they MUST be sorted
        for (Uint32 i = 0; i < this->bone_tracks.size(); i++) {
            this->bone_tracks[i]->SortByTimestamp();
        }
    }

    void Animation::Reset() {
        this->anim_time = 0;
    }

}