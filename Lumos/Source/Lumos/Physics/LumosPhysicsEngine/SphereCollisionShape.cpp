#include "Precompiled.h"
#include "SphereCollisionShape.h"
#include "RigidBody3D.h"
#include "Maths/Matrix3.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    SphereCollisionShape::SphereCollisionShape()
    {
        m_Radius = 1.0f;
        m_LocalTransform = Maths::Matrix4::Scale(Maths::Vector3(m_Radius));
        m_Type = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::SphereCollisionShape(float radius)
    {
        m_Radius = radius;
        m_LocalTransform = Maths::Matrix4::Scale(Maths::Vector3(m_Radius));
        m_Type = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::~SphereCollisionShape()
    {
    }

    Maths::Matrix3 SphereCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        float i = 2.5f * invMass / (m_Radius * m_Radius); //SOLID
        //float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

        Maths::Matrix3 inertia;
        inertia.m00_ = i;
        inertia.m11_ = i;
        inertia.m22_ = i;

        return inertia;
    }

    std::vector<Maths::Vector3>& SphereCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        m_Axes.clear();
        return m_Axes;
    }

    std::vector<CollisionEdge>& SphereCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void SphereCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        Maths::Vector3 pos = transform.Translation();

        if(out_min)
            *out_min = pos - axis * m_Radius;

        if(out_max)
            *out_max = pos + axis * m_Radius;
    }

    void SphereCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const Maths::Vector3& axis,
        ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        refPolygon.Faces[0] = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void SphereCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        auto pos = transform.Translation();
        auto sphere = Maths::Sphere(pos, m_Radius);
        DebugRenderer::DebugDraw(sphere, Maths::Vector4(1.0f, 1.0f, 1.0f, 0.2f));
        DebugRenderer::DebugDrawSphere(m_Radius, pos, Maths::Vector4(1.0f, 0.3f, 1.0f, 1.0f));
    }
}
