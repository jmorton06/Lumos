#include "Precompiled.h"
#include "AStar.h"
#include "PathEdge.h"

namespace Lumos
{

    AStar::AStar(const std::vector<PathNode*>& nodes)
    {
        // Create node data
        for(auto it = nodes.begin(); it != nodes.end(); ++it)
            m_NodeData[*it] = new QueueablePathNode(*it);
    }

    AStar::~AStar()
    {
    }

    void AStar::Reset()
    {
        // Clear caches
        m_OpenList = PathNodePriorityQueue();
        m_ClosedList.clear();
        m_Path.clear();

        // Reset node data
        for(auto it = m_NodeData.begin(); it != m_NodeData.end(); ++it)
        {
            it->second->Parent = nullptr;
            it->second->fScore = std::numeric_limits<float>::max();
            it->second->gScore = std::numeric_limits<float>::max();
        }
    }

    bool AStar::FindPath(PathNode* start, PathNode* end)
    {
        // Clear caches
        Reset();

        // Add start node to open list
        m_NodeData[start]->gScore = 0.0f;
        m_NodeData[start]->fScore = m_NodeData[start]->node->HeuristicValue(*end);
        m_OpenList.Push(m_NodeData[start]);

        bool success = false;
        while(!m_OpenList.empty())
        {
            QueueablePathNode* p = m_OpenList.Top();

            // Move this node to the closed list
            m_OpenList.Pop();
            m_ClosedList.push_back(p);

            // Check if this is the end node
            if(p->node == end)
            {
                success = true;
                break;
            }

            // For each node connected to the next node
            for(size_t i = 0; i < p->node->NumConnections(); i++)
            {
                PathEdge* pq = p->node->Edge(i);

                // Skip an edge that cannot be traversed
                if(!pq->Traversable())
                    continue;

                QueueablePathNode* q = m_NodeData[pq->OtherNode(p->node)];

                // Calculate new scores
                float gScore = p->gScore + pq->Cost();
                float fScore = gScore + q->node->HeuristicValue(*end);

                // Search for this node on open and closed lists
                auto closedIt = std::find(m_ClosedList.begin(), m_ClosedList.end(), q);
                auto openIt = m_OpenList.Find(q);

                if(closedIt != m_ClosedList.end() || openIt != m_OpenList.end())
                {
                    // Check if this path is more efficient that the previous best
                    if(q->gScore > gScore)
                    {
                        q->Parent = p;
                        q->gScore = gScore;
                        q->fScore = fScore;
                        m_OpenList.Update();
                    }
                }
                else
                {
                    // Add this path to the open list if it has yet to be considered
                    q->Parent = p;
                    q->gScore = gScore;
                    q->fScore = fScore;
                    m_OpenList.Push(q);
                }
            }
        }

        // If successful then reconstruct the best path
        if(success)
        {
            // Add nodes to path
            QueueablePathNode* n = m_ClosedList.back();
            while(n)
            {
                m_Path.push_back(n->node);
                n = n->Parent;
            }

            // Reverse path to be ordered start to end
            std::reverse(m_Path.begin(), m_Path.end());
        }

        return success;
    }

}