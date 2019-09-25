#include "lmpch.h"
#include "ALManager.h"
#include "ALSoundNode.h"
#include "Maths/Maths.h"
#include "Graphics/Camera/Camera.h"
#include "Utilities/TimeStep.h"

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Audio
    {
		ALManager::ALManager(int numChannels) : m_Context(nullptr), m_Device(nullptr), m_NumChannels(numChannels)
		{
			m_Listener = nullptr;
            
            m_DebugName = "OpenAL Audio";
		}

		ALManager::~ALManager()
        {
            alcDestroyContext(m_Context);
            alcCloseDevice(m_Device);
        }

        void ALManager::OnInit()
        {
            LUMOS_LOG_INFO("Creating SoundSystem!");
            LUMOS_LOG_INFO("Found the following devices: {0}", alcGetString(nullptr, ALC_DEVICE_SPECIFIER));

            m_Device = alcOpenDevice(nullptr);

            if (!m_Device)
                LUMOS_LOG_INFO("Failed to create SoundSystem! (No valid device!)");

            LUMOS_LOG_INFO("SoundSystem created with device: {0}", alcGetString(m_Device, ALC_DEVICE_SPECIFIER));	//Outputs used OAL device!

            m_Context = alcCreateContext(m_Device, nullptr);

            alcMakeContextCurrent(m_Context);
            alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
        }

        void ALManager::OnUpdate(TimeStep* dt, Scene* scene)
        {
			UpdateListener();

			for (auto node : m_SoundNodes)
				node->OnUpdate(dt->GetElapsedMillis());
        }

		void ALManager::UpdateListener()
		{
			if (m_Listener)
			{
				Maths::Vector3 worldPos = m_Listener->GetPosition();
				Maths::Vector3 velocity = m_Listener->GetVelocity();
                
                ALfloat direction[6];
                
                auto orientation = m_Listener->GetOrientation();
                
                direction[0] = -2 * (orientation.w * orientation.y +
                                     orientation.x * orientation.z);
                direction[1] = 2 * (orientation.x * orientation.w -
                                    orientation.z * orientation.y);
                direction[2] = 2 * (orientation.x * orientation.x +
                                    orientation.y * orientation.y) - 1;
                direction[3] = 2 * (orientation.x * orientation.y -
                                    orientation.w * orientation.z);
                direction[4] = 1 - 2 * (orientation.x * orientation.x +
                                        orientation.z * orientation.z);
                direction[5] = 2 * (orientation.w * orientation.x +
                                    orientation.y * orientation.z);

				alListenerfv(AL_POSITION, reinterpret_cast<float*>(&worldPos));
				alListenerfv(AL_VELOCITY, reinterpret_cast<float*>(&velocity));
                alListenerfv(AL_ORIENTATION,direction);// reinterpret_cast<float*>(&dirup));
			}
		}

		void ALManager::OnImGui()
		{
			ImGui::Text("OpenAL Audio");

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Number Of Audio Sources");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
            ImGui::Text("%5.2lu", m_SoundNodes.size());
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Number Of Channels");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%5.2i", m_NumChannels);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
		}
    }
}
