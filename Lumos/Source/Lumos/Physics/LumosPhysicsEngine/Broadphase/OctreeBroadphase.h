#pragma once
#include "Broadphase.h"
#include "Core/DataStructures/Set.h"

#define MAX_OBJECTS_PER_NODE 1024
#define MAX_PARTITION_DEPTH 16

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
        OctreeBroadphase(u32 maxObjectsPerPartition, u32 maxPartitionDepth, u32 maxLeafCount = 1024);
        virtual ~OctreeBroadphase();

        struct OctreeNode
        {
            OctreeNode()
                : ChildCount(0)
                , PhysicsObjectCount(0)
            {
            }

            ~OctreeNode()
            {
            }

            uint32_t ChildCount         = 0;
            uint32_t PhysicsObjectCount = 0;
            OctreeNode* Children;
            OctreeNode* NextLeaf = nullptr;
            RigidBody3D** PhysicsObjects;

            Maths::BoundingBox boundingBox;
        };

        void FindPotentialCollisionPairs(RigidBody3D* rootObject, TDArray<CollisionPair>& collisionPairs, uint32_t totalRigidBodyCount) override;
        void DebugDraw() override;
        void Divide(OctreeNode& node, size_t iteration);
        void DebugDrawOctreeNode(const OctreeNode& node);

    private:
        u32 m_MaxObjectsPerPartition;
        u32 m_MaxPartitionDepth;
        u32 m_MinPartitionSize;
        u32 m_MaxLeafCount;

        // leaves are chained through the nodes: no fixed limit to run past
        uint32_t m_LeafCount = 0;
        OctreeNode m_RootNode;
        OctreeNode* m_FirstLeaf = nullptr;
        void AddLeaf(OctreeNode& node);
        Arena* m_Arena;
        HashSet(uint64_t) m_PairSet = { 0 }; // reused every step, only grows
    };
}
