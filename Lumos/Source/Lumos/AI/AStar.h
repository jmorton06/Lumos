#pragma once

#include "PathNode.h"
#include "PathNodePriorityQueue.h"
#include "QueueablePathNode.h"

namespace Lumos
{
    class AStar
    {
    public:
        explicit AStar(const std::vector<PathNode*>& nodes);
        virtual ~AStar();

        void Reset();
        bool FindPath(PathNode* start, PathNode* end);

        PathNodePriorityQueue OpenList() const
        {
            return m_OpenList;
        }

        const std::vector<QueueablePathNode*>& ClosedList() const
        {
            return m_ClosedList;
        }

        const std::vector<PathNode*>& Path() const
        {
            return m_Path;
        }

        float PathCost() const
        {
            return m_ClosedList.back()->gScore;
        }

    private:
        std::map<PathNode*, QueueablePathNode*> m_NodeData;
        PathNodePriorityQueue m_OpenList;
        std::vector<QueueablePathNode*> m_ClosedList;
        std::vector<PathNode*> m_Path;
    };
}
