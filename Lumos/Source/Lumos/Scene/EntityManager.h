#pragma once

#include "Entity.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entity/registry.hpp>
DISABLE_WARNING_POP

namespace Lumos
{

    class Scene;
    class Entity;

    template <typename... Component>
    struct EntityView
    {
        class iterator;
        using TView = entt::view<entt::get_t<Component...>>;

    public:
        EntityView(Scene* scene);

        Entity operator[](int i)
        {
            ASSERT(i < Size(), "Index out of range on Entity View");
            return Entity(m_View[i], m_Scene);
        }

        bool Empty() const { return m_View.empty(); }
        size_t Size() const { return m_View.size(); }
        Entity Front() { return Entity(m_View[0], m_Scene); }

        iterator begin();
        iterator end();

        class iterator
        {
        public:
            using iterator_category = std::output_iterator_tag;
            using value_type        = Entity;
            using difference_type   = std::ptrdiff_t;
            using pointer           = Entity*;
            using reference         = Entity&;

            explicit iterator(EntityView<Component...>& view, size_t index = 0)
                : view(view)
                , nIndex(index)
            {
            }

            Entity operator*() const
            {
                return view[int(nIndex)];
            }

            iterator& operator++()
            {
                nIndex++;
                return *this;
            }

            iterator operator++(int)
            {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator!=(const iterator& rhs) const
            {
                return nIndex != rhs.nIndex;
            }

        private:
            size_t nIndex = 0;
            EntityView<Component...>& view;
        };

        Scene* m_Scene;
        TView m_View;
    };

    template <typename... Component>
    EntityView<Component...>::EntityView(Scene* scene)
        : m_Scene(scene)
        , m_View(scene->GetRegistry().view<Component...>())
    {
    }

    template <typename... Component>
    typename EntityView<Component...>::iterator EntityView<Component...>::begin()
    {
        return EntityView<Component...>::iterator(*this, 0);
    }

    template <typename... Component>
    typename EntityView<Component...>::iterator EntityView<Component...>::end()
    {
        return EntityView<Component...>::iterator(*this, Size());
    }

    //    template <typename... Components>
    //    class EntityGroup
    //    {
    //    public:
    //        EntityGroup(Scene* scene)
    //            : m_Scene(scene)
    //        {
    //            // Expand the component types into a tuple
    //            std::tuple enttGroupTypes = std::make_tuple(entt::type_id<Components>()...);
    //
    //            // Create the entt::group using std::apply to expand the tuple
    //            m_Group = scene->GetRegistry().group<Components...>(std::apply([](auto&&... args) { return entt::get<std::decay_t<decltype(args)>...>; }, enttGroupTypes));
    //        }
    //
    //        Entity operator[](int i)
    //        {
    //            ASSERT(i < Size(), "Index out of range on Entity View");
    //            return Entity(m_Group[i], m_Scene);
    //        }
    //
    //        size_t Size() const
    //        {
    //            return m_Group.size();
    //        }
    //        Entity Front()
    //        {
    //            return Entity(m_Group[0], m_Scene);
    //        }
    //
    //    private:
    //        Scene* m_Scene;
    //        entt::group<Components...>& m_Group;
    //    };

    template <typename...>
    struct TypeList
    {
    };

    class EntityManager
    {
    public:
        EntityManager(Scene* scene)
            : m_Scene(scene)
        {
            m_Registry = {};
        }

        Entity Create();
        Entity Create(const std::string& name);

        template <typename... Components>
        auto GetEntitiesWithTypes()
        {
            return m_Registry.group<Components...>();
        }

        template <typename... Component>
        EntityView<Component...> GetEntitiesWithType()
        {
            return EntityView<Component...>(m_Scene);
        }

        template <typename R, typename T>
        void AddDependency()
        {
            m_Registry.template on_construct<R>().template connect<&entt::registry::get_or_emplace<T>>();
        }

        entt::registry& GetRegistry()
        {
            return m_Registry;
        }

        void Clear();

        Entity GetEntityByUUID(uint64_t id);
        bool EntityExists(u64 id);

    private:
        Scene* m_Scene = nullptr;
        entt::registry m_Registry;
    };
}
