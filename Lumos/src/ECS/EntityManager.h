#pragma once

#include "LM.h"
#include "ECS.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
	class Entity;

	class LUMOS_EXPORT EntityManager : public TSingleton<EntityManager>
	{
		friend class TSingleton<EntityManager>;
		friend class Entity;
	public:
		EntityManager() = default;
		~EntityManager() { Clear(); }

		void Clear();
		Entity* CreateEntity(const String& name = "");
		void DeleteEntity(Entity* entity);

		const std::vector<Entity*>& GetEntities() const { return m_Entities; }

	private:
		std::vector<Entity*> m_Entities;
	};
}
