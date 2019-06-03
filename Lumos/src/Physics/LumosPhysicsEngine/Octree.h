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
		Octree(size_t maxObjectsPerPartition, size_t maxPartitionDepth, std::shared_ptr<Broadphase> secondaryBroadphase);
		virtual ~Octree();

		struct OctreeNode
		{
			OctreeNode()
			{
			}
			~OctreeNode()
			{
			}

			std::vector<std::shared_ptr<PhysicsObject3D>> physicsObjects;
			std::vector<std::shared_ptr<OctreeNode>>	  childNodes;
			Maths::BoundingBox							  boundingBox;
		};

		void FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;
		void Divide(const std::shared_ptr<OctreeNode>& node, size_t iteration);
		static void DebugDrawOctreeNode(OctreeNode* node);

	private:
		size_t m_MaxObjectsPerPartition;
		size_t m_MaxPartitionDepth;

		std::shared_ptr<Broadphase> m_SecondaryBroadphase; //Broadphase stage used to determine collision pairs within subdivisions
		std::shared_ptr<OctreeNode> m_RootNode;
		std::vector<std::shared_ptr<OctreeNode>> m_LeafNodes;
	};
}