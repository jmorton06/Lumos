#include "Precompiled.h"
#include "SoundNode.h"

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALSoundNode.h"
#endif

namespace Lumos
{
    SoundNode* SoundNode::Create()
    {
#ifdef LUMOS_OPENAL
        return new ALSoundNode();
#else
        return nullptr;
#endif
    }

    SoundNode::SoundNode()
    {
        Reset();
    }

    SoundNode::SoundNode(SharedPtr<Sound> s)
    {
        Reset();
        SetSound(s);
    }

    void SoundNode::Reset()
    {
        m_Pitch = 1.0f;
        m_Volume = 1.0f;
        m_Radius = 50.0f;
        m_TimeLeft = 0.0f;
        m_IsLooping = true;
        m_Sound = nullptr;
        m_Paused = false;
        m_StreamPos = 0;
        m_IsGlobal = false;
        m_Stationary = false;
        m_ReferenceDistance = 1.0f;
        m_RollOffFactor = 1.0f;
        m_Velocity = glm::vec3(0.0f);
    }

    SoundNode::~SoundNode()
    {
    }

    void SoundNode::SetSound(SharedPtr<Sound> s)
    {
        m_Sound = s;
        if(m_Sound)
        {
            m_TimeLeft = m_Sound->GetLength();
        }
    }
}
