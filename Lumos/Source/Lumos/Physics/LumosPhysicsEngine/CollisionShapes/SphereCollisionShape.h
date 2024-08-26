#pragma once

#include "CollisionShape.h"
#include "Maths/Vector3.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"

namespace Lumos
{

    class LUMOS_EXPORT SphereCollisionShape : public CollisionShape
    {
    public:
        SphereCollisionShape();
        explicit SphereCollisionShape(float radius);
        ~SphereCollisionShape();

        // Collision Shape Functionality
        virtual Mat3 BuildInverseInertia(float invMass) const override;

        virtual TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                 const Vec3& axis,
                                                 ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        // Get/Set Sphere Radius
        void SetRadius(float radius)
        {
            m_Radius         = radius;
            m_LocalTransform = Mat4::Scale(Vec3(m_Radius * 2.0f));
        }
        float GetRadius() const
        {
            return m_Radius;
        }

        float GetSize() const override
        {
            return m_Radius;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_Radius);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_Radius);
            m_LocalTransform = Mat4::Scale(Vec3(m_Radius * 2.0f));
            m_Type           = CollisionShapeType::CollisionSphere;
        }

    protected:
        float m_Radius;
    };
}
