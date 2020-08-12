#include "lmpch.h"
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
		float i = 2.5f * invMass / (m_Radius * m_Radius); //SOLID
		//float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

		Maths::Matrix3 inertia;
		inertia.m00_ = i;
		inertia.m11_ = i;
		inertia.m22_ = i;

		return inertia;
	}

	void SphereCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject, std::vector<Maths::Vector3>* out_axes) const
	{
		/* There is infinite edges so handle seperately */
	}

	void SphereCollisionShape::GetEdges(const RigidBody3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		/* There is infinite edges on a sphere so handle seperately */
	}

	void SphereCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
	{
		Maths::Matrix4 transform;

		if(currentObject == nullptr)
			transform = m_LocalTransform;
		else
			transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

		Maths::Vector3 pos = transform.Translation();

		if(out_min)
			*out_min = pos - axis * m_Radius;

		if(out_max)
			*out_max = pos + axis * m_Radius;
	}

	void SphereCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Maths::Vector3& axis, std::list<Maths::Vector3>* out_face, Maths::Vector3* out_normal, std::vector<Maths::Plane>* out_adjacent_planes) const
	{
		if(out_face)
		{
			out_face->push_back(currentObject->GetPosition() + axis * m_Radius);
		}

		if(out_normal)
		{
			*out_normal = axis;
		}
	}

	void SphereCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
	{
		Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

		auto pos = transform.Translation();
		auto sphere = Maths::Sphere(pos, m_Radius);
		DebugRenderer::DebugDraw(sphere, Maths::Vector4(1.0f, 1.0f, 1.0f, 0.2f));
        DebugRenderer::DebugDrawSphere(m_Radius,pos, Maths::Vector4(1.0f, 0.3f, 1.0f, 1.0f));
	}
}
