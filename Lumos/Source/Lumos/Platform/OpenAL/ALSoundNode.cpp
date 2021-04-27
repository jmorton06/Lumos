#include "Precompiled.h"
#include "ALSoundNode.h"
#include "ALSound.h"
#include "ALManager.h"

#include "Core/Application.h"

#include "Graphics/Camera/Camera.h"

namespace Lumos
{
    ALSoundNode::ALSoundNode()
    {
        alGenSources(1, &m_Source);
    }

    ALSoundNode::~ALSoundNode()
    {
        alDeleteSources(1, &m_Source);
    }

    void ALSoundNode::OnUpdate(float msec)
    {
        alSourcef(m_Source, AL_GAIN, m_Volume);
        alSourcef(m_Source, AL_PITCH, m_Pitch);
        alSourcef(m_Source, AL_MAX_DISTANCE, m_Radius);
        alSourcef(m_Source, AL_REFERENCE_DISTANCE, m_ReferenceDistance);

        Maths::Vector3 position;
        Maths::Vector3 velocity;

        if(m_IsGlobal)
        {
            //position = Application::Get().GetSystem<AudioManager>()->GetListener()->GetPosition();
        }
        else
        {
            position = GetPosition();
        }

        if(m_Stationary)
        {
            velocity = Maths::Vector3(0.0f);
        }
        else
        {
            velocity = m_Velocity;
        }

        alSourcefv(m_Source, AL_POSITION, reinterpret_cast<float*>(&position));
        alSourcefv(m_Source, AL_VELOCITY, reinterpret_cast<float*>(&velocity));
    }

    void ALSoundNode::Pause()
    {
        alSourcePause(m_Source);
        m_Paused = true;
    }

    void ALSoundNode::Resume()
    {
        alSourcePlay(m_Source);
        m_Paused = false;
    }

    void ALSoundNode::Stop()
    {
        alSourceStop(m_Source);
    }

    void ALSoundNode::SetSound(Sound* s)
    {
        m_Sound = s;
        if(m_Sound)
        {
            m_TimeLeft = m_Sound->GetLength();
            alSourcei(m_Source, AL_BUFFER, static_cast<ALSound*>(m_Sound)->GetBuffer());
            alSourcef(m_Source, AL_MAX_DISTANCE, m_Radius);
            alSourcef(m_Source, AL_ROLLOFF_FACTOR, 1.0f);
            alSourcef(m_Source, AL_REFERENCE_DISTANCE, m_ReferenceDistance);
            alSourcei(m_Source, AL_LOOPING, m_IsLooping ? 1 : 0);
            alSourcef(m_Source, AL_GAIN, m_Volume);
            alSourcef(m_Source, AL_PITCH, m_Pitch);
            //alSourcePlay(m_Source);
        }
    }
}
