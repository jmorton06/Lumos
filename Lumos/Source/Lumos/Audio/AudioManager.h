#pragma once
#include "Core/Core.h"
#include "Scene/ISystem.h"
#include "Core/DataStructures/Vector.h"

namespace Lumos
{
    class Camera;
    class SoundNode;

    struct Listener
    {
        bool m_Enabled = true;
    };

    class LUMOS_EXPORT AudioManager : public ISystem
    {
    public:
        static AudioManager* Create();

        virtual ~AudioManager()                                          = default;
        virtual bool OnInit() override                                   = 0;
        virtual void OnUpdate(const TimeStep& dt, Scene* scene) override = 0;
        virtual void UpdateListener(Scene* scene) {};

        void AddSoundNode(SoundNode* node)
        {
            m_SoundNodes.EmplaceBack(node);
        }
        void OnDebugDraw() override {};

        void ClearNodes()
        {
            m_SoundNodes.Clear();
        }

        bool GetPaused() const { return m_Paused; }
        void SetPaused(bool paused);

    protected:
        Vector<SoundNode*> m_SoundNodes;
        bool m_Paused;
    };
}
