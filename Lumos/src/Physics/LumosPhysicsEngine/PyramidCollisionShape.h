#pragma once

#include "LM.h"
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

		virtual void GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<Maths::Vector3>* out_axes) const override;
		virtual void GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const override;

		virtual void GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const override;
		virtual void GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, std::list<Maths::Vector3>* out_face, Maths::Vector3* out_normal, std::vector<Maths::Plane>* out_adjacent_planes) const override;

		virtual void DebugDraw(const PhysicsObject3D* currentObject) const override;

		//Set Cuboid Dimensions
		void SetHalfWidth(float half_width) { m_PyramidHalfDimensions.SetX(fabs(half_width)); }
		void SetHalfHeight(float half_height) { m_PyramidHalfDimensions.SetY(fabs(half_height)); }
		void SetHalfDepth(float half_depth) { m_PyramidHalfDimensions.SetZ(fabs(half_depth)); }

		//Get Cuboid Dimensions
		float GetHalfWidth()	const { return m_PyramidHalfDimensions.GetX(); }
		float GetHalfHeight()	const { return m_PyramidHalfDimensions.GetY(); }
		float GetHalfDepth()	const { return m_PyramidHalfDimensions.GetZ(); }

		float GetSize() const override { return m_PyramidHalfDimensions.x; };

	protected:
		//Constructs the static cube hull
		static void ConstructPyramidHull();

	protected:
		Maths::Vector3		m_PyramidHalfDimensions;
		Maths::Vector3		m_Normals[5];
		
		static std::unique_ptr<Hull> m_PyramidHull;
	};
}