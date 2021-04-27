#pragma once

#include "CollisionShape.h"
#include "Hull.h"

namespace Lumos
{

    class LUMOS_EXPORT PyramidCollisionShape : public CollisionShape
    {
    public:
        PyramidCollisionShape();
        PyramidCollisionShape(const Maths::Vector3& halfdims);
        ~PyramidCollisionShape();

        //Collision Shape Functionality
        virtual Maths::Matrix3 BuildInverseInertia(float invMass) const override;

        virtual std::vector<Maths::Vector3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual std::vector<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
            const Maths::Vector3& axis,
            ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        const Maths::Vector3& GetHalfDimensions() const
        {
            return m_PyramidHalfDimensions;
        }
        void SetHalfDimensions(const Maths::Vector3& dims)
        {
            m_PyramidHalfDimensions = dims;

            m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);
            m_Type = CollisionShapeType::CollisionPyramid;

            Maths::Vector3 m_Points[5] = {
                m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, -1.0f),
                m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, 1.0f),
                m_LocalTransform * Maths::Vector3(1.0f, -1.0f, 1.0f),
                m_LocalTransform * Maths::Vector3(1.0f, -1.0f, -1.0f),
                m_LocalTransform * Maths::Vector3(0.0f, 1.0f, 0.0f)
            };

            m_Normals[0] = Maths::Vector3::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalised();
            m_Normals[1] = Maths::Vector3::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalised();
            m_Normals[2] = Maths::Vector3::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalised();
            m_Normals[3] = Maths::Vector3::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalised();
            m_Normals[4] = Maths::Vector3(0.0f, -1.0f, 0.0f);

            if(m_PyramidHull->GetNumVertices() == 0)
            {
                ConstructPyramidHull();
            }
        }

        float GetSize() const override
        {
            return m_PyramidHalfDimensions.x;
        };

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_PyramidHalfDimensions);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_PyramidHalfDimensions);

            m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);
            m_Type = CollisionShapeType::CollisionPyramid;

            Maths::Vector3 m_Points[5] = {
                m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, -1.0f),
                m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, 1.0f),
                m_LocalTransform * Maths::Vector3(1.0f, -1.0f, 1.0f),
                m_LocalTransform * Maths::Vector3(1.0f, -1.0f, -1.0f),
                m_LocalTransform * Maths::Vector3(0.0f, 1.0f, 0.0f)
            };

            m_Normals[0] = Maths::Vector3::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalised();
            m_Normals[1] = Maths::Vector3::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalised();
            m_Normals[2] = Maths::Vector3::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalised();
            m_Normals[3] = Maths::Vector3::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalised();
            m_Normals[4] = Maths::Vector3(0.0f, -1.0f, 0.0f);

            if(m_PyramidHull->GetNumVertices() == 0)
            {
                ConstructPyramidHull();
            }
        }

    protected:
        //Constructs the static cube hull
        static void ConstructPyramidHull();

    protected:
        Maths::Vector3 m_PyramidHalfDimensions;
        Maths::Vector3 m_Normals[5];

        static UniqueRef<Hull> m_PyramidHull;
    };
}
