#ifndef _ANIMATOR_H
#define _ANIMATOR_H

#include "rgtypes.h"
#include "animation.h"

struct Bone;

namespace Engine {

    class KinematicsModel;

    class Animator {
        private:
            KinematicsModel* model;
            Animation*       current_animation;
            Bone*            bones_state;
            Bool             animationChanged;
            Uint8            _offset[3];
            Float32          transition_time;
            Float64          timestamp;

        public:
            RG_DECLSPEC Animator(KinematicsModel* model);
            RG_DECLSPEC virtual ~Animator();

            RG_DECLSPEC void Update(double dt);

            RG_DECLSPEC void PlayAnimation(Animation* animation, Float32 transition);

            RG_INLINE void PlayAnimation(Animation* animation) { PlayAnimation(animation, 0.175); }
            RG_INLINE Animation* GetCurrentAnimation() { return this->current_animation; }
    };

}

#endif