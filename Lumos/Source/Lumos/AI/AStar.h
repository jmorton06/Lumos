#pragma once

#include "PathNode.h"
#include "PathNodePriorityQueue.h"
#include "QueueablePathNode.h"
#include "Core/DataStructures/Map.h"

namespace Lumos
{
    class AStar
    {
    public:
        explicit AStar(const TDArray<PathNode*>& nodes);
        virtual ~AStar();

        void Reset();
        bool FindPath(PathNode* start, PathNode* end);

        PathNodePriorityQueue OpenList() const
        {
            return m_OpenList;
        }

        const TDArray<QueueablePathNode*>& ClosedList() const
        {
            return m_ClosedList;
        }

        const TDArray<PathNode*>& Path() const
        {
            return m_Path;
        }

        float PathCost() const
        {
            return m_ClosedList.Back()->gScore;
        }

    private:
        HashMap(PathNode*, QueueablePathNode*) m_NodeData;
        PathNodePriorityQueue m_OpenList;
        TDArray<QueueablePathNode*> m_ClosedList;
        TDArray<PathNode*> m_Path;
    };
}
