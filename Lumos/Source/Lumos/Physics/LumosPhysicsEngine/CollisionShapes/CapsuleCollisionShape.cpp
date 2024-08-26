#include "Precompiled.h"
#include "CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/Matrix3.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    CapsuleCollisionShape::CapsuleCollisionShape(float radius, float height)
    {
        m_Radius         = radius;
        m_Height         = height;
        m_LocalTransform = Mat4(1.0); // Mat4::Scale(Vec3(m_Radius));
        m_Type           = CollisionShapeType::CollisionCapsule;
    }

    CapsuleCollisionShape::~CapsuleCollisionShape()
    {
    }

    Mat3 CapsuleCollisionShape::BuildInverseInertia(float invMass) const
    {
        Vec3 halfExtents(m_Radius, m_Radius, m_Radius);
        halfExtents.x += m_Height * 0.5f;

        float lx               = 2.0f * (halfExtents.x);
        float ly               = 2.0f * (halfExtents.y);
        float lz               = 2.0f * (halfExtents.z);
        const float x2         = lx * lx;
        const float y2         = ly * ly;
        const float z2         = lz * lz;
        const float scaledmass = (1.0f / invMass) * float(.08333333);

        Mat3 inertia(1.0f);
        inertia.SetDiagonal(Vec3(1.0f / scaledmass * (y2 + z2), 1.0f / scaledmass * (x2 + z2), 1.0f / scaledmass * (x2 + y2)));
        //        inertia[0][0] = 1.0f / scaledmass * (y2 + z2);
        //        inertia[1][1] = 1.0f / scaledmass * (x2 + z2);
        //        inertia[2][2] = 1.0f / scaledmass * (x2 + y2);

        return inertia;
    }

    TDArray<Vec3>& CapsuleCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        return m_Axes;
    }

    TDArray<CollisionEdge>& CapsuleCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void CapsuleCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        //        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        //        Vec3 pos       = transform[3];
        //
        //        Vec3 topPosition    = Vec3(transform * Vec4(0.0f, m_Height * 0.5f, 0.0f, 1.0f));
        //        Vec3 bottomPosition = Vec3(transform * Vec4(0.0f, -m_Height * 0.5f, 0.0f, 1.0f));
        //
        //        // Transform the axis into the local coordinate space of the capsule
        //        Mat4 inverseTransform = glm::affineInverse(transform);
        //        Vec3 localAxis        = Vec3(inverseTransform * Vec4(axis, 1.0f));
        //
        //        float minProj = Maths::Dot(topPosition, localAxis) - m_Radius;
        //        float maxProj = Maths::Dot(bottomPosition, localAxis) + m_Radius;
        //
        //        *out_min = topPosition + minProj * localAxis;
        //        *out_max = bottomPosition + maxProj * localAxis;

        float minCorrelation = FLT_MAX, maxCorrelation = -FLT_MAX;

        Mat4 transform       = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Vec3 localAxis = transform.Transpose() * Vec4(axis, 1.0f);

        Vec3 pos       = transform.GetPositionVector();
        Vec3 minVertex = Vec3(0.0f), maxVertex = Vec3(0.0f);

        Vec3 topPosition    = Vec3(transform * Vec4(0.0f, m_Height * 0.5f, 0.0f, 1.0f));
        Vec3 bottomPosition = Vec3(transform * Vec4(0.0f, -m_Height * 0.5f, 0.0f, 1.0f));

        // Transform the axis into the local coordinate space of the capsule
        // Mat4 inverseTransform = glm::affineInverse(transform);
        // Vec3 localAxis        = Vec3(inverseTransform * Vec4(axis, 1.0f));

        float minProj = Maths::Dot(topPosition, localAxis) - m_Radius;
        float maxProj = Maths::Dot(bottomPosition, localAxis) + m_Radius;

        if(out_min)
            *out_min = topPosition + minProj; // * localAxis;
        if(out_max)
            *out_max = bottomPosition + maxProj; // * localAxis;

        // Capsule Collision shape
        //		for(size_t i = 0; i < m_Vertices.size(); ++i)
        //		{
        //			const float cCorrelation = Maths::Dot(local_axis, m_Vertices[i].pos);
        //
        //			if(cCorrelation > maxCorrelation)
        //			{
        //				maxCorrelation = cCorrelation;
        //				maxVertex      = int(i);
        //			}
        //
        //			if(cCorrelation <= minCorrelation)
        //			{
        //				minCorrelation = cCorrelation;
        //				minVertex      = int(i);
        //			}
        //		}

        //		if(out_min)
        //			*out_min = pos - axis * m_Radius;
        //
        //		if(out_max)
        //			*out_max = pos + axis * m_Radius;
    }

    void CapsuleCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                            const Vec3& axis,
                                                            ReferencePolygon& refPolygon) const
    {
        refPolygon.Faces[0]  = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void CapsuleCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION();
        Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        Vec3 scale;
        Quat rotation;
        Vec3 translation;
        transform.Decompose(translation, rotation, scale);
        DebugRenderer::DebugDrawCapsule(translation, rotation, m_Height, m_Radius, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
}
