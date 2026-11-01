#pragma once
#include "Core/Undo.h"
#include "Scene/Entity.h"

namespace Lumos
{
    // Undo for scene edits. Entities are tracked by UUID, so a step still finds
    // its target after the entity has been deleted and brought back.
    namespace SceneUndo
    {
        // Installs the resolver. Safe to call more than once.
        void Init();

        // Mark a component as about to change. Plain-data components only
        // (Transform, Light, Camera...) - anything owning memory needs an entity edit.
        void PushComponentRaw(Entity entity, void* component, i64 size, u64 typeKey);

        template <typename T>
        void PushComponent(Entity entity)
        {
            if(T* component = entity.TryGetComponent<T>())
                PushComponentRaw(entity, component, (i64)sizeof(T), (u64)entt::type_hash<T>::value());
        }

        // Call right after creating an entity.
        void EntityCreated(Entity entity);
        // Records the entity (and its children), then destroys it.
        void DestroyEntity(Entity entity);
        void SetName(Entity entity, const char* name);
        void SetParent(Entity entity, Entity parent);

        // Wrap anything else (add / remove component, edits to non-plain data):
        // the entity is saved before and after, and undo swaps between the two.
        void BeginEntityEdit(Entity entity);
        void EndEntityEdit(Entity entity, const char* label = nullptr);
        // The same, by UUID - the shape UIPushUndoCallbacks wants.
        void BeginEntityEditByID(u64 uuid);
        void EndEntityEditByID(u64 uuid);

        // Undoing a structural edit rebuilds the entity, which gives it a new handle.
        // Lets whoever holds a selection follow it (0xFFFFFFFF = was / is not there).
        using EntityReplacedFn = void (*)(u32 oldHandle, u32 newHandle);
        void SetEntityReplacedCallback(EntityReplacedFn fn);
    }
}
