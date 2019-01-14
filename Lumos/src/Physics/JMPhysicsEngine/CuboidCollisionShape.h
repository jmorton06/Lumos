#pragma once
#include "LM.h"
#include "CollisionShape.h"
#include "Hull.h"

namespace Lumos
{
	class LUMOS_EXPORT CuboidCollisionShape : public CollisionShape
	{
	public:
		CuboidCollisionShape();
		explicit CuboidCollisionShape(const maths::Vector3& halfdims);
		~CuboidCollisionShape();

		//Collision Shape Functionality
		virtual maths::Matrix3 BuildInverseInertia(float invMass) const override;

		virtual void GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const override;
		virtual void GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const override;

		virtual void GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const override;
		virtual void GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const override;

		virtual void DebugDraw(const PhysicsObject3D* currentObject) const override;

		//Set Cuboid Dimensions
		void SetHalfWidth(float half_width) { m_CuboidHalfDimensions.SetX(fabs(half_width)); }
		void SetHalfHeight(float half_height) { m_CuboidHalfDimensions.SetY(fabs(half_height)); }
		void SetHalfDepth(float half_depth) { m_CuboidHalfDimensions.SetZ(fabs(half_depth)); }

		//Get Cuboid Dimensions
		float GetHalfWidth()	const { return m_CuboidHalfDimensions.GetX(); }
		float GetHalfHeight()	const { return m_CuboidHalfDimensions.GetY(); }
		float GetHalfDepth()	const { return m_CuboidHalfDimensions.GetZ(); }

		virtual float GetSize() const override { return m_CuboidHalfDimensions.x; }

	protected:
		//Constructs the static cube hull
		static void ConstructCubeHull();

	protected:
		maths::Vector3 m_CuboidHalfDimensions;

		static std::shared_ptr<Hull> m_CubeHull;
	};

}