#pragma once
#include "Core/UUID.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/BoundingBox.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"
#include "Core/Function.h"

namespace Lumos
{
    class LumosPhysicsEngine;
    class Manifold;
    class CollisionShape;
    class RigidBody3D;

    enum CollisionShapeType : unsigned int;

    // Callback function called whenever a collision is detected between two objects
    // Params:
    //	RigidBody3D* this_obj			- The current object class that contains the callback
    //	RigidBody3D* colliding_obj	- The object that is colliding with the given object
    // Return:
    //   True	- The physics engine should process the collision as normal
    //	False	- The physics engine should drop the collision pair and not do any further collision resolution/manifold generation
    //			  > This can be useful for AI to see if a player/agent is inside an area/collision volume
    typedef Function<bool(RigidBody3D* this_obj, RigidBody3D* colliding_obj)> PhysicsCollisionCallback;

    struct RigidBody3DProperties
    {
        RigidBody3DProperties();
        ~RigidBody3DProperties();
        Vec3 Position        = Vec3(0.0f);
        Vec3 LinearVelocity  = Vec3(0.0f);
        Vec3 Force           = Vec3(0.0f);
        float Mass           = 1.0f;
        Quat Orientation     = Quat();
        Vec3 AngularVelocity = Vec3(0.0f);
        Vec3 Torque          = Vec3(0.0f);
        bool Static          = false;
        float Elasticity     = 0.5f;
        float Friction       = 0.5f;
        bool AtRest          = false;
        bool isTrigger       = false;
        SharedPtr<CollisionShape> Shape;
    };

    class alignas(16) RigidBody3D
    {
        friend class LumosPhysicsEngine;
        template <typename Archive>
        friend void save(Archive& archive, const RigidBody3D& rigidBody3D);

        template <typename Archive>
        friend void load(Archive& archive, RigidBody3D& rigidBody3D);

    public:
        ~RigidBody3D();

        //<--------- GETTERS ------------->
        const Vec3& GetPosition() const { return m_Position; }
        const Vec3& GetLinearVelocity() const { return m_LinearVelocity; }
        const Vec3& GetForce() const { return m_Force; }
        float GetInverseMass() const { return m_InvMass; }
        const Quat& GetOrientation() const { return m_Orientation; }
        const Vec3& GetAngularVelocity() const { return m_AngularVelocity; }
        const Vec3& GetTorque() const { return m_Torque; }
        const Mat3& GetInverseInertia() const { return m_InvInertia; }
        const Mat4& GetWorldSpaceTransform() const; // Built from scratch or returned from cached value

        const Maths::BoundingBox& GetWorldSpaceAABB();

        void WakeUp();
        void SetIsAtRest(const bool isAtRest);

        Maths::BoundingBox GetLocalBoundingBox() const
        {
            return m_LocalBoundingBox;
        }

        void SetRestVelocityThreshold(float vel)
        {
            if(vel <= 0.0f)
                m_RestVelocityThresholdSquared = -1.0f;
            else
                m_RestVelocityThresholdSquared = vel * vel;
        }

        void SetLocalBoundingBox(const Maths::BoundingBox& bb)
        {
            m_LocalBoundingBox  = bb;
            m_WSAabbInvalidated = true;
        }

        //<--------- SETTERS ------------->

        void SetPosition(const Vec3& v)
        {
            m_Position               = v;
            m_WSTransformInvalidated = true;
            m_WSAabbInvalidated      = true;
            // m_AtRest = false;
        }

        void SetLinearVelocity(const Vec3& v)
        {
            if(m_Static)
                return;
            m_LinearVelocity = v;
            m_AtRest         = false;
        }
        void SetForce(const Vec3& v)
        {
            if(m_Static)
                return;
            m_Force  = v;
            m_AtRest = false;
        }

        void SetOrientation(const Quat& v)
        {
            m_Orientation            = v;
            m_WSTransformInvalidated = true;
            m_AtRest                 = false;
        }

