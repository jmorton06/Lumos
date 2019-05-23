#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace lumos
{
	class PhysicsObject3D;

	struct LUMOS_EXPORT CollisionEdge
	{
		CollisionEdge(const maths::Vector3& a, const maths::Vector3& b)
			: posA(a), posB(b) {}

		maths::Vector3 posA;
		maths::Vector3 posB;
	};

	enum CollisionShapeType : unsigned int
	{
		CollisionCuboid = 1,
		CollisionSphere = 2,
		CollisionPyramid = 3,
		CollisionShapeTypeMax
	};

	class LUMOS_EXPORT CollisionShape
	{
	public:
		CollisionShape(): m_Type() { m_LocalTransform.ToIdentity(); }
		virtual ~CollisionShape() {}

		// Constructs an inverse inertia matrix of the given collision volume. This is the equivilant of the inverse mass of an object for rotation,
		//   a good source for non-inverse inertia matricies can be found here: https://en.wikipedia.org/wiki/List_of_moments_of_inertia
		virtual maths::Matrix3 BuildInverseInertia(float invMass) const = 0;
		virtual float GetSize() const = 0;

		// Draws this collision shape to the debug renderer
		virtual void DebugDraw(const PhysicsObject3D* currentObject) const = 0;

		//<----- USED BY COLLISION DETECTION ----->
		// Get all possible collision axes
		//	- This is a list of all the face normals ignoring any duplicates and parallel vectors.
		virtual void GetCollisionAxes(
			const PhysicsObject3D* currentObject,
			std::vector<maths::Vector3>* out_axes) const = 0;

		// Get all shape Edges
		//	- Returns a list of all edges AB that form the convex hull of the collision shape. These are
		//    used to check edge/edge collisions aswell as finding the closest point to a sphere. */
		virtual void GetEdges(
			const PhysicsObject3D* currentObject,
			std::vector<CollisionEdge>* out_edges) const = 0;

		// Get the min/max vertices along a given axis
		virtual void GetMinMaxVertexOnAxis(
			const PhysicsObject3D* currentObject,
			const maths::Vector3& axis, maths::Vector3* out_min,
			maths::Vector3* out_max) const = 0;

		// Get all data needed to build manifold
		//	- Computes the face that is closest to parallel to that of the given axis,
		//    returning the face (as a list of vertices), face normal and the planes
		//    of all adjacent faces in order to clip against.
		virtual void GetIncidentReferencePolygon(const PhysicsObject3D* currentObject,
			const maths::Vector3& axis,
			std::list<maths::Vector3>* out_face,
			maths::Vector3* out_normal,
			std::vector<maths::Plane>* out_adjacent_planes) const = 0;

		void SetLocalTransform(const maths::Matrix4& transform){ m_LocalTransform = transform; }

		inline CollisionShapeType GetType() const { return m_Type; }

	protected:
		CollisionShapeType m_Type;
		maths::Matrix4 m_LocalTransform;
	};
}
