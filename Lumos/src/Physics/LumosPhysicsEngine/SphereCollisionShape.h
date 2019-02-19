#pragma once

#include "LM.h"
#include "CollisionShape.h"

namespace Lumos
{

	class LUMOS_EXPORT SphereCollisionShape : public CollisionShape
	{
	public:
		SphereCollisionShape();
		explicit SphereCollisionShape(float radius);
		~SphereCollisionShape();

		//Collision Shape Functionality
		virtual maths::Matrix3 BuildInverseInertia(float invMass) const override;

		virtual void GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const override;
		virtual void GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const override;

		virtual void GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const override;
		virtual void GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const override;

		virtual void DebugDraw(const PhysicsObject3D* currentObject) const override;

		//Get/Set Sphere Radius
		void SetRadius(float radius) { m_Radius = radius; }
		float GetRadius() const  { return m_Radius; }

		float GetSize() const override { return m_Radius; }

	protected:

		float m_Radius;
	};
}