#include "LM.h"
#include "ALManager.h"
#include "ALSoundNode.h"
#include "Maths/Maths.h"
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
    namespace Audio
    {
		ALManager::ALManager(int numChannels) : m_NumChannels(numChannels)
        {

        }

		ALManager::~ALManager()
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(m_Context);
            alcCloseDevice(m_Device);
        }

        void ALManager::OnInit()
        {
            LUMOS_CORE_INFO("Creating SoundSystem!");
            LUMOS_CORE_INFO("Found the following devices: {0}", alcGetString(nullptr, ALC_DEVICE_SPECIFIER));

            m_Device = alcOpenDevice(nullptr);

            if (!m_Device)
                LUMOS_CORE_INFO("Failed to create SoundSystem! (No valid device!)");

            LUMOS_CORE_INFO("SoundSystem created with device: {0}", alcGetString(m_Device, ALC_DEVICE_SPECIFIER));	//Outputs used OAL device!

            m_Context = alcCreateContext(m_Device, nullptr);

            alcMakeContextCurrent(m_Context);
            alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
        }

        void ALManager::OnUpdate()
        {
			UpdateListener();

			for (auto node : m_SoundNodes)
				node->OnUpdate(0.0f);
        }

		void ALManager::UpdateListener()
		{
			if (m_Listener)
			{
				maths::Vector3 worldPos = m_Listener->GetPosition();
				maths::Vector3 velocity = m_Listener->GetVelocity();

				maths::Vector3 dirup[2];
				dirup[0] = m_Listener->GetForwardDirection();
				dirup[1] = m_Listener->GetUpDirection();

				alListenerfv(AL_POSITION, reinterpret_cast<float*>(&worldPos));
				alListenerfv(AL_VELOCITY, reinterpret_cast<float*>(&velocity));
				alListenerfv(AL_ORIENTATION, reinterpret_cast<float*>(&dirup));
			}
		}
    }
}
