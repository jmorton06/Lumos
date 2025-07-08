#pragma once
#include "Audio/AudioManager.h"

typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

namespace Lumos
{
    namespace Maths
    {
        class Transform;
    }

    namespace Audio
    {
        class ALManager : public AudioManager
        {
        public:
            ALManager(int numChannels = 8);
            ~ALManager();

            bool OnInit() override;
            void OnUpdate(const TimeStep& dt, Scene* scene) override;
            void UpdateListener(Scene* scene) override;
            void UpdateListener(Maths::Transform& listenerTransform);
            void OnImGui() override;

        private:
            ALCcontext* m_Context;
            ALCdevice* m_Device;

            int m_NumChannels     = 0;
            u32 m_LatestNodeCount = 0;
        };
    }
}
