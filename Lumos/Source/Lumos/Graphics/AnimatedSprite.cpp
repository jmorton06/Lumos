#include "Precompiled.h"
#include "AnimatedSprite.h"

namespace Lumos::Graphics
{
    AnimatedSprite::AnimatedSprite()
    {
        m_CurrentFrame = 0;
    }

    AnimatedSprite::AnimatedSprite(const SharedRef<Texture2D>& texture, const Maths::Vector2& position, const Maths::Vector2& scale, const std::vector<Maths::Vector2>& frames,
        float frameDuration, const std::string& stateName)
    {
        m_Texture = texture;
        m_Position = position;
        m_Scale = scale;

        AnimationState state;
        state.Frames = frames;
        state.FrameDuration = frameDuration;
        state.Mode = PlayMode::Loop;

        m_AnimationStates[stateName] = state;

        m_State = stateName;
        m_CurrentFrame = 0;

        m_UVs = GetAnimatedUVs();
    }

    const std::array<Maths::Vector2, 4>& AnimatedSprite::GetAnimatedUVs()
    {
        LUMOS_PROFILE_FUNCTION();
        auto& currentState = m_AnimationStates[m_State];
        if(m_AnimationStates.find(m_State) == m_AnimationStates.end() || currentState.Frames.empty())
            return GetDefaultUVs();

        auto min = currentState.Frames[m_CurrentFrame];
        auto max = currentState.Frames[m_CurrentFrame] + m_Scale;

        min.x /= m_Texture->GetWidth();
        min.y /= m_Texture->GetHeight();

        max.x /= m_Texture->GetWidth();
        max.y /= m_Texture->GetHeight();

        m_UVs = GetUVs(min, max);

        return m_UVs;
    }

    void AnimatedSprite::OnUpdate(float dt)
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_AnimationStates.find(m_State) == m_AnimationStates.end())
            return;

        m_FrameTimer += dt;

        auto& currentState = m_AnimationStates[m_State];
        if(m_FrameTimer >= currentState.FrameDuration)
        {
            m_FrameTimer = 0.0f;

            if(m_Forward)
            {

                if(m_CurrentFrame == currentState.Frames.size() - 1)
                {
                    if(currentState.Mode == PlayMode::PingPong)
                    {
                        m_CurrentFrame--;
                        m_Forward = false;
                    }
                    else
                    {
                        m_CurrentFrame = 0;
                    }
                }
                else
                    m_CurrentFrame++;
            }
            else
            {
                if(m_CurrentFrame == 0)
                {
                    if(currentState.Mode == PlayMode::PingPong)
                    {
                        m_CurrentFrame = 1;
                        m_Forward = true;
                    }
                    else
                    {
                        m_CurrentFrame = 1;
                        m_Forward = true;
                    }
                }
                else
                    m_CurrentFrame--;
            }

            GetAnimatedUVs();
        }
    }

    void AnimatedSprite::AddState(const std::vector<Maths::Vector2>& frames, float frameDuration, const std::string& stateName)
    {
        AnimationState state;
        state.Frames = frames;
        state.FrameDuration = frameDuration;
        state.Mode = PlayMode::Loop;

        m_AnimationStates[stateName] = state;
    }

    void AnimatedSprite::SetState(const std::string& state)
    {
        m_State = state;
        m_CurrentFrame = 0;
        m_FrameTimer = 0.0f;

        auto found = m_AnimationStates.find(state);

        if(found == m_AnimationStates.end())
        {
            LUMOS_LOG_ERROR("Animated Sprite does not contain state {0}", state);
        }

        m_UVs = GetAnimatedUVs();
    }
}