        void SetAngularVelocity(const Vec3& v);
        void SetTorque(const Vec3& v)
        {
            if(m_Static)
                return;
            m_Torque = v;
            m_AtRest = false;
        }
        void SetInverseInertia(const Mat3& v) { m_InvInertia = v; }

        //<---------- CALLBACKS ------------>
        void SetOnCollisionCallback(PhysicsCollisionCallback& callback) { m_OnCollisionCallback = callback; }
        bool FireOnCollisionEvent(RigidBody3D* obj_a, RigidBody3D* obj_b)
        {
            const bool handleCollision = (m_OnCollisionCallback) ? m_OnCollisionCallback(obj_a, obj_b) : true;

            // Wake up on collision
            if(handleCollision)
                WakeUp();

            return handleCollision;
        }

        void FireOnCollisionManifoldCallback(RigidBody3D* a, RigidBody3D* b, Manifold* manifold)
        {
            for(auto& callback : m_OnCollisionManifoldCallbacks)
                callback(a, b, manifold);
        }

        void AutoResizeBoundingBox();
        void RestTest();

        void DebugDraw(uint64_t flags) const;

        typedef Function<void(RigidBody3D*, RigidBody3D*, Manifold*)> OnCollisionManifoldCallback;

        void AddOnCollisionManifoldCallback(const OnCollisionManifoldCallback callback) { m_OnCollisionManifoldCallbacks.PushBack(callback); }

        void SetCollisionShape(CollisionShapeType type);
        void SetCollisionShape(const SharedPtr<CollisionShape>& shape);
        void CollisionShapeUpdated();
        void SetInverseMass(const float& v);
        void SetMass(const float& v);
        const SharedPtr<CollisionShape>& GetCollisionShape() const;

        bool GetIsTrigger() const { return m_Trigger; }
        void SetIsTrigger(bool trigger) { m_Trigger = trigger; }

        float GetAngularFactor() const { return m_AngularFactor; }
        void SetAngularFactor(float factor) { m_AngularFactor = factor; }

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const { return m_Friction; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetIsStatic(const bool isStatic)
        {
            m_Static = isStatic;
            if(m_Static)
                m_AtRest = true;
        }
        // void SetIsColliding(const bool colliding) { m_IsColliding = colliding; }
        UUID GetUUID() const { return m_UUID; }

        // For iteration
        RigidBody3D* m_Prev = nullptr;
        RigidBody3D* m_Next = nullptr;

        bool Valid() const { return m_UUID != 0; }

        uint16_t GetCollisionLayer() const { return m_CollisionLayer; }
        void SetCollisionLayer(u16 layer) { m_CollisionLayer = layer; }

        RigidBody3DProperties GetProperties();

    protected:
        RigidBody3D(const RigidBody3DProperties& properties = RigidBody3DProperties());

        float m_RestVelocityThresholdSquared;
        float m_AverageSummedVelocity;
        float m_Elasticity;
        float m_Friction;

        mutable Mat4 m_WSTransform;
        Maths::BoundingBox m_LocalBoundingBox; //!< Model orientated bounding box in model space

        mutable Maths::BoundingBox m_WSAabb; //!< Axis aligned bounding box of this object in world space

        mutable bool m_WSTransformInvalidated;
        mutable bool m_WSAabbInvalidated; //!< Flag indicating if the cached world space transoformed AABB is invalid
        bool m_Static;
        bool m_AtRest;
        float m_AngularFactor;

        UUID m_UUID;

        u16 m_CollisionLayer = 0;

        Vec3 m_Position;
        float m_InvMass;
        Vec3 m_LinearVelocity;
        bool m_Trigger = false;
        Vec3 m_Force;

        Quat m_Orientation;
        Vec3 m_AngularVelocity;
        Vec3 m_Torque;
        Mat3 m_InvInertia;

        SharedPtr<CollisionShape> m_CollisionShape;
        PhysicsCollisionCallback m_OnCollisionCallback;
        TDArray<OnCollisionManifoldCallback> m_OnCollisionManifoldCallbacks; //!< Collision callbacks post manifold generation
    };
}
