#pragma once
#include "CollisionShape.h"
#include "Core/Reference.h"

namespace Lumos
{
    struct CompoundChild
    {
        SharedPtr<CollisionShape> Shape;
        Mat4 LocalTransform = Mat4(1.0f);
    };

    class LUMOS_EXPORT CompoundCollisionShape : public CollisionShape
    {
    public:
        CompoundCollisionShape();
        ~CompoundCollisionShape();

        void AddChild(SharedPtr<CollisionShape> shape, const Mat4& localTransform = Mat4(1.0f));
        void RemoveChild(uint32_t index);
        void ClearChildren();

        uint32_t GetChildCount() const { return (uint32_t)m_Children.Size(); }
        const CompoundChild& GetChild(uint32_t index) const { return m_Children[index]; }

        Mat3 BuildInverseInertia(float invMass) const override;
        float GetSize() const override;
        void DebugDraw(const RigidBody3D* currentObject) const override;

        TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;
        void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        void GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const override;

    protected:
        TDArray<CompoundChild> m_Children;
        mutable TDArray<Vec3> m_CachedAxes;
        mutable TDArray<CollisionEdge> m_CachedEdges;
    };
}
