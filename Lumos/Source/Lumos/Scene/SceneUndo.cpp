#include "Precompiled.h"
#include "SceneUndo.h"

#include "Core/Application.h"
#include "Core/OS/Memory.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include "Scene/EntityManager.h"

namespace Lumos
{
    namespace SceneUndo
    {
        namespace
        {
            // Bigger than this and one delete would eat the undo buffer.
            constexpr u64 kMaxEntityBytes = Megabytes(4);

            enum LifetimeKind : u32
            {
                Lifetime_Created = 0,
                Lifetime_Destroyed,
            };

            struct LifetimeHeader
            {
                u64 uuid;
                u64 parent;
                u32 kind;
                u32 dataSize;
            };

            struct NameHeader
            {
                u64 uuid;
                u32 oldSize;
                u32 newSize;
            };

            struct ParentPayload
            {
                u64 uuid;
                u64 oldParent;
                u64 newParent;
            };

            struct EditHeader
            {
                u64 uuid;
                u64 parent;
                u32 beforeSize;
                u32 afterSize;
            };

            // BeginEntityEdit keeps the "before" here until EndEntityEdit.
            EntityReplacedFn s_Replaced = nullptr;
            Arena* s_EditArena    = nullptr;
            String8 s_EditBefore  = {};
            u64 s_EditUUID        = 0;

            Scene* CurrentScene()
            {
                return Application::Get().GetSceneManager() ? Application::Get().GetCurrentScene() : nullptr;
            }

            Entity FindEntity(Scene* scene, u64 uuid)
            {
                if(!scene || uuid == 0)
                    return Entity();
                auto& registry = scene->GetRegistry();
                auto view      = registry.view<IDComponent>();
                for(auto e : view)
                {
                    if((u64)view.get<IDComponent>(e).ID == uuid)
                        return Entity(e, scene);
                }
                return Entity();
            }

            u64 ParentUUID(Entity entity)
            {
                Entity parent = entity.GetParent();
                return parent.Valid() ? parent.GetID() : 0;
            }

            u32 Recreate(Scene* scene, String8 data, u64 parentUUID)
            {
                Entity entity = scene->DeserialiseEntityFromString(data);
                if(!entity.Valid())
                    return 0xFFFFFFFFu;
                Entity parent = FindEntity(scene, parentUUID);
                if(parent.Valid())
                    entity.SetParent(parent);
                return (u32)entt::to_integral(entity.GetHandle());
            }

            u32 Remove(Scene* scene, u64 uuid)
            {
                Entity entity = FindEntity(scene, uuid);
                if(!entity.Valid())
                    return 0xFFFFFFFFu;
                const u32 handle = (u32)entt::to_integral(entity.GetHandle());
                scene->DestroyEntity(entity);
                return handle;
            }

            void* Resolve(u64 owner, u64 typeKey)
            {
                Scene* scene  = CurrentScene();
                Entity entity = FindEntity(scene, owner);
                if(!entity.Valid())
                    return nullptr;
                auto* storage = scene->GetRegistry().storage((entt::id_type)typeKey);
                if(!storage || !storage->contains(entity.GetHandle()))
                    return nullptr;
                return storage->value(entity.GetHandle());
            }

            void LifetimeCommand(const void* payload, i64 size, bool undo)
            {
                Scene* scene = CurrentScene();
                if(!scene || size < (i64)sizeof(LifetimeHeader))
                    return;
                LifetimeHeader header;
                MemoryCopy(&header, payload, sizeof(header));
                const String8 data = Str8((u8*)payload + sizeof(header), header.dataSize);

                // undoing a create removes it; undoing a destroy brings it back
                const bool exists = (header.kind == Lifetime_Created) ? !undo : undo;
                u32 oldHandle     = 0xFFFFFFFFu;
                u32 newHandle     = 0xFFFFFFFFu;
                if(exists)
                    newHandle = Recreate(scene, data, header.parent);
                else
                    oldHandle = Remove(scene, header.uuid);
                if(s_Replaced && (oldHandle != 0xFFFFFFFFu || newHandle != 0xFFFFFFFFu))
                    s_Replaced(oldHandle, newHandle);
            }

