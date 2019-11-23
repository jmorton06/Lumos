#pragma once
#include "lmpch.h"

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
		
		_FORCE_INLINE_ PathNodePriorityQueue OpenList() const
		{
			return m_OpenList;
		}

		_FORCE_INLINE_ std::vector<QueueablePathNode *> ClosedList() const
		{
			return m_ClosedList;
		}

		_FORCE_INLINE_ std::vector<PathNode *> Path() const
		{
			return m_Path;
		}

		_FORCE_INLINE_ float PathCost() const
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
