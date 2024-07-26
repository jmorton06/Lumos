#pragma once
#include "Sprite.h"
#include <unordered_map>

namespace Lumos::Graphics
{
    class AnimatedSprite : public Sprite
    {
        template <typename Archive>
        friend void save(Archive& archive, const AnimatedSprite& sprite);

        template <typename Archive>
        friend void load(Archive& archive, AnimatedSprite& sprite);

    public:
        enum class PlayMode
        {
            Loop = 0,
            PingPong
        };

        struct AnimationState
        {
            PlayMode Mode;
            std::vector<Vec2> Frames;
            float FrameDuration = 1.0f;

            template <typename Archive>
            void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("PlayMode", Mode),
                        cereal::make_nvp("Frames", Frames),
                        cereal::make_nvp("FrameDuration", FrameDuration));
            }
        };

        AnimatedSprite();

        AnimatedSprite(const SharedPtr<Texture2D>& texture, const Vec2& position, const Vec2& scale, const std::vector<Vec2>& frames, float frameDuration, const std::string& stateName);
        virtual ~AnimatedSprite() = default;

        void OnUpdate(float dt);

        const std::array<Vec2, 4>& GetAnimatedUVs();

        void AddState(const std::vector<Vec2>& frames, float frameDuration, const std::string& stateName);
        void SetState(const std::string& state);
        const std::string& GetState() const { return m_State; }
        std::unordered_map<std::string, AnimationState>& GetAnimationStates() { return m_AnimationStates; }

        std::unordered_map<std::string, AnimationState> m_AnimationStates;
        uint32_t m_CurrentFrame = 0;
        float m_FrameTimer      = 0.0f;
        std::string m_State;
        bool m_Forward = true;
    };
}
