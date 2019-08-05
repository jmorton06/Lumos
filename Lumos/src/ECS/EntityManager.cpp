#include "LM.h"
#include "EntityManager.h"
#include "Entity.h"

void Lumos::EntityManager::Clear()
{
	for (auto entity : m_Entities)
		delete entity;

	m_Entities.clear();
}

Lumos::Entity* Lumos::EntityManager::CreateEntity(const String& name)
{
	auto entity = lmnew Entity(name);
	m_Entities.emplace_back(entity);
	return entity;
}

void Lumos::EntityManager::DeleteEntity(Entity* entity)
{
	for (int i = 0; i < m_Entities.size(); i++)
	{
		if (m_Entities[i] == entity)
		{
            if(entity != nullptr)
                delete entity;
			m_Entities.erase(m_Entities.begin() + i);
		}
	}
}
