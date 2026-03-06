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
        // Proper capsule inertia: cylinder (height h, radius r) + two hemisphere caps
        // Capsule oriented along Y-axis
        const float r  = m_Radius;
        const float h  = m_Height;
        const float r2 = r * r;
        const float h2 = h * h;

        // Volume fractions for mass distribution
        const float cylVol = Maths::M_PI * r2 * h;
        const float sphVol = (4.0f / 3.0f) * Maths::M_PI * r2 * r;
        const float totalVol = cylVol + sphVol;

        const float mass = 1.0f / invMass;
        const float massCyl = mass * cylVol / totalVol;
        const float massSph = mass * sphVol / totalVol;

        // Cylinder inertia about center (Y-aligned)
        const float Ixx_cyl = massCyl * (r2 / 4.0f + h2 / 12.0f);
        const float Iyy_cyl = massCyl * r2 / 2.0f;

        // Sphere inertia + parallel axis for hemispheres offset from center
        // Hemisphere COM is 3r/8 from flat face, so distance from capsule center = h/2 + 3r/8
        const float d = h / 2.0f + 3.0f * r / 8.0f;
        const float Ixx_sph = massSph * (2.0f * r2 / 5.0f + d * d);
        const float Iyy_sph = massSph * 2.0f * r2 / 5.0f;

        const float Ixx = Ixx_cyl + Ixx_sph;
        const float Iyy = Iyy_cyl + Iyy_sph;
        const float Izz = Ixx; // Symmetric about Y

        Mat3 inertia(1.0f);
        inertia.SetDiagonal(Vec3(1.0f / Ixx, 1.0f / Iyy, 1.0f / Izz));
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

        // Calculate the min and max points as actual world space positions on the capsule surface
        Vec3 axisNormalized = axis.Normalised();

        // Find which endpoint is more extreme along the axis
        bool topIsMax = topProj > bottomProj;
        Vec3 maxEndpoint = topIsMax ? topPosition : bottomPosition;
        Vec3 minEndpoint = topIsMax ? bottomPosition : topPosition;

        if(out_min)
            *out_min = minEndpoint - axisNormalized * m_Radius;
        if(out_max)
            *out_max = maxEndpoint + axisNormalized * m_Radius;
    }

    void CapsuleCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                            const Vec3& axis,
                                                            ReferencePolygon& refPolygon) const
    {
        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        // Get the top and bottom hemisphere centers
        Vec3 topPosition    = Vec3(transform * Vec4(0.0f, m_Height * 0.5f, 0.0f, 1.0f));
        Vec3 bottomPosition = Vec3(transform * Vec4(0.0f, -m_Height * 0.5f, 0.0f, 1.0f));

        // Determine which hemisphere is more aligned with the contact axis
        Vec3 axisNorm = axis.Normalised();
        float topDot    = Maths::Dot((topPosition - currentObject->GetPosition()).Normalised(), axisNorm);
        float bottomDot = Maths::Dot((bottomPosition - currentObject->GetPosition()).Normalised(), axisNorm);

        // Use the hemisphere center that best faces the axis
        Vec3 closestHemisphereCenter = topDot > bottomDot ? topPosition : bottomPosition;

        // Contact point is on the sphere surface of the hemisphere
        refPolygon.Faces[0]  = closestHemisphereCenter + axisNorm * m_Radius;
        refPolygon.FaceCount = 1;
        refPolygon.Normal    = axisNorm;
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
