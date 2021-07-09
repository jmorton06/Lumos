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
        OctreeBroadphase(size_t maxObjectsPerPartition, size_t maxPartitionDepth, const SharedRef<Broadphase>& secondaryBroadphase);
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

            uint32_t Index = 0;
            uint32_t ChildCount = 0;
            uint32_t PhysicsObjectCount = 0;
            uint32_t ChildNodeIndices[8] = { 0 };
            RigidBody3D* PhysicsObjects[MAX_OBJECTS_PER_NODE];

            Maths::BoundingBox boundingBox;
        };

        void FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount, std::vector<CollisionPair>& collisionPairs) override;
        void DebugDraw() override;
        void Divide(OctreeNode& node, size_t iteration);
        void DebugDrawOctreeNode(const OctreeNode& node);

    private:
        size_t m_MaxObjectsPerPartition;
        size_t m_MaxPartitionDepth;

        uint32_t m_CurrentPoolIndex = 0;
        uint32_t m_LeafCount = 0;

        SharedRef<Broadphase> m_SecondaryBroadphase; //Broadphase stage used to determine collision pairs within subdivisions
        OctreeNode m_NodePool[MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH];
        uint32_t m_Leaves[MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH * MAX_PARTITION_DEPTH] = { 0 };
    };
}
