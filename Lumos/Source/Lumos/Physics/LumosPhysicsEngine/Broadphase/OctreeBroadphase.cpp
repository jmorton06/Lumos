#include "Precompiled.h"
#include "OctreeBroadphase.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include "Core/DataStructures/Set.h"
#include "Maths/MathsUtilities.h"

#define DEBUG_CHECK_DUPLICATES 0
namespace Lumos
{

    OctreeBroadphase::OctreeBroadphase(const u32 maxObjectsPerPartition, const u32 maxPartitionDepth)
        : m_MaxObjectsPerPartition(maxObjectsPerPartition)
        , m_MaxPartitionDepth(maxPartitionDepth)
        , m_Leaves()
    {
        m_Arena            = ArenaAlloc(Megabytes(4));
        m_MinPartitionSize = 2;
    }

    OctreeBroadphase::~OctreeBroadphase()
    {
        ArenaRelease(m_Arena);
    }

    void OctreeBroadphase::FindPotentialCollisionPairs(RigidBody3D* rootObject,
                                                       TDArray<CollisionPair>& collisionPairs, uint32_t totalRigidBodyCount)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaClear(m_Arena);
        m_LeafCount = 0;

        m_RootNode.ChildCount         = 0;
        m_RootNode.PhysicsObjectCount = 0;
        m_RootNode.boundingBox        = Maths::BoundingBox();
        m_RootNode.PhysicsObjects     = PushArrayNoZero(m_Arena, RigidBody3D*, totalRigidBodyCount);
#define LEAF_COUNT 1024
        m_Leaves = PushArrayNoZero(m_Arena, OctreeNode*, LEAF_COUNT);

        RigidBody3D* current = rootObject;
        while(current)
        {
            if(current->GetCollisionShape())
            {
                LUMOS_PROFILE_SCOPE_LOW("Merge Bounding box and add Physics Object");
                m_RootNode.boundingBox.Merge(current->GetWorldSpaceAABB());
                m_RootNode.PhysicsObjects[m_RootNode.PhysicsObjectCount] = current;
                m_RootNode.PhysicsObjectCount++;
            }

            current = current->m_Next;
        }

        m_RootNode.boundingBox.ExtendToCube();

        // Recursively divide world
        Divide(m_RootNode, 0);

        HashSet(size_t) collisionPairHashSet = { 0 };
        collisionPairHashSet.arena           = m_Arena;

