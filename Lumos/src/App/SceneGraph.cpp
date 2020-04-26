#include "lmpch.h"
#include "SceneGraph.h"
#include "Maths/Transform.h"

namespace Lumos
{
	SceneGraph::SceneGraph()
	{
	}

	void SceneGraph::Init(entt::registry & registry)
	{
		//registry.on_construct<Hierarchy>().connect<&Hierarchy::on_construct>();
		//registry.on_update<Hierarchy>().connect<&Hierarchy::on_update>();
		//registry.on_destroy<Hierarchy>().connect<&Hierarchy::on_destroy>();
	}

	void SceneGraph::Update(entt::registry & registry)
    {
		auto view = registry.view<Maths::Transform>();

		if (view.empty())
			return;

		for (auto entity : view)
		{
			const auto hierarchy = registry.try_get<Hierarchy>(entity);
			if (hierarchy && hierarchy->parent() != entt::null)
			{
				UpdateTransform(entity, registry);
			}
			else
			{
				registry.get<Maths::Transform>(entity).SetWorldMatrix(Maths::Matrix4());
			}
		}
    }

	void SceneGraph::UpdateTransform(entt::entity entity, entt::registry & registry)
	{
		auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
		if (hierarchyComponent)
		{
			auto transform = registry.try_get<Maths::Transform>(entity);
			if (transform && hierarchyComponent->parent() != entt::null)
			{
				auto parentTransform = registry.try_get<Maths::Transform>(hierarchyComponent->parent());
				if (parentTransform)
				{
					transform->SetWorldMatrix(parentTransform->GetWorldMatrix());
				}
			}

			entt::entity child = hierarchyComponent->first();
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<Hierarchy>(child);
				auto next = hierarchyComponent ? hierarchyComponent->next() : entt::null;
				UpdateTransform(child, registry);
				child = next;
			}
		}
	}

	void Hierarchy::Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy)
	{
		Hierarchy::on_destroy(entity, registry);

		hierarchy._parent = parent;
		Hierarchy::on_construct(entity, registry, hierarchy);
	}

	bool Hierarchy::compare(const entt::registry& registry, const entt::entity rhs) const
	{
		if (rhs == entt::null || rhs == this->_parent || rhs == this->_prev)
		{
			return true;
		}
		else {
			if (this->_parent == entt::null)
			{
				return false;
			}
			else
			{
				auto& this_parent_h = registry.get<Hierarchy>(this->_parent);
				auto& rhs_h = registry.get<Hierarchy>(rhs);
				if (this_parent_h.compare(registry, rhs_h._parent)) 
				{
					return true;
				}
			}
		}
		return false;
	}

	void Hierarchy::on_construct(entt::entity entity, entt::registry& registry, Hierarchy& hierarchy)
	{
		if (hierarchy._parent != entt::null)
		{
			auto& parent_hierarchy = registry.get_or_emplace<Hierarchy>(hierarchy._parent);

			if (parent_hierarchy._first == entt::null) 
			{
				parent_hierarchy._first = entity;
			}
			else
			{
				// get last children
				auto prev_ent = parent_hierarchy._first;
				auto current_hierarchy = registry.try_get<Hierarchy>(prev_ent);
				while (current_hierarchy != nullptr && current_hierarchy->_next != entt::null)
				{
					prev_ent = current_hierarchy->_next;
					current_hierarchy = registry.try_get<Hierarchy>(prev_ent);
				}
				// add new
				current_hierarchy->_next = entity;
				hierarchy._prev = prev_ent;
			}
			//// sort
		/*	registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs) {
				auto& right_h = registry.get<Hierarchy>(rhs);
				auto result = right_h.compare(registry, lhs);
				return result;
			});*/
		}

	}

	void DeleteChildren(entt::entity parent, entt::registry& registry)
	{
		auto hierarchy = registry.try_get<Hierarchy>(parent);

		if (hierarchy)
		{
			entt::entity child = hierarchy->first();
			while (child != entt::null)
			{
				DeleteChildren(child, registry);
				hierarchy = registry.try_get<Hierarchy>(child);
				registry.destroy(child);
				
				if (hierarchy)
				{
					child = hierarchy->next();
				}

			}
		}
	}

	void Hierarchy::on_update(entt::entity entity, entt::registry& registry)
	{
		auto& hierarchy = registry.get<Hierarchy>(entity);
		// if is the first child
		if (hierarchy._prev == entt::null)
		{
			if (hierarchy._parent != entt::null)
			{
				auto parent_hierarchy = registry.try_get<Hierarchy>(hierarchy._parent);
				if (parent_hierarchy != nullptr)
				{
					parent_hierarchy->_first = hierarchy._next;
					if (hierarchy._next != entt::null)
					{
						auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy._next);
						if (next_hierarchy != nullptr)
						{
							next_hierarchy->_prev = entt::null;
						}
					}
					
				}
			}
		}
		else
		{
			auto prev_hierarchy = registry.try_get<Hierarchy>(hierarchy._prev);
			if (prev_hierarchy != nullptr)
			{
				prev_hierarchy->_next = hierarchy._next;
			}
			if (hierarchy._next != entt::null)
			{
				auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy._next);
				if (next_hierarchy != nullptr)
				{
					next_hierarchy->_prev = hierarchy._prev;
				}
			}
		}

		// sort
	/*	registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs)
		{
			auto& right_h = registry.get<Hierarchy>(rhs);
			return right_h.compare(registry, lhs);
		});*/
	}

	void Hierarchy::on_destroy(entt::entity entity, entt::registry& registry) 
	{
		auto& hierarchy = registry.get<Hierarchy>(entity);
		// if is the first child
		if (hierarchy._prev == entt::null || !registry.valid(hierarchy._prev))
		{
			if (hierarchy._parent != entt::null && registry.valid(hierarchy._parent))
			{
				auto parent_hierarchy = registry.try_get<Hierarchy>(hierarchy._parent);
				if (parent_hierarchy != nullptr)
				{
					parent_hierarchy->_first = hierarchy._next;
					if (hierarchy._next != entt::null)
					{
						auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy._next);
						if (next_hierarchy != nullptr)
						{
							next_hierarchy->_prev = entt::null;
						}
					}

				}
			}
		}
		else 
		{
			auto prev_hierarchy = registry.try_get<Hierarchy>(hierarchy._prev);
			if (prev_hierarchy != nullptr)
			{
				prev_hierarchy->_next = hierarchy._next;
			}
			if (hierarchy._next != entt::null) 
			{
				auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy._next);
				if (next_hierarchy != nullptr) 
				{
					next_hierarchy->_prev = hierarchy._prev;
				}
			}
		}

		// sort
		//registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs) 
		//{
		//	auto& right_h = registry.get<Hierarchy>(rhs);
		//	return right_h.compare(registry, lhs);
		//});
	}
}
