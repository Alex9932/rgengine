#ifndef _ANIMATION_H
#define _ANIMATION_H

#include "rgtypes.h"
#include "rgmath.h"
#include <vector>

typedef struct BoneKeyFrame {
    Uint32 timestamp;
    vec3   scale;
    quat   rotation;
    vec3   translation;
    vec4   interp_x;
    vec4   interp_y;
    vec4   interp_z;
    vec4   interp_r;
} BoneKeyFrame;

namespace Engine {

    class AnimationTrack {
        private:
            std::vector<BoneKeyFrame> keyframes;
            Uint32 frames      = 0;
            Uint32 nameCRCHash = 0;
            char   name[32];
            Uint32 framecount  = 0;

        public:
            RG_INLINE AnimationTrack(String name) {
                SDL_strlcpy(this->name, name, 32);
                this->nameCRCHash = rgCRC32(name, (Uint32)SDL_strlen(name));
            }
            RG_INLINE ~AnimationTrack() {}

            RG_INLINE Uint32 GetNameHash() { return this->nameCRCHash; }
            RG_INLINE String GetName() { return this->name; }
            RG_INLINE BoneKeyFrame* GetKeyFrame(Uint32 i) { return &this->keyframes[i]; }
            RG_INLINE Uint32 GetKeyframeCount() { return framecount; }
            RG_INLINE void AddKeyFrame(BoneKeyFrame* keyframe) { this->keyframes.push_back(*keyframe); framecount++; }

            RG_DECLSPEC void SortByTimestamp();
    };

    class Animation {
        private:
            std::vector<AnimationTrack*> bone_tracks;
            double anim_time  = 0;
            double anim_fps   = 30; // VMD animations in 30 fps
            double anim_speed = 1;
            Uint32 lastframe  = 0;
            Bool   repeat     = false;

        public:
            RG_DECLSPEC Animation();
            RG_DECLSPEC ~Animation();

            RG_DECLSPEC void Update(double dt);
            RG_DECLSPEC void Reset();

            RG_INLINE AnimationTrack** GetBoneAnimationTracks() { return &this->bone_tracks[0]; }
            RG_DECLSPEC AnimationTrack* GetBoneAnimationTrack(String name);
            RG_DECLSPEC AnimationTrack* GetBoneAnimationTrack(Uint32 crc_hash);
            RG_DECLSPEC void AddBoneAnimationTrack(AnimationTrack* track);

            RG_INLINE Uint32 GetAnimationTrackCount() { return (Uint32)this->bone_tracks.size(); }

            RG_DECLSPEC void Finish(Uint32 frame);

            RG_INLINE void SetTime(double time) { this->anim_time = time; }
            RG_INLINE void SetFramerate(double fps) { this->anim_fps = fps; }
            RG_INLINE void SetSpeed(double speed) { this->anim_speed = speed; }
            RG_INLINE Uint32 GetLastFrame() { return this->lastframe; }
            RG_INLINE double GetTime() { return this->anim_time; }
            RG_INLINE double GetFramerate() { return this->anim_fps; }
            RG_INLINE double GetSpeed() { return this->anim_speed; }
            RG_INLINE void SetRepeat(Bool r) { this->repeat = r; }
    };

}

#endif