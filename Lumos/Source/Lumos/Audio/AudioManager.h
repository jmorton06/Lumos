#pragma once
#include "Core/Core.h"
#include "Scene/ISystem.h"
#include <cereal/cereal.hpp>
#include <vector>
namespace Lumos
{
    class Camera;
    class SoundNode;

    struct Listener
    {
        bool m_Enabled = true;

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("enabled", m_Enabled));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("enabled", m_Enabled));
        }
    };

    class LUMOS_EXPORT AudioManager : public ISystem
    {
    public:
        static AudioManager* Create();

        virtual ~AudioManager()                                          = default;
        virtual void OnInit() override                                   = 0;
        virtual void OnUpdate(const TimeStep& dt, Scene* scene) override = 0;
        virtual void UpdateListener(Scene* scene) {};

        void AddSoundNode(SoundNode* node)
        {
            m_SoundNodes.emplace_back(node);
        }
        void OnDebugDraw() override {};

        void ClearNodes()
        {
            m_SoundNodes.clear();
        }

        bool GetPaused() const { return m_Paused; }
        void SetPaused(bool paused);

    protected:
        std::vector<SoundNode*> m_SoundNodes;
        bool m_Paused;
    };
}
