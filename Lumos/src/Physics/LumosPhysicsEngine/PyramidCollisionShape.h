#pragma once

#include "LM.h"
#include "CollisionShape.h"
#include "Hull.h"

namespace lumos
{

	class LUMOS_EXPORT PyramidCollisionShape : public CollisionShape
	{
	public:
		PyramidCollisionShape();
		PyramidCollisionShape(const maths::Vector3& halfdims);
		~PyramidCollisionShape();

		//Collision Shape Functionality
		virtual maths::Matrix3 BuildInverseInertia(float invMass) const override;

		virtual void GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const override;
		virtual void GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const override;

		virtual void GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const override;
		virtual void GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const override;

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
		maths::Vector3		m_PyramidHalfDimensions;
		static Hull*		m_PyramidHull;
		maths::Vector3		m_Normals[5];
	};
}