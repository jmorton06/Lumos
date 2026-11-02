#pragma once
#include "Core/Core.h"
#include "Scene/ISystem.h"
#include <string>
#include <unordered_map>

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
        virtual void UpdateListener(Scene* scene) { };

        void OnDebugDraw() override { };

        bool GetPaused() const { return m_Paused; }
        void SetPaused(bool paused);

        float GetBusVolume(const std::string& bus) const
        {
            auto it = m_BusVolumes.find(bus);
            return it == m_BusVolumes.end() ? 1.0f : it->second;
        }
        void SetBusVolume(const std::string& bus, float volume) { m_BusVolumes[bus] = volume; }
        const std::unordered_map<std::string, float>& GetBusVolumes() const { return m_BusVolumes; }

    protected:
        bool m_Paused;
        std::unordered_map<std::string, float> m_BusVolumes { { "Master", 1.0f } };
    };
}
