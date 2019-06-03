#include "LM.h"
#include "Audio/AudioManager.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace Lumos
{
    namespace Audio
    {
        class ALManager : public AudioManager
        {
		public:
			ALManager(int numChannels = 8);
            ~ALManager();

            void OnInit() override;
            void OnUpdate(TimeStep* dt) override;
			void UpdateListener();
			void OnIMGUI() override;

        private:
            ALCcontext* m_Context;
		    ALCdevice* m_Device;

            int m_NumChannels = 0;
		};
    }
}