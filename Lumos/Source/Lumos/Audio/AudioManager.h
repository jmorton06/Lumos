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

    // Named volume bus. Unregistered names resolve to 1.0 — passing through
    // the per-sound volume unchanged. The "Master" bus exists implicitly.
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

        // Bus volumes are multipliers applied on top of each SoundNode's own
        // volume the next time OpenAL gain is pushed. Defaults to 1.0 for
        // unknown bus names so unmixed code keeps working unchanged.
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
