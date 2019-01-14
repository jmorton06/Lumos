#include "JM.h"
#include "SphereCollisionShape.h"
#include "PhysicsObject3D.h"
#include "Maths/Matrix3.h"

#include "Graphics/Renderers/DebugRenderer.h"

namespace jm
{

	SphereCollisionShape::SphereCollisionShape()
	{
		m_Radius = 1.0f;
		m_LocalTransform = maths::Matrix4::Scale(maths::Vector3(m_Radius));
		m_Type = CollisionShapeType::CollisionSphere;
	}

	SphereCollisionShape::SphereCollisionShape(float radius)
	{
		m_Radius = radius;
		m_LocalTransform = maths::Matrix4::Scale(maths::Vector3(m_Radius));
		m_Type = CollisionShapeType::CollisionSphere;
	}

	SphereCollisionShape::~SphereCollisionShape()
	{
	}

	maths::Matrix3 SphereCollisionShape::BuildInverseInertia(float invMass) const
	{
		float i = 2.5f * invMass / (m_Radius * m_Radius); //SOLID
		//float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

		maths::Matrix3 inertia;
		inertia._11 = i;
		inertia._22 = i;
		inertia._33 = i;

		return inertia;
	}

	void SphereCollisionShape::GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const
	{
		/* There is infinite edges so handle seperately */
	}

	void SphereCollisionShape::GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		/* There is infinite edges on a sphere so handle seperately */
	}

	void SphereCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const
	{
		maths::Matrix4 transform;

		if (currentObject == nullptr)
			transform = m_LocalTransform;
		else
			transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

		maths::Vector3 pos = transform.GetPositionVector();

		if (out_min)
			*out_min = pos - axis * m_Radius;

		if (out_max)
			*out_max = pos + axis * m_Radius;
	}

	void SphereCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const
	{
		if (out_face)
		{
			out_face->push_back(currentObject->GetPosition() + axis * m_Radius);
		}

		if (out_normal)
		{
			*out_normal = axis;
		}
	}

	void SphereCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
		maths::Vector3 pos = currentObject->GetPosition();

		//Draw Filled Circle
		DebugRenderer::DrawPoint(pos, m_Radius, maths::Vector4(1.0f, 1.0f, 1.0f, 0.2f));

		//Draw Perimeter Axes
		maths::Vector3 lastX = pos + maths::Vector3(0.0f, 1.0f, 0.0f) * m_Radius;
		maths::Vector3 lastY = pos + maths::Vector3(1.0f, 0.0f, 0.0f) * m_Radius;
		maths::Vector3 lastZ = pos + maths::Vector3(1.0f, 0.0f, 0.0f) * m_Radius;
		for (int itr = 1; itr < 21; ++itr)
		{
			float angle = itr / 20.0f * 6.2831853f;
			float alpha = cosf(angle) * m_Radius;
			float beta = sinf(angle) * m_Radius;

			maths::Vector3 newX = pos + maths::Vector3(0.0f, alpha, beta);
			maths::Vector3 newY = pos + maths::Vector3(alpha, 0.0f, beta);
			maths::Vector3 newZ = pos + maths::Vector3(alpha, beta, 0.0f);

			DebugRenderer::DrawThickLine(lastX, newX, 0.02f, maths::Vector4(1.0f, 0.3f, 1.0f, 1.0f));
			DebugRenderer::DrawThickLine(lastY, newY, 0.02f, maths::Vector4(1.0f, 0.3f, 1.0f, 1.0f));
			DebugRenderer::DrawThickLine(lastZ, newZ, 0.02f, maths::Vector4(1.0f, 0.3f, 1.0f, 1.0f));

			lastX = newX;
			lastY = newY;
			lastZ = newZ;
		}
	}
}