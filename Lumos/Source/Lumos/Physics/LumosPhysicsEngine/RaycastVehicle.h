#pragma once
#include "Core/DataStructures/TDArray.h"
#include "Maths/Vector3.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
    class RigidBody3D;
    class LumosPhysicsEngine;

    struct VehicleWheelConfig
    {
        Vec3 ConnectionPointLocal = Vec3(0.0f);
        float Radius              = 0.4f;
        float SuspensionRest      = 0.5f;     // spring rest length (mount -> wheel centre)
        float SuspensionStiffness = 30000.0f; // N/m
        float SuspensionDamping   = 4000.0f;  // N per (m/s)
        float MaxTravel           = 0.3f;     // max compression/extension from rest
        float GripLateral         = 2.0f;     // sideways friction coefficient
        float GripForward         = 2.0f;     // braking friction coefficient
        bool Steerable            = false;
        bool Powered              = false;
        bool Brakes               = true;
    };

    // Runtime state, refreshed every fixed step. Read by gameplay/visuals.
    struct VehicleWheelState
    {
        bool Grounded          = false;
        Vec3 ContactPoint      = Vec3(0.0f);
        Vec3 ContactNormal     = Vec3(0.0f, 1.0f, 0.0f);
        float SuspensionLength = 0.0f; // current mount -> wheel-centre distance
        float SteerAngle       = 0.0f; // radians, eased toward input
        float SpinAngle        = 0.0f; // visual roll about the axle
        Vec3 WorldPosition     = Vec3(0.0f);
        Quat WorldRotation     = Quat();
    };

    class RaycastVehicle
    {
    public:
        RaycastVehicle() = default;
        ~RaycastVehicle();

        // Engine-owned; created via LumosPhysicsEngine::CreateVehicle.
        void Init(RigidBody3D* chassis);
        int AddWheel(const VehicleWheelConfig& config); // returns wheel index

        // Inputs are -1..1. throttle<0 reverses; handbrake locks rear + cuts grip.
        void SetInputs(float throttle, float brake, float steer, bool handbrake = false);

        void Update(LumosPhysicsEngine* engine, float dt);

        u32 NumWheels() const { return m_Configs.Size(); }
        bool IsGrounded(u32 i) const { return i < m_States.Size() && m_States[i].Grounded; }
        const Vec3& WheelPosition(u32 i) const { return m_States[i].WorldPosition; }
        const Quat& WheelRotation(u32 i) const { return m_States[i].WorldRotation; }
        float GetForwardSpeed() const; // signed m/s along chassis forward

        RigidBody3D* GetChassis() const { return m_Chassis; }

        // Engine/handling tuning (sensible defaults; settable from Lua).
        void SetEngineForce(float f) { m_EngineForce = f; }
        void SetBrakeForce(float f) { m_BrakeForce = f; }
        void SetMaxSteerAngle(float radians) { m_MaxSteerAngle = radians; }
        void SetSteerSpeed(float s) { m_SteerSpeed = s; }
        void SetAntiRoll(float f) { m_AntiRollGrip = f; }

        // Runtime tuning across all wheels (for in-game settings panels).
        void SetGrip(float lateral, float forward)
        {
            for(u32 i = 0; i < m_Configs.Size(); ++i)
            {
                m_Configs[i].GripLateral = lateral;
                m_Configs[i].GripForward = forward;
            }
        }
        void SetSuspension(float stiffness, float damping)
        {
            for(u32 i = 0; i < m_Configs.Size(); ++i)
            {
                m_Configs[i].SuspensionStiffness = stiffness;
                m_Configs[i].SuspensionDamping   = damping;
            }
        }

    private:
        RigidBody3D* m_Chassis = nullptr;
        TDArray<VehicleWheelConfig> m_Configs;
        TDArray<VehicleWheelState> m_States;

        float m_Throttle    = 0.0f;
        float m_Brake       = 0.0f;
        float m_SteerInput  = 0.0f;
        bool m_Handbrake    = false;

        float m_EngineForce   = 8000.0f;
        float m_BrakeForce    = 12000.0f;
        float m_MaxSteerAngle = 0.5f;
        float m_SteerSpeed    = 4.0f; // rad/s easing toward target steer
        float m_AntiRollGrip  = 0.8f; // raise lateral grip toward COM to resist rollover
    };
}
