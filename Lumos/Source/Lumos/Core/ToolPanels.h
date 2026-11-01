#pragma once
#include "Core/Core.h"
#include "Scene/Entity.h"
#include "Graphics/UIDock.h"
#include <type_traits>

namespace Lumos
{
    namespace Graphics
    {
        class Texture2D;
    }

    // Scene tools built on the engine UI, shared by the editor and the in-game
    // debug workspace: entity tree, inspector, log, frame stats.
    namespace ToolPanels
    {
        // ---- selection ----
        // Defaults to the debug overlay's single selection. An editor installs
        // hooks to keep its own (multi) selection as the source of truth.
        struct SelectionHooks
        {
            u32 (*Get)()                          = nullptr; // primary entity, 0xFFFFFFFF = none
            void (*Set)(u32 entity, bool additive) = nullptr;
            bool (*IsSelected)(u32 entity)         = nullptr;
            void (*Focus)(u32 entity)              = nullptr; // double-click in the tree
        };
        void SetSelectionHooks(const SelectionHooks& hooks);
        u32 GetSelected();
        void Select(u32 entity, bool additive = false);
        bool IsSelected(u32 entity);

        // ---- component inspectors ----
        struct ComponentDesc
        {
            const char* Name = nullptr;
            u32 TypeId       = 0;
            u32 Size         = 0;
            // The bytes can be snapshotted for undo. Anything else is recorded by
            // saving the whole entity before and after the edit.
            bool PlainData = false;
            bool CanRemove = true;

            bool (*Has)(Entity)     = nullptr;
            void* (*Get)(Entity)    = nullptr;
            void (*Add)(Entity)     = nullptr; // null = not offered under Add Component
            void (*Remove)(Entity)  = nullptr;
            // Rows for a property grid that is already open. Null = header only.
            void (*Draw)(Entity entity, void* component) = nullptr;
        };
        void RegisterComponent(const ComponentDesc& desc);

        template <typename T>
        void RegisterComponent(const char* name, void (*draw)(Entity, void*), bool plainData = false, bool canAdd = true, bool canRemove = true)
        {
            ComponentDesc desc;
            desc.Name      = name;
            desc.TypeId    = (u32)entt::type_hash<T>::value();
            desc.Size      = (u32)sizeof(T);
            desc.PlainData = plainData;
            desc.CanRemove = canRemove;
            desc.Has       = [](Entity e) { return e.HasComponent<T>(); };
            desc.Get       = [](Entity e) -> void* { return e.TryGetComponent<T>(); };
            desc.Remove    = [](Entity e) { e.TryRemoveComponent<T>(); };
            if(canAdd && std::is_default_constructible<T>::value)
            {
                desc.Add = [](Entity e)
                {
                    if constexpr(std::is_default_constructible<T>::value)
                        e.GetOrAddComponent<T>();
                };
            }
            desc.Draw = draw;
            RegisterComponent(desc);
        }

        // Built-in component inspectors. Safe to call more than once.
        void Init();

        // ---- panels (dock callbacks) ----
        void DrawHierarchy(const UIDockPanelContext& context);
        void DrawInspector(const UIDockPanelContext& context);
        void DrawConsole(const UIDockPanelContext& context);
        void DrawStats(const UIDockPanelContext& context);
        // Tool theme: presets, the colours and the shape values, all live.
        void DrawTheme(const UIDockPanelContext& context);
        // Project files as a grid of tiles; folders open, files go to the open hook.
        void DrawAssets(const UIDockPanelContext& context);
        // Render, post-process and physics settings of the current scene.
        void DrawSceneSettings(const UIDockPanelContext& context);
        // Window, identity and startup settings of the project.
        void DrawProjectSettings(const UIDockPanelContext& context);

        // ---- assets ----
        struct AssetHooks
        {
            void (*Open)(const char* physicalPath) = nullptr; // double-click / Enter
            // A rendered preview for a model or material (VFS path): true with the picture's file in outPng when
            // there is one; otherwise asks for it to be made and returns false. Called every frame the tile shows.
            bool (*Preview)(const char* vfsPath, char* outPng, u32 size) = nullptr;
            // Model files (VFS path): bring it in as a tree of entities / run its import again. Menu rows show when set.
            void (*ImportAsScene)(const char* vfsPath) = nullptr;
            void (*Reimport)(const char* vfsPath)      = nullptr;
        };
        void SetAssetHooks(const AssetHooks& hooks);
        // Hands a file (VFS or physical path) to whatever opens files here: the script editor for text, and so on.
        // False when nothing is installed to open it.
        bool OpenAsset(const char* path);
        // Thumbnail + name + size row for a texture slot; a picture dropped on it from the Assets panel
        // comes back in outPath (VFS form). Hovering the thumbnail shows it large.
        bool TextureSlotRow(const char* label, Graphics::Texture2D* texture, char* outPath, u32 outSize);
        // A file dropped from the Assets panel on the widget just built, when its name ends in `extension`
        // (".lua"; null = any). VFS form.
        bool AssetDropTarget(const char* extension, char* outPath, u32 outSize);
        // A small button that opens a searchable list of the project's files of the given kinds
        // (extensions separated by ';', e.g. ".png;.jpg"). True on the frame one is picked, with
        // its VFS path in outPath. Put it in the row of the slot it fills.
        bool AssetPickButton(const char* id, const char* extensions, char* outPath, u32 outSize);
        // Folder the Assets panel shows, relative to the Assets root ("" = the root).
        void SetAssetFolder(const char* relativePath);
        // Size of the Assets tiles (0.6 .. 3), for whoever keeps settings between runs.
        float GetAssetTileScale();
        void SetAssetTileScale(float scale);
        // Drops the image previews. Call before the renderer goes.
        void ReleaseThumbnails();
        // Every file under the project's Assets folder, relative to it. Built on
        // first use; RefreshAssetIndex rebuilds it.
        u32 GetAssetIndexCount();
        const char* GetAssetIndexPath(u32 index);
        void RefreshAssetIndex();
        // Icon glyph for a file name, by extension.
        const char* AssetIcon(const char* name, bool folder);
        // Payload type for UIDragSource / UIDropTarget: the file's physical path (char[256]).
        constexpr const char* kAssetDragType = "lumos_asset";

        // Adds them all with their usual places: tree left, inspector right, console along
        // the bottom (as a drawer when asked); stats and theme start closed.
        void RegisterPanels(UIDockSpace* space, bool consoleAsDrawer);
    }

    // The tools docked around a running game (centre left clear for it).
    namespace DebugWorkspace
    {
        void SetVisible(bool visible);
        bool IsVisible();
        void Toggle();
        void OnUI();
        void Shutdown();
        UIDockSpace* GetDockSpace();
    }
}
