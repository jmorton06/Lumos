#include "Precompiled.h"
#include "PathEdge.h"

namespace Lumos
{

    PathEdge::PathEdge(PathNode* a, PathNode* b)
        : m_NodeA(a)
        , m_NodeB(b)
        , m_Traversable(true)
        , m_Weight(1.0f)
    {
        // Associate this edge with the nodes
        m_NodeA->m_connections.push_back(this);
        m_NodeB->m_connections.push_back(this);
    }

    PathEdge::~PathEdge()
    {
    }

    PathNode* PathEdge::OtherNode(PathNode* node)
    {
        PathNode* retVal = nullptr;

        if(node == m_NodeA)
            retVal = m_NodeB;
        else if(node == m_NodeB)
            retVal = m_NodeA;

        return retVal;
    }

    float PathEdge::StaticCost() const
    {
        return (m_NodeA->GetWorldSpaceTransform().Translation() - m_NodeB->GetWorldSpaceTransform().Translation()).Length();
    }

    void PathEdge::SetTraversable(bool traversable)
    {
        m_Traversable = traversable;
    }

    void PathEdge::SetWeight(float weight)
    {
        m_Weight = weight;
    }

    bool PathEdge::operator==(const PathEdge& other) const
    {
        return (m_NodeA == other.m_NodeA && m_NodeB == other.m_NodeB) || (m_NodeA == other.m_NodeB && m_NodeB == other.m_NodeA);
    }

    bool PathEdge::operator!=(const PathEdge& other) const
    {
        return !this->operator==(other);
    }

}
