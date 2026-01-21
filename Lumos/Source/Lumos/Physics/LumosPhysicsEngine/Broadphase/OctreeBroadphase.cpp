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
        m_Arena            = ArenaAlloc(Megabytes(8));
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
        //ArenaClear(m_Arena);
        ArenaRelease(m_Arena);
        m_Arena = ArenaAlloc(Megabytes(8));

        m_LeafCount = 0;

        m_RootNode.ChildCount         = 0;
        m_RootNode.PhysicsObjectCount = 0;
        m_RootNode.boundingBox        = Maths::BoundingBox();
        m_RootNode.PhysicsObjects     = PushArrayNoZero(m_Arena, RigidBody3D*, totalRigidBodyCount);
#define LEAF_COUNT 1024
        m_Leaves = PushArrayNoZero(m_Arena, OctreeNode*, LEAF_COUNT);

        // Early exit if no objects
        if(totalRigidBodyCount == 0)
            return;

        for(i32 i = 0; i < totalRigidBodyCount; i++)
        {
            RigidBody3D& current = rootObject[i];
            if(current.GetIsValid() && current.GetCollisionShape())
            {
                LUMOS_PROFILE_SCOPE_LOW("Merge Bounding box and add Physics Object");
                m_RootNode.boundingBox.Merge(current.GetWorldSpaceAABB());
                m_RootNode.PhysicsObjects[m_RootNode.PhysicsObjectCount] = &current;
                m_RootNode.PhysicsObjectCount++;
            }
        }

        // Early exit if no valid objects with collision shapes
        if(m_RootNode.PhysicsObjectCount == 0)
            return;

        m_RootNode.boundingBox.ExtendToCube();

        // Recursively divide world
        Divide(m_RootNode, 0);

        HashSet(size_t) collisionPairHashSet = { 0 };
        //collisionPairHashSet.arena           = m_Arena;

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

                    // Skip pairs of two static objects
                    if(obj1.GetIsStatic() && obj2.GetIsStatic())
                        continue;

                    // Skip pairs of two non-static objects that are both at rest
                    // (Don't skip static-dynamic pairs even if dynamic object is at rest,
                    // because another dynamic object could push the at-rest object into the static one)
                    if(obj1.GetIsAtRest() && obj2.GetIsAtRest() && !obj1.GetIsStatic() && !obj2.GetIsStatic())
                        continue;

                    // Skip pairs filtered out by collision layers
                    if(!obj1.CanCollideWith(&obj2))
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

        // Hard limit to prevent stack overflow (iOS has 512KB stack)
        constexpr size_t MAX_SAFE_DEPTH = 10;
        if(iteration > MAX_SAFE_DEPTH)
        {
            if(division.PhysicsObjectCount > 1 && m_LeafCount < LEAF_COUNT)
            {
                m_Leaves[m_LeafCount] = &division;
                m_LeafCount++;
            }
            return;
        }

        // Sanity check - if division is corrupted, bail out
        if(division.ChildCount > 8 || division.PhysicsObjectCount > 100000)
        {
            LERROR("Octree corruption detected: ChildCount=%u, PhysicsObjectCount=%u, depth=%zu",
                   division.ChildCount, division.PhysicsObjectCount, iteration);
            return;
        }

        // Exit conditions (partition depth limit or target object count reached)
        if(iteration > m_MaxPartitionDepth || division.PhysicsObjectCount <= m_MaxObjectsPerPartition || division.boundingBox.Size().x <= (float)m_MinPartitionSize)
        {
            LUMOS_PROFILE_SCOPE_LOW("Add Leaf");
            // Ignore any subdivisions that contain no objects
            if(division.PhysicsObjectCount > 1 && m_LeafCount < LEAF_COUNT)
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

        // Allocate children array - check for arena overflow
        OctreeNode* children = PushArray(m_Arena, OctreeNode, NUM_DIVISIONS);
        if(!children)
        {
            // Arena full - treat this node as a leaf
            LWARN("Octree arena overflow at depth %zu, treating as leaf", iteration);
            if(division.PhysicsObjectCount > 1)
            {
                m_Leaves[m_LeafCount] = &division;
                m_LeafCount++;
            }
            return;
        }
        division.Children = children;

        // Cache parent's physics object count before we start modifying children
        const uint32_t parentPhysicsObjectCount = division.PhysicsObjectCount;

        for(size_t i = 0; i < NUM_DIVISIONS; i++)
        {
            LUMOS_PROFILE_SCOPE_LOW("Create Child");

            // Use pointer to avoid reference aliasing issues
            OctreeNode* childNode = &division.Children[i];
            childNode->ChildCount         = 0;
            childNode->PhysicsObjectCount = 0;
            childNode->Children           = nullptr;
            childNode->PhysicsObjects     = PushArrayNoZero(m_Arena, RigidBody3D*, parentPhysicsObjectCount);

            // Check for arena overflow on physics objects allocation
            if(!childNode->PhysicsObjects && parentPhysicsObjectCount > 0)
            {
                LWARN("Octree arena overflow allocating physics objects");
                return;
            }

            division.ChildCount++;

            const Vec3 lower(divisionPoints[DIVISION_POINT_INDICES[i][0]].x,
                             divisionPoints[DIVISION_POINT_INDICES[i][1]].y,
                             divisionPoints[DIVISION_POINT_INDICES[i][2]].z);
            const Vec3 upper(divisionPoints[DIVISION_POINT_INDICES[i][3]].x,
                             divisionPoints[DIVISION_POINT_INDICES[i][4]].y,
                             divisionPoints[DIVISION_POINT_INDICES[i][5]].z);

            childNode->boundingBox.m_Min = lower;
            childNode->boundingBox.m_Max = upper;

            // Add objects inside division
            for(uint32_t j = 0; j < parentPhysicsObjectCount; j++)
            {
                RigidBody3D* physicsObject = division.PhysicsObjects[j];

                // Skip null objects (already completely contained by sibling)
                if(!physicsObject)
                    continue;

                const Maths::BoundingBox& boundingBox = physicsObject->GetWorldSpaceAABB();
                Maths::Intersection intersection      = childNode->boundingBox.IsInside(boundingBox);
                if(intersection != Maths::Intersection::OUTSIDE)
                {
                    childNode->PhysicsObjects[childNode->PhysicsObjectCount] = physicsObject;
                    childNode->PhysicsObjectCount++;

                    // If object is completely inside this child, remove from parent to avoid redundant checks
                    if(intersection == Maths::Intersection::INSIDE)
                        division.PhysicsObjects[j] = nullptr;
                }
            }

            // Do further subdivisioning only if child has objects
            if(childNode->PhysicsObjectCount > 0)
                Divide(*childNode, iteration + 1);
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
