#pragma once

#include "CollisionShape.h"
#include "Hull.h"

#include "Maths/Vector3.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{

    class LUMOS_EXPORT PyramidCollisionShape : public CollisionShape
    {
    public:
        PyramidCollisionShape();
        PyramidCollisionShape(const Vec3& halfdims);
        ~PyramidCollisionShape();

        // Collision Shape Functionality
        virtual Mat3 BuildInverseInertia(float invMass) const override;

        virtual TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                 const Vec3& axis,
                                                 ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        const Vec3& GetHalfDimensions() const
        {
            return m_PyramidHalfDimensions;
        }
        void SetHalfDimensions(const Vec3& dims)
        {
            m_PyramidHalfDimensions = dims;

            m_LocalTransform = Mat4::Scale(m_PyramidHalfDimensions);
            m_Type           = CollisionShapeType::CollisionPyramid;

            Vec3 m_Points[5] = {
                m_LocalTransform * Vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * Vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * Vec4(1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * Vec4(1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * Vec4(0.0f, 1.0f, 0.0f, 1.0f)
            };

            m_Normals[0] = Maths::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalised();
            m_Normals[1] = Maths::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalised();
            m_Normals[2] = Maths::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalised();
            m_Normals[3] = Maths::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalised();
            m_Normals[4] = Vec3(0.0f, -1.0f, 0.0f);

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

            m_LocalTransform = Mat4::Scale(m_PyramidHalfDimensions);
            m_Type           = CollisionShapeType::CollisionPyramid;

            Vec3 m_Points[5] = {
                m_LocalTransform * Vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * Vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * Vec4(1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * Vec4(1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * Vec4(0.0f, 1.0f, 0.0f, 1.0f)
            };

            m_Normals[0] = Maths::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalised();
            m_Normals[1] = Maths::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalised();
            m_Normals[2] = Maths::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalised();
            m_Normals[3] = Maths::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalised();
            m_Normals[4] = Vec3(0.0f, -1.0f, 0.0f);

            if(m_PyramidHull->GetNumVertices() == 0)
            {
                ConstructPyramidHull();
            }
        }

    protected:
        // Constructs the static cube hull
        static void ConstructPyramidHull();

    protected:
        Vec3 m_PyramidHalfDimensions;
        Vec3 m_Normals[5];

        static UniquePtr<Hull> m_PyramidHull;
    };
}