            void NameCommand(const void* payload, i64 size, bool undo)
            {
                Scene* scene = CurrentScene();
                if(!scene || size < (i64)sizeof(NameHeader))
                    return;
                NameHeader header;
                MemoryCopy(&header, payload, sizeof(header));
                Entity entity = FindEntity(scene, header.uuid);
                if(!entity.Valid())
                    return;
                const char* oldName = (const char*)payload + sizeof(header);
                const char* newName = oldName + header.oldSize;
                entity.GetOrAddComponent<NameComponent>().name = undo ? std::string(oldName, header.oldSize)
                                                                     : std::string(newName, header.newSize);
            }

            void ParentCommand(const void* payload, i64 size, bool undo)
            {
                Scene* scene = CurrentScene();
                if(!scene || size < (i64)sizeof(ParentPayload))
                    return;
                ParentPayload p;
                MemoryCopy(&p, payload, sizeof(p));
                Entity entity = FindEntity(scene, p.uuid);
                if(!entity.Valid())
                    return;
                Entity parent = FindEntity(scene, undo ? p.oldParent : p.newParent);
                entity.SetParent(parent.Valid() ? parent : Entity(entt::null, scene));
            }

            void EditCommand(const void* payload, i64 size, bool undo)
            {
                Scene* scene = CurrentScene();
                if(!scene || size < (i64)sizeof(EditHeader))
                    return;
                EditHeader header;
                MemoryCopy(&header, payload, sizeof(header));
                const u8* bytes      = (const u8*)payload + sizeof(header);
                const String8 before = Str8((u8*)bytes, header.beforeSize);
                const String8 after  = Str8((u8*)bytes + header.beforeSize, header.afterSize);

                const u32 oldHandle = Remove(scene, header.uuid);
                const u32 newHandle = Recreate(scene, undo ? before : after, header.parent);
                if(s_Replaced)
                    s_Replaced(oldHandle, newHandle);
            }

            void PushLifetime(Entity entity, LifetimeKind kind)
            {
                Scene* scene = entity.GetScene();
                if(!scene || !entity.Valid())
                    return;

                ArenaTemp scratch = ScratchBegin(0, 0);
                String8 data      = scene->SerialiseEntityToString(scratch.arena, entity);
                if(data.size > 0 && data.size < kMaxEntityBytes)
                {
                    LifetimeHeader header;
                    header.uuid     = entity.GetID();
                    header.parent   = ParentUUID(entity);
                    header.kind     = kind;
                    header.dataSize = (u32)data.size;

                    const u64 total = sizeof(header) + data.size;
                    u8* payload     = PushArrayNoZero(scratch.arena, u8, total);
                    MemoryCopy(payload, &header, sizeof(header));
                    MemoryCopy(payload + sizeof(header), data.str, data.size);
                    UndoPushCommand(LifetimeCommand, payload, (i64)total);
                }
                else if(data.size >= kMaxEntityBytes)
                {
                    LWARN("Undo: entity too large to record (%llu bytes)", (unsigned long long)data.size);
                }
                ScratchEnd(scratch);
            }
        }

        void Init()
        {
            InitialiseUndo();
            SetUndoResolver(&Resolve);
            if(!s_EditArena)
                s_EditArena = ArenaAlloc(kMaxEntityBytes + Kilobytes(64));
        }

        void PushComponentRaw(Entity entity, void* component, i64 size, u64 typeKey)
        {
            if(!entity.Valid() || !component)
                return;
            UndoPushOwned(component, size, entity.GetID(), typeKey);
        }

        void EntityCreated(Entity entity)
        {
            UndoSetLabel("Create Entity");
            PushLifetime(entity, Lifetime_Created);
            UndoCommit();
        }

        void DestroyEntity(Entity entity)
        {
            if(!entity.Valid())
                return;
            UndoSetLabel("Delete Entity");
            PushLifetime(entity, Lifetime_Destroyed);
            entity.GetScene()->DestroyEntity(entity);
            UndoCommit();
        }

