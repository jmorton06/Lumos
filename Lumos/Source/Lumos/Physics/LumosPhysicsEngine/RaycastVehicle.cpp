#include "Precompiled.h"
#include "RaycastVehicle.h"
#include "RigidBody3D.h"
#include "LumosPhysicsEngine.h"
#include "RaycastResult.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    static constexpr float kHalfPi = 1.57079632679f;

    RaycastVehicle::~RaycastVehicle() = default;

    void RaycastVehicle::Init(RigidBody3D* chassis)
    {
        m_Chassis = chassis;
    }

    int RaycastVehicle::AddWheel(const VehicleWheelConfig& config)
    {
        m_Configs.PushBack(config);
        VehicleWheelState state;
        state.SuspensionLength = config.SuspensionRest;
        m_States.PushBack(state);
        return (int)m_Configs.Size() - 1;
    }

    void RaycastVehicle::SetInputs(float throttle, float brake, float steer, bool handbrake)
    {
        m_Throttle   = Maths::Clamp(throttle, -1.0f, 1.0f);
        m_Brake      = Maths::Clamp(brake, 0.0f, 1.0f);
        m_SteerInput = Maths::Clamp(steer, -1.0f, 1.0f);
        m_Handbrake  = handbrake;
    }

    float RaycastVehicle::GetForwardSpeed() const
    {
        if(!m_Chassis)
            return 0.0f;
        const Vec3 fwd = m_Chassis->GetOrientation() * Vec3(0.0f, 0.0f, -1.0f);
        return Maths::Dot(m_Chassis->GetLinearVelocity(), fwd);
    }

    void RaycastVehicle::Update(LumosPhysicsEngine* engine, float dt)
    {
        if(!m_Chassis || m_Configs.Empty() || dt <= 0.0f)
            return;

        const Quat ori    = m_Chassis->GetOrientation();
        const Vec3 pos     = m_Chassis->GetPosition();
        const Vec3 up      = ori * Vec3(0.0f, 1.0f, 0.0f);
        const Vec3 fwd     = ori * Vec3(0.0f, 0.0f, -1.0f);

        const float invMass = m_Chassis->GetInverseMass();
        const float mass     = invMass > 0.0f ? 1.0f / invMass : 1.0f;
        const float massShare = mass / (float)m_Configs.Size();

        const u32 powered = [&]
        {
            u32 n = 0;
            for(u32 i = 0; i < m_Configs.Size(); ++i)
                if(m_Configs[i].Powered) n++;
            return n > 0 ? n : 1;
        }();

        for(u32 i = 0; i < m_Configs.Size(); ++i)
        {
            const VehicleWheelConfig& cfg = m_Configs[i];
            VehicleWheelState& st         = m_States[i];

            // Ease steer toward target (front wheels only).
            const float targetSteer = cfg.Steerable ? m_SteerInput * m_MaxSteerAngle : 0.0f;
            st.SteerAngle += Maths::Clamp(targetSteer - st.SteerAngle, -m_SteerSpeed * dt, m_SteerSpeed * dt);

            // Suspension ray: from the mount, straight down the chassis up axis.
            const Vec3 mount = pos + ori * cfg.ConnectionPointLocal;
            const float rayLen = cfg.SuspensionRest + cfg.Radius;

            RaycastQuery q(mount, -up, rayLen);
            q.IgnoreBody = m_Chassis;
            RaycastHit hit = engine->Raycast(q);

            if(!hit.Hit())
            {
                st.Grounded        = false;
                st.SuspensionLength = cfg.SuspensionRest;
                st.WorldPosition    = mount - up * cfg.SuspensionRest;
                st.WorldRotation    = ori * Quat::RotationY(st.SteerAngle) * Quat::RotationX(st.SpinAngle) * Quat::RotationZ(kHalfPi);
                continue;
            }

            st.Grounded      = true;
            st.ContactPoint  = hit.Point;
            st.ContactNormal = hit.Normal;

            // Spring length = how far the wheel centre sits below the mount.
            float springLen = hit.Distance - cfg.Radius;
            springLen        = Maths::Clamp(springLen, cfg.SuspensionRest - cfg.MaxTravel, cfg.SuspensionRest + cfg.MaxTravel);
            st.SuspensionLength = springLen;

            const Vec3 normal = hit.Normal;

            // --- Suspension: spring + damper along the contact normal ---
            const float compression = cfg.SuspensionRest - springLen; // +ve = compressed
            const Vec3 contactVel    = m_Chassis->GetPointVelocity(hit.Point);
            const float vn           = Maths::Dot(contactVel, normal);

            float suspForce = cfg.SuspensionStiffness * compression - cfg.SuspensionDamping * vn;
            if(suspForce < 0.0f)
                suspForce = 0.0f; // springs push, never pull

            m_Chassis->ApplyImpulseAtPoint(normal * (suspForce * dt), hit.Point);

            // --- Tyre friction at the contact, in the ground plane ---
            // Wheel forward, steered, projected onto the ground plane.
            Vec3 wheelFwd = (Quat::Rotation(st.SteerAngle, up) * fwd);
            wheelFwd      = wheelFwd - normal * Maths::Dot(wheelFwd, normal);
            if(Maths::Length(wheelFwd) < 1e-4f)
                continue;
            wheelFwd      = wheelFwd.Normalised();
            const Vec3 wheelRight = Maths::Cross(normal, wheelFwd).Normalised();

            // Recompute contact velocity after suspension impulse.
            const Vec3 cv = m_Chassis->GetPointVelocity(hit.Point);

            // Lateral grip: cancel sideways velocity, clamped to the friction circle.
            float lateralGrip = cfg.GripLateral;
            if(m_Handbrake && !cfg.Steerable)
                lateralGrip *= 0.4f; // loosen the rear for handbrake slides

            const float vLat       = Maths::Dot(cv, wheelRight);
            float lateralImpulse   = -vLat * massShare;
            const float maxLateral = lateralGrip * suspForce * dt;
            lateralImpulse         = Maths::Clamp(lateralImpulse, -maxLateral, maxLateral);

            // Apply lateral grip raised toward the COM height instead of at the
            // contact patch — applying it on the ground levers the car over its
            // outside wheels and flips it. (0=contact, 1=COM height, no roll.)
            const float comLift  = Maths::Dot(pos - hit.Point, up) * m_AntiRollGrip;
            const Vec3 gripPoint = hit.Point + up * comLift;
            m_Chassis->ApplyImpulseAtPoint(wheelRight * lateralImpulse, gripPoint);

            // Longitudinal: drive + brake along the wheel forward.
            const float vLong = Maths::Dot(cv, wheelFwd);
            float longImpulse = 0.0f;

            if(cfg.Powered && !m_Handbrake)
                longImpulse += (m_EngineForce * m_Throttle / (float)powered) * dt;

            const bool braking = (m_Brake > 0.0f || (m_Handbrake && !cfg.Steerable)) && cfg.Brakes;
            if(braking)
            {
                const float brakeStrength = m_Handbrake ? 1.0f : m_Brake;
                float brakeImpulse        = -vLong * massShare;
                const float maxBrake      = m_BrakeForce * brakeStrength * dt;
                brakeImpulse              = Maths::Clamp(brakeImpulse, -maxBrake, maxBrake);
                longImpulse += brakeImpulse;
            }

            // Apply drive/brake at the COM-raised point too: at the contact
            // patch the forward thrust (below the COM) pitches the front up
            // ("wheelie" on acceleration) / dives on braking.
            m_Chassis->ApplyImpulseAtPoint(wheelFwd * longImpulse, gripPoint);

            // Visual wheel: spin from rolling speed, placed at the contact.
            st.SpinAngle += (vLong / cfg.Radius) * dt;
            st.WorldPosition = hit.Point + normal * cfg.Radius;
            st.WorldRotation = ori * Quat::RotationY(st.SteerAngle) * Quat::RotationX(st.SpinAngle) * Quat::RotationZ(kHalfPi);
        }
    }
}
