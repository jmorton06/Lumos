#include "Precompiled.h"
#include "PathNode.h"
#include "PathEdge.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{

    PathNode::PathNode(const Vec3& position)
        : RigidBody3D()
    {
        SetPosition(position);
    }

    PathNode::~PathNode()
    {
    }

    bool PathNode::IsOnList(const std::vector<PathNode*>& list)
    {
        return std::find(list.begin(), list.end(), this) != list.end();
    }

    float PathNode::HeuristicValue(const PathNode& other) const
    {
        return Maths::Length(Vec3(GetWorldSpaceTransform().Translation() - other.GetWorldSpaceTransform().Translation()));
    }

}
