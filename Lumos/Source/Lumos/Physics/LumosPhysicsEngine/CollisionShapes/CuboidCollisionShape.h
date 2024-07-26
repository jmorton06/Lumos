#pragma once

#include "CollisionShape.h"
#include "Hull.h"

#include "Maths/Vector3.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    class LUMOS_EXPORT CuboidCollisionShape : public CollisionShape
    {
    public:
        CuboidCollisionShape();
        explicit CuboidCollisionShape(const Vec3& halfdims);
        ~CuboidCollisionShape();

        // Collision Shape Functionality
        virtual Mat3 BuildInverseInertia(float invMass) const override;

        virtual TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                 const Vec3& axis,
                                                 ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        // Set Cuboid Dimensions
        void SetHalfWidth(float half_width)
        {
            m_CuboidHalfDimensions.x = fabs(half_width);
            m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
            m_CubeHull->UpdateHull();
            // m_LocalTransform         = Mat4::Scale(m_CuboidHalfDimensions);
        }
        void SetHalfHeight(float half_height)
        {
            m_CuboidHalfDimensions.y = fabs(half_height);
            m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
            m_CubeHull->UpdateHull();
            // m_LocalTransform         = Mat4::Scale(m_CuboidHalfDimensions);
        }
        void SetHalfDepth(float half_depth)
        {
            m_CuboidHalfDimensions.z = fabs(half_depth);
            m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
            m_CubeHull->UpdateHull();
            // m_LocalTransform         = Mat4::Scale(m_CuboidHalfDimensions);
        }

        // Get Cuboid Dimensions
        float GetHalfWidth() const
        {
            return m_CuboidHalfDimensions.x;
        }
        float GetHalfHeight() const
        {
            return m_CuboidHalfDimensions.y;
        }
        float GetHalfDepth() const
        {
            return m_CuboidHalfDimensions.z;
        }

        const Vec3& GetHalfDimensions() const
        {
            return m_CuboidHalfDimensions;
        }
        void SetHalfDimensions(const Vec3& dims)
        {
            m_CuboidHalfDimensions = dims;
            m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
            m_CubeHull->UpdateHull();
            // m_LocalTransform       = Mat4::Scale(m_CuboidHalfDimensions);
        }

        virtual float GetSize() const override
        {
            return m_CuboidHalfDimensions.x;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_CuboidHalfDimensions);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_CuboidHalfDimensions);
            m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
            m_CubeHull->UpdateHull();
            // m_LocalTransform = Mat4::Scale(m_CuboidHalfDimensions);
            m_Type = CollisionShapeType::CollisionCuboid;

            if(m_CubeHull->GetNumVertices() == 0)
            {
                ConstructCubeHull();
            }
        }

    protected:
        // Constructs the static cube hull
        static void ConstructCubeHull();

    protected:
        Vec3 m_CuboidHalfDimensions;

        SharedPtr<BoundingBoxHull> m_CubeHull;
    };

}
