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
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        // Get the top and bottom positions of the capsule in world space
        Vec3 topPosition    = Vec3(transform * Vec4(0.0f, m_Height * 0.5f, 0.0f, 1.0f));
        Vec3 bottomPosition = Vec3(transform * Vec4(0.0f, -m_Height * 0.5f, 0.0f, 1.0f));

        // Project the top and bottom positions onto the axis
        float topProj    = Maths::Dot(topPosition, axis);
        float bottomProj = Maths::Dot(bottomPosition, axis);

        // Find the min and max projections (accounting for radius)
        float minProj = Maths::Min(topProj, bottomProj) - m_Radius;
        float maxProj = Maths::Max(topProj, bottomProj) + m_Radius;

        // Calculate the min and max points as world space positions
        // These are points along the axis that have the correct projections
        Vec3 axisNormalized = axis.Normalised();
        if(out_min)
            *out_min = axisNormalized * minProj;
        if(out_max)
            *out_max = axisNormalized * maxProj;
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