        // Add collision pairs in leaf world divisions
        for(uint32_t leafIndex = 0; leafIndex < m_LeafCount; leafIndex++)
        {
            auto& node           = *m_Leaves[leafIndex];
            uint32_t objectCount = node.PhysicsObjectCount;
            if(objectCount == 0)
                continue;

            for(size_t i = 0; i < objectCount - 1; ++i)
            {
                if(!node.PhysicsObjects[i])
                    continue;

                RigidBody3D& obj1 = *node.PhysicsObjects[i];

                for(size_t j = i + 1; j < objectCount; ++j)
                {
                    if(!node.PhysicsObjects[j])
                        continue;

                    RigidBody3D& obj2 = *node.PhysicsObjects[j];

                    // Skip pairs of two at objects at rest
                    if(obj1.GetIsAtRest() && obj2.GetIsAtRest())
                        continue;

                    if(!obj1.GetWorldSpaceAABB().IsInsideFast(obj2.GetWorldSpaceAABB()))
                        continue;

                    CollisionPair pair;
                    if(&obj1 < &obj2)
                    {
                        pair.pObjectA = &obj1;
                        pair.pObjectB = &obj2;
                    }
                    else
                    {
                        pair.pObjectA = &obj2;
                        pair.pObjectB = &obj1;
                    }

                    size_t pairHash = (size_t)pair.pObjectA + ((size_t)pair.pObjectB << 8);
                    if(!HashSetContains(&collisionPairHashSet, pairHash))
                    {
                        HashSetAdd(&collisionPairHashSet, pairHash);
                    }
                    else
                        continue;

                    // Slow check for duplicates
#if DEBUG_CHECK_DUPLICATES
                    bool duplicate = false;
                    for(int i = 0; i < collisionPairs.Size(); i++)
                    {
                        auto& pair2 = collisionPairs[i];

                        if(pair.pObjectA == pair2.pObjectA && pair.pObjectB == pair2.pObjectB)
                        {
                            duplicate = true;
                        }
                    }
                    if(!duplicate)
#endif
                        collisionPairs.EmplaceBack(pair);
                }
            }
        }
    }

    void OctreeBroadphase::DebugDraw()
    {
        DebugDrawOctreeNode(m_RootNode);
    }

    void OctreeBroadphase::Divide(OctreeNode& division, const size_t iteration)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // Exit conditions (partition depth limit or target object count reached)
        if(iteration > m_MaxPartitionDepth || division.PhysicsObjectCount <= m_MaxObjectsPerPartition || division.boundingBox.Size().x <= (float)m_MinPartitionSize)
        {
            LUMOS_PROFILE_SCOPE_LOW("Add Leaf");
            // Ignore any subdivisions that contain no objects
            if(division.PhysicsObjectCount > 1)
            {
                m_Leaves[m_LeafCount] = &division;
                m_LeafCount++;
            }

            return;
        }

        Vec3 divisionPoints[3] = { division.boundingBox.Min(), division.boundingBox.Center(), division.boundingBox.Max() };

        static const size_t NUM_DIVISIONS                            = 8;
        static const size_t DIVISION_POINT_INDICES[NUM_DIVISIONS][6] = {
            { 0, 0, 0, 1, 1, 1 },
            { 1, 0, 0, 2, 1, 1 },
            { 0, 1, 0, 1, 2, 1 },
            { 1, 1, 0, 2, 2, 1 },
            { 0, 0, 1, 1, 1, 2 },
            { 1, 0, 1, 2, 1, 2 },
            { 0, 1, 1, 1, 2, 2 },
            { 1, 1, 1, 2, 2, 2 }
        };

        division.Children = PushArray(m_Arena, OctreeNode, NUM_DIVISIONS);

        for(size_t i = 0; i < NUM_DIVISIONS; i++)
        {
            LUMOS_PROFILE_SCOPE_LOW("Create Child");
            OctreeNode& chileNode        = division.Children[division.ChildCount];
            chileNode.ChildCount         = 0;
            chileNode.PhysicsObjectCount = 0;
            chileNode.PhysicsObjects     = PushArrayNoZero(m_Arena, RigidBody3D*, division.PhysicsObjectCount);

            division.ChildCount++;

            const Vec3 lower(divisionPoints[DIVISION_POINT_INDICES[i][0]].x,
                             divisionPoints[DIVISION_POINT_INDICES[i][1]].y,
                             divisionPoints[DIVISION_POINT_INDICES[i][2]].z);
            const Vec3 upper(divisionPoints[DIVISION_POINT_INDICES[i][3]].x,
                             divisionPoints[DIVISION_POINT_INDICES[i][4]].y,
                             divisionPoints[DIVISION_POINT_INDICES[i][5]].z);

            chileNode.boundingBox.m_Min = lower;
            chileNode.boundingBox.m_Max = upper;

            // Add objects inside division
            for(uint32_t i = 0; i < division.PhysicsObjectCount; i++)
            {
                LUMOS_PROFILE_SCOPE_LOW("PhysicsObject BB check");
                RigidBody3D* physicsObject = division.PhysicsObjects[i];

                if(!physicsObject || !physicsObject->GetCollisionShape())
                    continue;

                const Maths::BoundingBox& boundingBox = physicsObject->GetWorldSpaceAABB();
                Maths::Intersection intersection      = chileNode.boundingBox.IsInside(boundingBox);
                if(intersection != Maths::Intersection::OUTSIDE)
                {
                    chileNode.PhysicsObjects[chileNode.PhysicsObjectCount] = physicsObject;
                    chileNode.PhysicsObjectCount++;

                    if(intersection == Maths::Intersection::INSIDE)
                        division.PhysicsObjects[i] = nullptr;
                }
            }

            // Do further subdivisioning
            Divide(chileNode, iteration + 1);
        }
    }

    void OctreeBroadphase::DebugDrawOctreeNode(const OctreeNode& node)
    {
        DebugRenderer::DebugDraw(node.boundingBox, Vec4(0.8f, 0.2f, 0.4f, 1.0f), false, true, 0.1f);

        // Draw sub divisions
        for(uint32_t i = 0; i < node.ChildCount; i++)
            DebugDrawOctreeNode(node.Children[i]);
    }
}
