#include "Audio/AudioManager.h"

namespace Lumos
{
    namespace Audio
    {
        class EmptyAudioManager : public AudioManager
        {
        public:
            EmptyAudioManager(int numChannels = 8) { }
            ~EmptyAudioManager() = default;

            bool OnInit() override { return true; };
            void OnUpdate(const TimeStep& dt, Scene* scene) override {};
            void OnImGui() override { }
        };
    }
}