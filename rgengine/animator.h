#ifndef _ANIMATOR_H
#define _ANIMATOR_H

#include "rgtypes.h"
#include "animation.h"

namespace Engine {

    class KinematicsModel;

    class Animator {
        private:
            KinematicsModel* model;
            Animation* current_animation;

        public:
            RG_DECLSPEC Animator(KinematicsModel* model);
            RG_DECLSPEC virtual ~Animator();

            RG_DECLSPEC void Update(double dt);

            RG_DECLSPEC void PlayAnimation(Animation* animation);
            RG_DECLSPEC void OverrideAnimation(Animation* animation, Uint32 time);

            RG_INLINE Animation* GetCurrentAnimation() { return this->current_animation; }
    };

}

#endif