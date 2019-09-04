#include "LM.h"
#include "ComponentManager.h"
#include "ECS/Component/Components.h"

namespace Lumos
{
	ComponentManager::ComponentManager()
	{
		m_NextComponentType = 0;
		RegisterComponent<MeshComponent>();
		RegisterComponent<TransformComponent>();
		RegisterComponent<SpriteComponent>();
		RegisterComponent<LightComponent>();
		RegisterComponent<SoundComponent>();
		RegisterComponent<TextureMatrixComponent>();
		RegisterComponent<Physics3DComponent>();
		RegisterComponent<Physics2DComponent>();
		RegisterComponent<AIComponent>();
		RegisterComponent<ParticleComponent>();
		RegisterComponent<CameraComponent>();
		RegisterComponent<MaterialComponent>();
	}

	ComponentManager::~ComponentManager()
	{
	}

	void ComponentManager::OnUpdate()
	{
		for (auto& componentArray : m_ComponentArrays)
			componentArray.second->OnUpdate();
	}

	std::vector<LumosComponent*> ComponentManager::GetAllComponents(Entity* entity)
	{
		std::vector<LumosComponent*> components;

		for (auto& componentArray : m_ComponentArrays)
		{
			auto component = static_cast<ComponentArray<LumosComponent>*>(componentArray.second.get())->GetData(entity);
			if (component != nullptr)
				components.emplace_back(component);
		}

		return components;
	}
}
