#pragma once
#include "Sound.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    class LUMOS_EXPORT SoundNode
    {
        template <typename Archive>
        friend void save(Archive& archive, const SoundNode& node);

        template <typename Archive>
        friend void load(Archive& archive, SoundNode& node);

    public:
        static SoundNode* Create();
        SoundNode();
        SoundNode(SharedPtr<Sound> s);
        virtual ~SoundNode();

        void Reset();

        SharedPtr<Sound> GetSound() const { return m_Sound; }

        void SetVelocity(const Vec3& vel) { m_Velocity = vel; }
        Vec3 GetVelocity() const { return m_Velocity; }

        void SetPosition(const Vec3& pos) { m_Position = pos; }
        Vec3 GetPosition() const { return m_Position; }

        void SetVolume(float volume);
        float GetVolume() const { return m_Volume; }

        void SetLooping(bool state) { m_IsLooping = state; }
        bool GetLooping() const { return m_IsLooping; }

        void SetPaused(bool state) { m_Paused = state; }
        bool GetPaused() const { return m_Paused; }

        void SetRadius(float value);
        float GetRadius() const { return m_Radius; }

        float GetPitch() const { return m_Pitch; }
        void SetPitch(float value) { m_Pitch = value; }

        float GetReferenceDistance() const { return m_ReferenceDistance; }
        void SetReferenceDistance(float value) { m_ReferenceDistance = value; }

        float GetRollOffFactor() const { return m_RollOffFactor; }
        void SetRollOffFactor(float value) { m_RollOffFactor = value; }

        bool GetIsGlobal() const { return m_IsGlobal; }
        void SetIsGlobal(bool value) { m_IsGlobal = value; }

        bool GetStationary() const { return m_Stationary; }
        void SetStationary(bool value) { m_Stationary = value; }

        double GetTimeLeft() const { return m_TimeLeft; }

        virtual void OnUpdate(float msec) = 0;
        virtual void Pause()              = 0;
        virtual void Resume()             = 0;
        virtual void Stop()               = 0;
        virtual void SetSound(SharedPtr<Sound> s);

    protected:
        SharedPtr<Sound> m_Sound;
        Vec3 m_Position;
        Vec3 m_Velocity;
        float m_Volume;
        float m_Radius;
        float m_Pitch;
        bool m_IsLooping;
        bool m_IsGlobal;
        double m_TimeLeft;
        bool m_Paused;
        float m_ReferenceDistance;
        float m_RollOffFactor;
        bool m_Stationary;
        double m_StreamPos;
    };

}
