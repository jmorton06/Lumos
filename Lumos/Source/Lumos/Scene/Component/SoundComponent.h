#pragma once

#include "Audio/SoundNode.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    class LUMOS_EXPORT SoundComponent
    {
    public:
        SoundComponent();
        explicit SoundComponent(SharedPtr<SoundNode>& sound);

        void Init();

        void OnImGui();

        SoundNode* GetSoundNode() const { return m_SoundNode.get(); }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(*m_SoundNode.get());
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            auto* node = SoundNode::Create();
            archive(*node);
            m_SoundNode = SharedPtr<SoundNode>(node);
        }

    private:
        SharedPtr<SoundNode> m_SoundNode;
    };
}
