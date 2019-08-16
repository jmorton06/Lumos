#pragma once

#include "LM.h"
#include "Broadphase.h"

namespace Lumos
{
	class PhysicsObject3D;
	struct CollisionPair;

	namespace Maths
	{
		class BoundingBox;
	}

	class LUMOS_EXPORT Octree : public Broadphase
	{
	public:
		Octree(size_t maxObjectsPerPartition, size_t maxPartitionDepth, const Ref<Broadphase>& secondaryBroadphase);
		virtual ~Octree();

		struct OctreeNode
		{
			OctreeNode()
			{
			}
			~OctreeNode()
			{
			}

			std::vector<Ref<PhysicsObject3D>> physicsObjects;
			std::vector<Ref<OctreeNode>>	  childNodes;
			Maths::BoundingBox							  boundingBox;
		};

		void FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>>& objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;
		void Divide(const Ref<OctreeNode>& node, size_t iteration);
		static void DebugDrawOctreeNode(OctreeNode* node);

	private:
		size_t m_MaxObjectsPerPartition;
		size_t m_MaxPartitionDepth;

		Ref<Broadphase> m_SecondaryBroadphase; //Broadphase stage used to determine collision pairs within subdivisions
		Ref<OctreeNode> m_RootNode;
		std::vector<Ref<OctreeNode>> m_LeafNodes;
	};
}
