
#include "Audio/AudioManager.h"

#include <AL/al.h>
#include <AL/alc.h>

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

            int m_NumChannels = 0;
        };
    }
}
