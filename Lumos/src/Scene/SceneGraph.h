#pragma once

#include <entt/entity/fwd.hpp>
#include <cereal/cereal.hpp>

namespace Lumos
{
	struct NameComponent
	{
		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::make_nvp("Name", name));
		}
		std::string name;
	};

	struct ActiveComponent
	{
		ActiveComponent()
		{
			active = true;
		}

		ActiveComponent(bool act)
		{
			active = act;
		}

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::make_nvp("Active", active));
		}

		bool active = true;
	};

	class Hierarchy
	{
	public:
		Hierarchy(entt::entity p);
		Hierarchy();

		inline entt::entity parent() const
		{
			return _parent;
		}
		inline entt::entity next() const
		{
			return _next;
		}
		inline entt::entity prev() const
		{
			return _prev;
		}
		inline entt::entity first() const
		{
			return _first;
		}

		// Return true if rhs is an ancestor of rhs
		bool compare(const entt::registry& registry, const entt::entity rhs) const;

		// update hierarchy components when hierarchy component is added
		static void on_construct(entt::registry& registry, entt::entity entity);

		// update hierarchy components when hierarchy component is removed
		static void on_destroy(entt::registry& registry, entt::entity entity);

		static void on_update(entt::registry& registry, entt::entity entity);

		static void Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy);

		entt::entity _parent;
		entt::entity _first;
		entt::entity _next;
		entt::entity _prev;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::make_nvp("First", _first), cereal::make_nvp("Next", _next), cereal::make_nvp("Previous", _prev), cereal::make_nvp("Parent", _parent));
		}
	};

	class SceneGraph
	{
	public:
		SceneGraph();
		~SceneGraph() = default;

		void Init(entt::registry& registry);

		void Update(entt::registry& registry);
		void UpdateTransform(entt::entity entity, entt::registry& registry);
	};
}