        void SetName(Entity entity, const char* name)
        {
            if(!entity.Valid() || !name)
                return;
            NameComponent& component = entity.GetOrAddComponent<NameComponent>();
            if(component.name == name)
                return;

            ArenaTemp scratch = ScratchBegin(0, 0);
            NameHeader header;
            header.uuid    = entity.GetID();
            header.oldSize = (u32)component.name.size();
            header.newSize = (u32)strlen(name);

            const u64 total = sizeof(header) + header.oldSize + header.newSize;
            u8* payload     = PushArrayNoZero(scratch.arena, u8, total);
            MemoryCopy(payload, &header, sizeof(header));
            MemoryCopy(payload + sizeof(header), component.name.data(), header.oldSize);
            MemoryCopy(payload + sizeof(header) + header.oldSize, name, header.newSize);

            UndoSetLabel("Rename");
            UndoPushCommand(NameCommand, payload, (i64)total);
            ScratchEnd(scratch);

            component.name = name;
            UndoCommit();
        }

        void SetParent(Entity entity, Entity parent)
        {
            if(!entity.Valid())
                return;

            ParentPayload p;
            p.uuid      = entity.GetID();
            p.oldParent = ParentUUID(entity);
            p.newParent = parent.Valid() ? parent.GetID() : 0;
            if(p.oldParent == p.newParent)
                return;

            entity.SetParent(parent.Valid() ? parent : Entity(entt::null, entity.GetScene()));
            // refused (would make a cycle): nothing to record
            if(ParentUUID(entity) != p.newParent)
                return;

            UndoSetLabel("Reparent");
            UndoPushCommand(ParentCommand, &p, (i64)sizeof(p));
            UndoCommit();
        }

        void BeginEntityEdit(Entity entity)
        {
            s_EditBefore = {};
            s_EditUUID   = 0;
            if(!entity.Valid() || !s_EditArena)
                return;

            ArenaClear(s_EditArena);
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 data      = entity.GetScene()->SerialiseEntityToString(scratch.arena, entity);
            if(data.size > 0 && data.size < kMaxEntityBytes / 2)
            {
                s_EditBefore = PushStr8Copy(s_EditArena, data);
                s_EditUUID   = entity.GetID();
            }
            ScratchEnd(scratch);
        }

        void BeginEntityEditByID(u64 uuid)
        {
            BeginEntityEdit(FindEntity(CurrentScene(), uuid));
        }

        void EndEntityEditByID(u64 uuid)
        {
            EndEntityEdit(FindEntity(CurrentScene(), uuid), nullptr);
        }

        void SetEntityReplacedCallback(EntityReplacedFn fn)
        {
            s_Replaced = fn;
        }

        void EndEntityEdit(Entity entity, const char* label)
        {
            if(s_EditUUID == 0 || !entity.Valid() || entity.GetID() != s_EditUUID)
            {
                s_EditUUID = 0;
                return;
            }

            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 after     = entity.GetScene()->SerialiseEntityToString(scratch.arena, entity);
            if(after.size > 0 && after.size < kMaxEntityBytes / 2 && !(after == s_EditBefore))
            {
                EditHeader header;
                header.uuid       = s_EditUUID;
                header.parent     = ParentUUID(entity);
                header.beforeSize = (u32)s_EditBefore.size;
                header.afterSize  = (u32)after.size;

                const u64 total = sizeof(header) + s_EditBefore.size + after.size;
                u8* payload     = PushArrayNoZero(scratch.arena, u8, total);
                MemoryCopy(payload, &header, sizeof(header));
                MemoryCopy(payload + sizeof(header), s_EditBefore.str, s_EditBefore.size);
                MemoryCopy(payload + sizeof(header) + s_EditBefore.size, after.str, after.size);

                UndoSetLabel(label ? label : "Edit Entity");
                UndoPushCommand(EditCommand, payload, (i64)total);
                UndoCommit();
            }
            ScratchEnd(scratch);
            s_EditUUID = 0;
        }
    }
}
