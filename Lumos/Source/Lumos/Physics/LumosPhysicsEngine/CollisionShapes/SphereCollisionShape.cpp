#include "Precompiled.h"
#include "SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Matrix3.h"

namespace Lumos
{

    SphereCollisionShape::SphereCollisionShape()
    {
        m_Radius         = 1.0f;
        m_LocalTransform = Mat4::Scale(Vec3(m_Radius * 2.0f));
        m_Type           = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::SphereCollisionShape(float radius)
    {
        m_Radius         = radius;
        m_LocalTransform = Mat4::Scale(Vec3(m_Radius * 2.0f));
        m_Type           = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::~SphereCollisionShape()
    {
    }

    Mat3 SphereCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        float i = 2.5f * invMass / (m_Radius * m_Radius); // SOLID
        // float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

        Mat3 inertia = Mat3(1.0f);
        inertia.SetDiagonal(Vec3(i, i, i));

        return inertia;
    }

    TDArray<Vec3>& SphereCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        m_Axes.Clear();
        return m_Axes;
    }

    TDArray<CollisionEdge>& SphereCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void SphereCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        Vec3 pos = transform.GetPositionVector();

        if(out_min)
            *out_min = pos - axis * m_Radius;

        if(out_max)
            *out_max = pos + axis * m_Radius;
    }

    void SphereCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                           const Vec3& axis,
                                                           ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        refPolygon.Faces[0]  = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void SphereCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        Vec3 pos    = transform.GetPositionVector();
        auto sphere = Maths::BoundingSphere(pos, m_Radius);
        DebugRenderer::DebugDraw(sphere, Vec4(1.0f, 1.0f, 1.0f, 0.2f));
        DebugRenderer::DebugDrawSphere(m_Radius, pos, Vec4(1.0f, 0.3f, 1.0f, 1.0f));
    }
}
