#pragma once
#include "LM.h"

#include "PathNode.h"
#include "PathNodePriorityQueue.h"
#include "QueueablePathNode.h"

namespace Lumos
{
	class LUMOS_EXPORT AStar
	{
	public:
		explicit AStar(const std::vector<PathNode *> &nodes);
		virtual ~AStar();

		void Reset();
		bool FindPath(PathNode *start, PathNode *end);
		
		inline PathNodePriorityQueue OpenList() const
		{
			return m_OpenList;
		}

		inline std::vector<QueueablePathNode *> ClosedList() const
		{
			return m_ClosedList;
		}

		inline std::vector<PathNode *> Path() const
		{
			return m_Path;
		}

		inline float PathCost() const
		{
			return m_ClosedList.back()->gScore;
		}

	private:
		std::map<PathNode *, QueueablePathNode *> m_NodeData;
		PathNodePriorityQueue m_OpenList;
		std::vector<QueueablePathNode*> m_ClosedList;
		std::vector<PathNode*> m_Path;
	};
}