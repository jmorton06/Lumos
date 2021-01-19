#pragma once
#include "Broadphase.h"

#define MAX_OBJECTS_PER_NODE 1024
#define MAX_PARTITION_DEPTH 8

namespace Lumos
{
	class RigidBody3D;
	struct CollisionPair;

	namespace Maths
	{
		class BoundingBox;
	}

	class LUMOS_EXPORT OctreeBroadphase : public Broadphase
	{
	public:
        OctreeBroadphase(size_t maxObjectsPerPartition, size_t maxPartitionDepth, const Ref<Broadphase>& secondaryBroadphase);
		virtual ~OctreeBroadphase();

		struct OctreeNode
		{
			OctreeNode()
				: Index(0)
				, ChildCount(0)
				, PhysicsObjectCount(0)
			{
			}

			~OctreeNode()
			{
			}
			
            u32 Index = 0;
			u32 ChildCount = 0;
			u32 PhysicsObjectCount = 0;
			u32 ChildNodeIndices[8] = {0};
			Ref<RigidBody3D> PhysicsObjects[MAX_OBJECTS_PER_NODE];
			
			Maths::BoundingBox boundingBox;
		};

		void FindPotentialCollisionPairs(Ref<RigidBody3D>* objects, u32 objectCount, std::vector<CollisionPair>& collisionPairs) override;
		void DebugDraw() override;
		void Divide(OctreeNode& node, size_t iteration);
        void DebugDrawOctreeNode(const OctreeNode& node);

	private:
		size_t m_MaxObjectsPerPartition;
		size_t m_MaxPartitionDepth;
        
        u32 m_CurrentPoolIndex = 0;
        u32 m_LeafCount = 0;

		Ref<Broadphase> m_SecondaryBroadphase; //Broadphase stage used to determine collision pairs within subdivisions
		OctreeNode m_NodePool[MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH];
		u32 m_Leaves[MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH] = {0};
	};
}
