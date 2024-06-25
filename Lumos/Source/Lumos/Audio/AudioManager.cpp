//#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
//#endif
#include "AudioManager.h"
#include "Core/Application.h"
#include "Scene/Scene.h"
#include "Scene/Component/SoundComponent.h"
#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALManager.h"
#endif
#include "EmptyAudioManager.h"

#include <entt/entity/registry.hpp>

namespace Lumos
{
    AudioManager* AudioManager::Create()
    {
        AudioManager* AManager;

#ifdef LUMOS_OPENAL
        AManager = new Audio::ALManager();
#else
        AManager = new Audio::EmptyAudioManager();
#endif

        if(!AManager->OnInit())
        {
            // If empty audio manager already then init wouldn't fail
            delete AManager;
            AManager = new Audio::EmptyAudioManager();
        }

        AManager->SetPaused(true);

        return AManager;
    }

    void AudioManager::SetPaused(bool paused)
    {
        m_Paused = paused;
        if(!Application::Get().GetCurrentScene())
            return;
        auto soundsView = Application::Get().GetCurrentScene()->GetRegistry().view<SoundComponent>();

        if(m_Paused)
        {
            for(auto entity : soundsView)
                soundsView.get<SoundComponent>(entity).GetSoundNode()->Stop();
        }
        else
        {
            for(auto entity : soundsView)
            {
                auto soundNode = soundsView.get<SoundComponent>(entity).GetSoundNode();
                if(!soundNode->GetPaused())
                    soundNode->Resume();
            }
        }
    }
}
