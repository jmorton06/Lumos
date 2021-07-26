#pragma once

#include "CollisionShape.h"
#include "Hull.h"

namespace Lumos
{
    class LUMOS_EXPORT CuboidCollisionShape : public CollisionShape
    {
    public:
        CuboidCollisionShape();
        explicit CuboidCollisionShape(const Maths::Vector3& halfdims);
        ~CuboidCollisionShape();

        //Collision Shape Functionality
        virtual Maths::Matrix3 BuildInverseInertia(float invMass) const override;

        virtual std::vector<Maths::Vector3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual std::vector<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
            const Maths::Vector3& axis,
            ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        //Set Cuboid Dimensions
        void SetHalfWidth(float half_width)
        {
            m_CuboidHalfDimensions.x = fabs(half_width);
            m_LocalTransform = Maths::Matrix4::Scale(m_CuboidHalfDimensions);
        }
        void SetHalfHeight(float half_height)
        {
            m_CuboidHalfDimensions.y = fabs(half_height);
            m_LocalTransform = Maths::Matrix4::Scale(m_CuboidHalfDimensions);
        }
        void SetHalfDepth(float half_depth)
        {
            m_CuboidHalfDimensions.z = fabs(half_depth);
            m_LocalTransform = Maths::Matrix4::Scale(m_CuboidHalfDimensions);
        }

        //Get Cuboid Dimensions
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

        const Maths::Vector3& GetHalfDimensions() const
        {
            return m_CuboidHalfDimensions;
        }
        void SetHalfDimensions(const Maths::Vector3& dims)
        {
            m_CuboidHalfDimensions = dims;
            m_LocalTransform = Maths::Matrix4::Scale(m_CuboidHalfDimensions);
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

            m_LocalTransform = Maths::Matrix4::Scale(m_CuboidHalfDimensions);
            m_Type = CollisionShapeType::CollisionCuboid;

            if(m_CubeHull->GetNumVertices() == 0)
            {
                ConstructCubeHull();
            }
        }

    protected:
        //Constructs the static cube hull
        static void ConstructCubeHull();

    protected:
        Maths::Vector3 m_CuboidHalfDimensions;

        static SharedRef<Hull> m_CubeHull;
    };

}
