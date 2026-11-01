#include "Precompiled.h"
#include "ToolPanels.h"

#include "Core/Application.h"
#include "Core/DebugMenu.h"
#include "Core/Engine.h"
#include "Core/LMLog.h"
#include "Core/Undo.h"
#include "Core/OS/Input.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/Renderer.h"
#include "Core/OS/FileDialogs.h"
#include "Core/OS/OS.h"
#include "Graphics/UI.h"
#include "Graphics/UIInternal.h"
#include "Graphics/UITheme.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/Light.h"
#include "Graphics/Light2D.h"
#include "Graphics/Material.h"
#include "Graphics/ParticleManager.h"
#include "Audio/Sound.h"
#include "Core/SpriteEditor.h"
#include "Audio/AudioManager.h"
#include "Graphics/Environment.h"
#include "Graphics/RHI/Texture.h"
#include "Scene/EntityFactory.h"
#include "Scene/EntityManager.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Scene/Component/DecalComponent.h"
#include "Scene/Component/TerrainComponent.h"
#include "Scene/Component/VoxelWorldComponent.h"
#include "Audio/SoundNode.h"
#include "Scene/Component/SoundComponent.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/MeshCollisionShape.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Transform.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneGraph.h"
#include "Scene/SceneUndo.h"
#include "Scene/Component/Components.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "ImGui/IconsMaterialDesignIcons.h"

#include <filesystem>

namespace Lumos
{
    void RegisterLuaScriptInspector(); // LuaScriptComponent.cpp

    namespace ToolPanels
    {
        namespace
        {
            constexpr u32 kNoEntity = 0xFFFFFFFFu;
            // colours come from the tool theme, so the panels follow whatever is picked there
            inline Vec4 DimColour() { return UIGetToolTheme().TextDim; }
            inline Vec4 TextColour() { return UIGetToolTheme().Text; }

            SelectionHooks s_Hooks;

            constexpr u32 kMaxComponents = 64;
            ComponentDesc s_Components[kMaxComponents];
            u32 s_ComponentCount = 0;
            bool s_Initialised   = false;

            Scene* CurrentScene()
            {
                return Application::Get().GetSceneManager() ? Application::Get().GetCurrentScene() : nullptr;
            }

            bool ControlHeld()
            {
                Input& input = Input::Get();
                return input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
                    || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
            }

            char Lower(char c) { return (c >= 'A' && c <= 'Z') ? char(c + 32) : c; }

            bool ContainsNoCase(const char* text, const char* pattern)
            {
                if(!pattern || !pattern[0])
                    return true;
                for(const char* t = text; *t; t++)
                {
                    const char* a = t;
                    const char* b = pattern;
                    while(*a && *b && Lower(*a) == Lower(*b))
                    {
                        a++;
                        b++;
                    }
                    if(!*b)
                        return true;
                }
                return false;
            }

            void EntityName(entt::registry& registry, entt::entity e, char* out, size_t size)
            {
                const NameComponent* name = registry.try_get<NameComponent>(e);
                if(name && !name->name.empty())
                    snprintf(out, size, "%s", name->name.c_str());
                else
                    snprintf(out, size, "Entity %u", (u32)entt::to_integral(e));
            }

            // A local-transform write made after the scene graph pass has to
            // rebuild the world matrix itself, or it never reaches the renderer.
            void RefreshWorldMatrix(entt::registry& registry, entt::entity e, Maths::Transform* transform)
            {
                if(Hierarchy* hierarchy = registry.try_get<Hierarchy>(e))
                {
                    const entt::entity parent = hierarchy->Parent();
                    if(parent != entt::null && registry.valid(parent))
                    {
                        if(Maths::Transform* parentTransform = registry.try_get<Maths::Transform>(parent))
                        {
                            transform->SetWorldMatrix(parentTransform->GetWorldMatrix());
                            return;
                        }
                    }
                }
                transform->SetWorldMatrix();
            }

            void DimLabel(const char* id, const char* text)
            {
                UIPushStyle(StyleVar_TextColor, DimColour());
                UILabelCStr(id, text);
                UIPopStyle(StyleVar_TextColor);
            }

            // ---- hierarchy state ----
            // One bit per entity index: is its row unfolded.
            u64 s_Expanded[1024]          = { 0 };
            char s_HierarchyFilter[64]    = { 0 };
            f32 s_HierarchyScroll         = 0.0f;
            u32 s_ContextEntity           = kNoEntity;

            // the row whose name is being typed over
            u32 s_RenameEntity     = kNoEntity;
            char s_RenameBuffer[128] = { 0 };
            bool s_RenameHadFocus  = false;
            bool s_RenameClaimed   = false;

            bool IsExpanded(entt::entity e)
            {
                const u32 index = (u32)entt::to_entity(e);
                return index < 65536 && (s_Expanded[index >> 6] & (1ull << (index & 63)));
            }

            void SetExpanded(entt::entity e, bool expanded)
            {
                const u32 index = (u32)entt::to_entity(e);
                if(index >= 65536)
                    return;
                if(expanded)
                    s_Expanded[index >> 6] |= (1ull << (index & 63));
                else
                    s_Expanded[index >> 6] &= ~(1ull << (index & 63));
            }

            struct HierarchyFrame
            {
                Scene* scene;
                entt::registry* registry;
                i32 index; // rows passed so far
                i32 first; // rows built are [first, last)
                i32 last;
                f32 dpi;
                f32 rowLogical; // row height before DPI
                // asked for by a row; applied once the tree has been walked
                u32 reparentChild;
                u32 reparentTo;
                u32 renameTo; // row whose typed name was accepted
            };

            // fresh ids for an entity and everything under it
            void RenewIds(entt::registry& registry, entt::entity e)
            {
                if(IDComponent* idComponent = registry.try_get<IDComponent>(e))
                    idComponent->ID = UUID();
                if(const Hierarchy* hierarchy = registry.try_get<Hierarchy>(e))
                    for(entt::entity child = hierarchy->First(); child != entt::null; child = registry.get<Hierarchy>(child).Next())
                        RenewIds(registry, child);
            }

            void HierarchyRow(HierarchyFrame& frame, entt::entity e, i32 depth, bool hasChildren)
            {
                entt::registry& registry = *frame.registry;
                const u32 id             = (u32)entt::to_integral(e);
                const f32 dpi            = frame.dpi;

                char name[128];
                EntityName(registry, e, name, sizeof(name));

                char rowId[32];
                snprintf(rowId, sizeof(rowId), "###ent%u", id);
                UI_Interaction row = UIBeginSelectableRow(rowId, IsSelected(id), frame.rowLogical);
                {
                    UI_Widget* rowWidget                       = GetUIState()->parents.Back();
                    rowWidget->style_vars[StyleVar_Padding]     = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                    rowWidget->style_vars[StyleVar_ItemSpacing] = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);

                    if(depth > 0)
                        PushWidget(0, Str8Lit(""), 0x494E444Eull, { SizeKind_Pixels, (f32)depth * 14.0f * dpi }, { SizeKind_Pixels, 1.0f });

                    if(hasChildren)
                    {
                        const bool expanded = IsExpanded(e);
                        UIPushStyle(StyleVar_TextColor, DimColour());
                        if(UIIconButton(expanded ? ICON_MDI_CHEVRON_DOWN "###fold" : ICON_MDI_CHEVRON_RIGHT "###fold", 18.0f).clicked)
                            SetExpanded(e, !expanded);
                        UIPopStyle(StyleVar_TextColor);
                    }
                    else
                    {
                        PushWidget(0, Str8Lit(""), 0x4E4F4152ull, { SizeKind_Pixels, 18.0f * dpi }, { SizeKind_Pixels, 1.0f });
                    }

                    const ActiveComponent* active = registry.try_get<ActiveComponent>(e);
                    const bool hidden             = active && !active->active;
                    const Vec4 faded              = Vec4(DimColour().x, DimColour().y, DimColour().z, 0.55f);

                    // what kind of thing it is, at a glance
                    const char* icon = ICON_MDI_VECTOR_POINT;
                    if(registry.all_of<Camera>(e))
                        icon = ICON_MDI_CAMERA;
                    else if(registry.all_of<Graphics::Light>(e))
                        icon = ICON_MDI_LIGHTBULB;
                    else if(registry.all_of<Graphics::ModelComponent>(e))
                        icon = ICON_MDI_CUBE_OUTLINE;
                    else if(registry.all_of<Graphics::Sprite>(e) || registry.all_of<Graphics::AnimatedSprite>(e))
                        icon = ICON_MDI_IMAGE;
                    else if(registry.all_of<TextComponent>(e))
                        icon = ICON_MDI_PENCIL;
                    else if(hasChildren)
                        icon = ICON_MDI_FOLDER;

                    UIPushStyle(StyleVar_TextColor, hidden ? faded : DimColour());
                    UILabel("###icon", Str8C((char*)icon));
                    UIPopStyle(StyleVar_TextColor);

                    if(s_RenameEntity == id)
                    {
                        UI_State* ui = GetUIState();
                        if(!s_RenameClaimed)
                        {
                            s_RenameClaimed = true;
                            UIFocusNextTextInput();
                        }
                        UI_Interaction field = UITextField("###rename", s_RenameBuffer, sizeof(s_RenameBuffer), "", 0.0f);
                        if(field.widget)
                            field.widget->style_vars[StyleVar_Padding] = Vec4(4.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);

                        // Enter or a press elsewhere drops the focus and keeps the name; Escape throws it away
                        const bool cancel = Input::Get().GetKeyPressedRaw(InputCode::Key::Escape);
                        if(ui->FocusedTextInput != 0)
                            s_RenameHadFocus = true;
                        const bool accept = s_RenameHadFocus && (Input::Get().GetKeyPressedRaw(InputCode::Key::Enter));
                        if(cancel || accept || (s_RenameHadFocus && ui->FocusedTextInput == 0))
                        {
                            if(!cancel && s_RenameBuffer[0] && strcmp(s_RenameBuffer, name) != 0)
                                frame.renameTo = id;
                            if(cancel || accept)
                            {
                                ui->FocusedTextInput    = 0;
                                ui->TextInputBuffer     = nullptr;
                                ui->TextInputBufferSize = 0;
                            }
                            s_RenameEntity = kNoEntity;
                        }
                    }
                    else
                    {
                        UIPushStyle(StyleVar_TextColor, hidden ? faded : TextColour());
                        UILabel("###name", Str8C(name));
                        UIPopStyle(StyleVar_TextColor);
                    }

                    // shown on the row under the pointer, and on anything that is switched off
                    if(hidden || row.hovering)
                    {
                        UIFlexSpacer();
                        UIPushStyle(StyleVar_TextColor, hidden ? faded : DimColour());
                        if(UIIconButton(hidden ? ICON_MDI_EYE_OFF "###eye" : ICON_MDI_EYE "###eye", 18.0f).clicked)
                            Entity(e, frame.scene).SetActive(hidden);
                        UIPopStyle(StyleVar_TextColor);
                    }
                }
                UIEndSelectableRow();

                if(row.clicked)
                    Select(id, ControlHeld());
                if(row.double_clicked && s_Hooks.Focus)
                    s_Hooks.Focus(id);
                if(row.right_clicked)
                {
                    s_ContextEntity = id;
                    Select(id, false);
                    UIOpenPopupAt("##hierarchycontext", UIMousePosition());
                }

                // drag onto another row to parent it there
                UIDragSource("lumos_entity", &id, sizeof(id), Str8C(name));
                u32 dropped = kNoEntity;
                if(UIDropTarget("lumos_entity", &dropped, sizeof(dropped)) && dropped != id)
                {
                    frame.reparentChild = dropped;
                    frame.reparentTo    = id;
                }
            }

            void HierarchyWalk(HierarchyFrame& frame, entt::entity e, i32 depth)
            {
                entt::registry& registry = *frame.registry;
                Hierarchy* hierarchy     = registry.try_get<Hierarchy>(e);
                const bool hasChildren   = hierarchy && hierarchy->First() != entt::null && registry.valid(hierarchy->First());

                if(frame.index >= frame.first && frame.index < frame.last)
                    HierarchyRow(frame, e, depth, hasChildren);
                frame.index++;

                if(!hasChildren || !IsExpanded(e) || depth > 64)
                    return;

                entt::entity child = hierarchy->First();
                i32 guard          = 0;
                while(child != entt::null && registry.valid(child) && guard++ < 100000)
                {
                    Hierarchy* childHierarchy = registry.try_get<Hierarchy>(child);
                    const entt::entity next   = childHierarchy ? childHierarchy->Next() : entt::null;
                    HierarchyWalk(frame, child, depth + 1);
                    child = next;
                }
            }

            UI_Widget* ListSpacer(u64 hash, f32 height)
            {
                UI_Widget* spacer = PushWidget(0, Str8Lit(""), hash, { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, Maths::Max(height, 0.0f) });
                spacer->style_vars[StyleVar_Padding] = Vec4(0.0f);
                return spacer;
            }

            // Transparent, unpadded scroll area for a list the caller virtualises.
            void BeginListArea(const char* id, f32 height, f32* scroll)
            {
                UIBeginScrollArea(id, height, scroll);
                UI_Widget* area                            = GetUIState()->parents.Back();
                area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                area->style_vars[StyleVar_Border]          = Vec4(0.0f);
                area->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                area->style_vars[StyleVar_ItemSpacing]     = Vec4(0.0f);
            }

            // ---- inspector state ----
            char s_NameEdit[128]     = { 0 };
            char s_NameOriginal[128] = { 0 };
            u32 s_NameEditFor        = kNoEntity;
            bool s_NameWasFocused    = false;
            u32 s_RenameRequest      = kNoEntity; // asked for by the menu / F2, started once the rows are built

            // an entity copied from the tree, as prefab text
            Arena* s_ClipboardArena    = nullptr;
            String8 s_EntityClipboard = {};

            // ---- console state ----
            char s_ConsoleFilter[64] = { 0 };
            f32 s_ConsoleScroll      = 0.0f;
            bool s_ConsoleLevels[3]  = { true, true, true }; // info / trace, warning, error
            bool s_ConsoleFollow     = true;
            // selected lines, by record index (anchor, last clicked); ~0 = none
            u64 s_ConsoleSelA = ~0ull;
            u64 s_ConsoleSelB = ~0ull;

            // ---- stats state ----
            f32 s_FrameTimes[120] = { 0.0f };
            u32 s_FrameCursor     = 0;
            u64 s_FrameStamp      = 0;

            // ---- built-in component inspectors ----

            // ---- rows shared by the component inspectors ----

            // a row that is one button
            bool ActionRow(const char* label, const char* button)
            {
                UIBeginPropertyRow(label);
                const bool pressed = UIButton(button).clicked;
                UIEndPropertyRow();
                return pressed;
            }

            const char* const kImageExtensions = ".png;.jpg;.jpeg;.tga;.bmp;.hdr;.limg";
            char s_TextureMenuPath[512]        = { 0 }; // the texture slot right-clicked

            // does name end in one of these (any case)?
            bool EndsWithOneOf(const char* name, std::initializer_list<const char*> extensions)
            {
                const size_t length = strlen(name);
                for(const char* extension : extensions)
                {
                    const size_t tail = strlen(extension);
                    if(tail <= length && strcasecmp(name + length - tail, extension) == 0)
                        return true;
                }
                return false;
            }

            // Thumbnail + file name; an image dragged from the asset panel, or picked from the list, lands in outPath (VFS form).
            bool TextureRow(const char* label, Graphics::Texture2D* texture, char* outPath, u32 outSize)
            {
                const f32 dpi = GetUIState()->DPIScale;
                UIBeginPropertyRow(label);
                bool dropped        = false;
                char physical[256] = { 0 };
                if(texture)
                {
                    UI_Interaction thumb = UIImageSized("##slot", texture, Vec2(34.0f * dpi, 34.0f * dpi));
                    dropped              = UIDropTarget(kAssetDragType, physical, sizeof(physical));
                    // right click (or hold): open the picture in the Sprite Editor (menu drawn by the Inspector)
                    if(thumb.right_clicked && !texture->GetFilepath().empty())
                    {
                        snprintf(s_TextureMenuPath, sizeof(s_TextureMenuPath), "%s", texture->GetFilepath().c_str());
                        UIOpenPopupAt("##textureslotmenu", UIMousePosition());
                    }
                    const char* path     = texture->GetFilepath().c_str();
                    const char* name     = strrchr(path, '/');
                    {
                        // the picture large, with what it is
                        char caption[400];
                        snprintf(caption, sizeof(caption), "%s%s%u x %u    %u mips", path[0] ? path : "", path[0] ? "\n" : "", texture->GetWidth(), texture->GetHeight(), texture->GetMipMapLevels());
                        UIImageTooltip(thumb, texture, caption);
                    }
                    char info[160];
                    name = name ? name + 1 : path;
                    // embedded textures have no file
                    if(name[0])
                        snprintf(info, sizeof(info), "%s\n%u x %u", name, texture->GetWidth(), texture->GetHeight());
                    else
                        snprintf(info, sizeof(info), "%u x %u", texture->GetWidth(), texture->GetHeight());
                    UIPushStyle(StyleVar_TextColor, UIGetToolTheme().TextDim);
                    UILabelCStr("##slotinfo", info);
                    UIPopStyle(StyleVar_TextColor);
                }
                else
                {
                    UIPushStyle(StyleVar_TextColor, UIGetToolTheme().TextDim);
                    UILabelCStr("##slotnone", "none");
                    UIPopStyle(StyleVar_TextColor);
                    dropped = UIDropTarget(kAssetDragType, physical, sizeof(physical));
                }
                // or pick one from a list
                UIFlexSpacer();
                const bool picked = AssetPickButton("###slotpick", kImageExtensions, outPath, outSize);
                UIEndPropertyRow();
                if(picked)
                    return true;

                if(!dropped || !physical[0])
                    return false;
                ArenaTemp scratch = ScratchBegin(nullptr, 0);
                String8 vfs       = FileSystem::Get().AbsolutePathToFileSystem(scratch.arena, Str8C(physical));
                snprintf(outPath, outSize, "%.*s", (int)vfs.size, (const char*)vfs.str);
                ScratchEnd(scratch);
                return outPath[0] != 0;
            }

            // Name of the entity a component points at; an entity dragged from the tree replaces it.
            bool EntityRow(const char* label, Scene* scene, u64 uuid, u64* outUuid)
            {
                Entity target     = uuid ? scene->GetEntityManager()->GetEntityByUUID(uuid) : Entity();
                const bool valid  = target.Valid();
                UIBeginPropertyRow(label);
                UIPushStyle(StyleVar_TextColor, valid ? UIGetToolTheme().Text : UIGetToolTheme().TextDim);
                UILabelCStr("##entityref", valid ? target.GetName().c_str() : "None - drop an entity");
                UIPopStyle(StyleVar_TextColor);
                u32 dropped    = kNoEntity;
                const bool got = UIDropTarget("lumos_entity", &dropped, sizeof(dropped));
                UIEndPropertyRow();

                if(!got || dropped == kNoEntity || !scene->GetRegistry().valid((entt::entity)dropped))
                    return false;
                *outUuid = (u64)Entity((entt::entity)dropped, scene).GetID();
                return true;
            }

            void DrawTransform(Entity entity, void* component)
            {
                Maths::Transform* transform = (Maths::Transform*)component;
                bool moved                  = false;

                Vec3 position = transform->GetLocalPosition();
                if(UIPropertyVec3("Position", &position.x, 0.02f))
                {
                    transform->SetLocalPosition(position);
                    moved = true;
                }

                Vec3 euler = transform->GetLocalOrientation().ToEuler();
                if(UIPropertyVec3("Rotation", &euler.x, 0.5f))
                {
                    transform->SetLocalOrientation(Quat(euler));
                    moved = true;
                }

                Vec3 scale = transform->GetLocalScale();
                if(UIPropertyVec3("Scale", &scale.x, 0.01f))
                {
                    transform->SetLocalScale(scale);
                    moved = true;
                }

                if(moved)
                    RefreshWorldMatrix(entity.GetScene()->GetRegistry(), entity.GetHandle(), transform);
            }

            void DrawCamera(Entity, void* component)
            {
                Camera* camera = (Camera*)component;

                bool orthographic = camera->IsOrthographic();
                if(UIPropertyBool("Orthographic", &orthographic))
                    camera->SetIsOrthographic(orthographic);

                if(camera->IsOrthographic())
                {
                    float scale = camera->GetScale();
                    if(UIPropertyFloat("Scale", &scale, 0.05f, 0.01f, 10000.0f, "%.2f"))
                        camera->SetScale(scale);
                }
                else
                {
                    float fov = camera->GetFOV();
                    if(UIPropertySlider("Field of View", &fov, 1.0f, 179.0f, "%.1f"))
                        camera->SetFOV(fov);
                }

                float nearPlane = camera->GetNear();
                if(UIPropertyFloat("Near", &nearPlane, 0.01f, 0.001f, 1000.0f, "%.3f"))
                    camera->SetNear(nearPlane);
                float farPlane = camera->GetFar();
                if(UIPropertyFloat("Far", &farPlane, 1.0f, 0.01f, 1000000.0f, "%.1f"))
                    camera->SetFar(farPlane);

                float aperture = camera->GetAperture();
                if(UIPropertyFloat("Aperture", &aperture, 0.05f, 0.5f, 64.0f, "%.1f"))
                    camera->SetAperture(aperture);
                float shutter = camera->GetShutterSpeed();
                if(UIPropertyFloat("Shutter Speed", &shutter, 0.0005f, 0.00001f, 10.0f, "%.4f"))
                    camera->SetShutterSpeed(shutter);
                float sensitivity = camera->GetSensitivity();
                if(UIPropertyFloat("ISO", &sensitivity, 5.0f, 25.0f, 51200.0f, "%.0f"))
                    camera->SetSensitivity(sensitivity);
            }

            void DrawLight(Entity, void* component)
            {
                Graphics::Light* light = (Graphics::Light*)component;

                static const char* kTypes[] = { "Directional", "Spot", "Point" };
                int type                    = Maths::Clamp((int)light->Type, 0, 2);
                if(UIPropertyDropdown("Type", &type, kTypes, 3))
                    light->Type = (float)type;

                UIPropertyColour("Colour", &light->Colour.x, false);
                UIPropertyFloat("Intensity", &light->Intensity, 100.0f, 0.0f, 0.0f, "%.0f");
                if(type != 0)
                    UIPropertyFloat("Radius", &light->Radius, 0.05f, 0.0f, 100000.0f, "%.2f");
                if(type == 1)
                {
                    UIPropertySlider("Angle", &light->Angle, 0.0f, 1.0f);
                    UIPropertyBool("Cast Shadows", &light->CastShadows);
                    if(light->CastShadows)
                    {
                        static const char* kResolutions[] = { "Auto", "256", "512", "1024", "2048" };
                        static const char* kUpdates[]     = { "Every Frame", "On Change" };
                        int resolution = Maths::Clamp((int)light->ShadowResolution, 0, 4);
                        if(UIPropertyDropdown("Shadow Resolution", &resolution, kResolutions, 5))
                            light->ShadowResolution = (u8)resolution;
                        int update = Maths::Clamp((int)light->ShadowUpdate, 0, 1);
                        if(UIPropertyDropdown("Shadow Update", &update, kUpdates, 2))
                            light->ShadowUpdate = (u8)update;
                        UIPropertySlider("Shadow Bias", &light->ShadowBias, 0.0f, 8.0f);
                        UIPropertySlider("Normal Bias", &light->ShadowNormalBias, 0.0f, 8.0f);
                        UIPropertyFloat("Shadow Near", &light->ShadowNear, 0.005f, 0.001f, 100.0f, "%.3f");
                    }
                }
                else if(type == 2)
                    UIPropertyLabel("Shadows", Str8Lit("Spot lights only"));
            }

            void DrawSprite(Entity, void* component)
            {
                Graphics::Sprite* sprite = (Graphics::Sprite*)component;

                Vec4 colour = sprite->GetColour();
                if(UIPropertyColour("Colour", &colour.x))
                    sprite->SetColour(colour);

                Vec2 position = sprite->GetPosition();
                if(UIPropertyVec2("Offset", &position.x, 0.01f))
                    sprite->SetPosition(position);

                Vec2 scale = sprite->GetScale();
                if(UIPropertyVec2("Size", &scale.x, 0.01f))
                    sprite->SetScale(scale);

                char path[256];
                if(TextureRow("Texture", sprite->GetTexture().get(), path, sizeof(path)))
                    sprite->SetTextureFromFile(path);

                // paint it, or sketch its collision shape, in the Sprite Editor
                const SharedPtr<Graphics::Texture2D>& texture = sprite->GetTexture();
                if(texture && !texture->GetFilepath().empty())
                {
                    UIBeginPropertyRow("");
                    if(UIButton(ICON_MDI_PENCIL "  Edit Picture###spriteeditpic").clicked)
                    {
                        SpriteEditor::SetShapeMode(false);
                        SpriteEditor::Open(texture->GetFilepath().c_str());
                    }
                    if(UIButton(ICON_MDI_VECTOR_POLYGON "  Collision Shape###spriteeditshape").clicked && SpriteEditor::Open(texture->GetFilepath().c_str()))
                        SpriteEditor::SetShapeMode(true);
                    UIEndPropertyRow();
                }
            }

            void DrawAnimatedSprite(Entity, void* component)
            {
                Graphics::AnimatedSprite* sprite = (Graphics::AnimatedSprite*)component;

                // the states it has, by name; picking one plays it
                auto& states = sprite->GetAnimationStates();
                constexpr int kMaxStates = 32;
                const char* names[kMaxStates];
                int count   = 0;
                int current = 0;
                for(auto& state : states)
                {
                    if(count >= kMaxStates)
                        break;
                    if(state.first == sprite->GetState())
                        current = count;
                    names[count++] = state.first.c_str();
                }
                if(count > 0)
                {
                    if(UIPropertyDropdown("State", &current, names, count))
                        sprite->SetState(names[current]);
                    auto found = states.find(sprite->GetState());
                    if(found != states.end())
                    {
                        char frames[48];
                        snprintf(frames, sizeof(frames), "%u", (u32)found->second.Frames.size());
                        UIPropertyLabel("Frames", Str8C(frames));
                        UIPropertyFloat("Frame Time", &found->second.FrameDuration, 0.005f, 0.001f, 60.0f, "%.3f s");
                        static const char* kModes[] = { "Loop", "Ping Pong" };
                        int mode                    = (int)found->second.Mode;
                        if(UIPropertyDropdown("Play Mode", &mode, kModes, 2))
                            found->second.Mode = (Graphics::AnimatedSprite::PlayMode)mode;
                    }
                }
                else
                    UIPropertyLabel("State", Str8Lit("no states (add them from a script)"));

                Vec4 colour = sprite->GetColour();
                if(UIPropertyColour("Colour", &colour.x))
                    sprite->SetColour(colour);
                Vec2 position = sprite->GetPosition();
                if(UIPropertyVec2("Offset", &position.x, 0.01f))
                    sprite->SetPosition(position);
                Vec2 scale = sprite->GetScale();
                if(UIPropertyVec2("Size", &scale.x, 0.01f))
                    sprite->SetScale(scale);

                char path[256];
                if(TextureRow("Sprite Sheet", sprite->GetTexture().get(), path, sizeof(path)))
                    sprite->SetTextureFromFile(path);
            }

            void DrawRigidBody2D(Entity, void* component)
            {
                RigidBody2DComponent* bodyComponent = (RigidBody2DComponent*)component;
                RigidBody2D* body                   = bodyComponent->GetRigidBody();
                if(!body)
                {
                    UIPropertyLabel("Body", Str8Lit("not created"));
                    return;
                }

                Vec2 position = body->GetPosition();
                if(UIPropertyVec2("Position", &position.x, 0.02f))
                    body->SetPosition(position);
                f32 angle = body->GetAngle();
                if(UIPropertyFloat("Angle", &angle, 0.01f, 0.0f, 0.0f, "%.2f"))
                    body->SetOrientation(angle);
                Vec2 velocity = body->GetLinearVelocity();
                if(UIPropertyVec2("Velocity", &velocity.x, 0.05f))
                    body->SetLinearVelocity(velocity);

                bool isStatic = body->GetIsStatic();
                if(UIPropertyBool("Static", &isStatic))
                    body->SetIsStatic(isStatic);
                f32 friction = body->GetFriction();
                if(UIPropertySlider("Friction", &friction, 0.0f, 1.0f))
                    body->SetFriction(friction);
                f32 elasticity = body->GetElasticity();
                if(UIPropertySlider("Elasticity", &elasticity, 0.0f, 1.0f))
                    body->SetElasticity(elasticity);
                f32 damping = body->GetDamping();
                if(UIPropertySlider("Damping", &damping, 0.0f, 1.0f))
                    body->SetLinearDamping(damping);

                static const char* kShapes[] = { "Square", "Circle", "Custom" };
                const int shape              = (int)body->GetShapeType();
                UIPropertyLabel("Shape", Str8C((char*)kShapes[shape >= 0 && shape < 3 ? shape : 2]));
                UIPropertyLabel("At Rest", body->GetIsAtRest() ? Str8Lit("yes") : Str8Lit("no"));
            }

            // half size of the entity's model in body space (scale included), centred on the body
            static bool ModelHalfSize(Entity entity, Vec3& outHalf)
            {
                Graphics::ModelComponent* model = entity.TryGetComponent<Graphics::ModelComponent>();
                if(!model || !model->ModelRef || model->ModelRef->GetMeshes().Empty())
                    return false;
                Maths::BoundingBox box;
                for(auto& mesh : model->ModelRef->GetMeshes())
                    if(mesh)
                        box.Merge(mesh->GetBoundingBox());
                Vec3 scale(1.0f);
                if(Maths::Transform* t = entity.TryGetComponent<Maths::Transform>())
                    scale = t->GetWorldScale();
                const Vec3 size = (box.Max() - box.Min()) * 0.5f;
                outHalf         = Vec3(Maths::Abs(size.x * scale.x), Maths::Abs(size.y * scale.y), Maths::Abs(size.z * scale.z));
                return outHalf.x > 0.0f || outHalf.y > 0.0f || outHalf.z > 0.0f;
            }

            // size a simple shape to the model so a moving body gets a sensible collider
            static void FitShapeToModel(Entity entity, RigidBody3D* body)
            {
                Vec3 half;
                const SharedPtr<CollisionShape>& shape = body->GetCollisionShape();
                if(!shape || !ModelHalfSize(entity, half))
                    return;
                half = Vec3(Maths::Max(half.x, 0.001f), Maths::Max(half.y, 0.001f), Maths::Max(half.z, 0.001f));
                switch(shape->GetType())
                {
                case CollisionCuboid:
                    ((CuboidCollisionShape*)shape.get())->SetHalfDimensions(half);
                    break;
                case CollisionPyramid:
                    ((PyramidCollisionShape*)shape.get())->SetHalfDimensions(half);
                    break;
                case CollisionSphere:
                    ((SphereCollisionShape*)shape.get())->SetRadius(Maths::Max(half.x, Maths::Max(half.y, half.z)));
                    break;
                case CollisionCapsule:
                {
                    // upright: radius from the wider of x / z, the rest of the height is the segment
                    const float radius = Maths::Max(half.x, half.z);
                    auto* capsule      = (CapsuleCollisionShape*)shape.get();
                    capsule->SetRadius(radius);
                    capsule->SetHeight(Maths::Max(half.y * 2.0f - radius * 2.0f, 0.0f));
                    break;
                }
                default:
                    return;
                }
                body->CollisionShapeUpdated();
            }

            void DrawRigidBody3D(Entity entity, void* component)
            {
                RigidBody3DComponent* bodyComponent = (RigidBody3DComponent*)component;
                RigidBody3D* body                   = bodyComponent->GetRigidBody();
                if(!body)
                    return;

                Vec3 position = body->GetPosition();
                if(UIPropertyVec3("Position", &position.x, 0.02f))
                    body->SetPosition(position);

                Vec3 velocity = body->GetLinearVelocity();
                if(UIPropertyVec3("Velocity", &velocity.x, 0.05f))
                    body->SetLinearVelocity(velocity);

                float friction = body->GetFriction();
                if(UIPropertySlider("Friction", &friction, 0.0f, 1.0f))
                    body->SetFriction(friction);
                float elasticity = body->GetElasticity();
                if(UIPropertySlider("Elasticity", &elasticity, 0.0f, 1.0f))
                    body->SetElasticity(elasticity);

                // triangle colliders only work standing still: a moving one would fall through everything
                const bool meshShape = body->GetCollisionShape() && body->GetCollisionShape()->GetType() == CollisionMesh;
                bool isStatic        = body->GetIsStatic();
                if(meshShape)
                    UIPropertyLabel("Static", Str8Lit("yes (mesh colliders do not move)"));
                else if(UIPropertyBool("Static", &isStatic))
                    body->SetIsStatic(isStatic);
                if(!isStatic && !meshShape)
                {
                    const float inverseMass = body->GetInverseMass();
                    float mass              = inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
                    if(UIPropertyFloat("Mass", &mass, 0.05f, 0.0001f, 1000000.0f, "%.2f"))
                        body->SetInverseMass(1.0f / Maths::Max(mass, 0.0001f));
                }

                Vec3 angular = body->GetAngularVelocity();
                if(UIPropertyVec3("Angular Velocity", &angular.x, 0.05f))
                    body->SetAngularVelocity(angular);

                bool trigger = body->GetIsTrigger();
                if(UIPropertyBool("Trigger", &trigger))
                    body->SetIsTrigger(trigger);

                // 0 on an axis locks it
                Vec3 linearFactor = body->GetLinearFactor();
                if(UIPropertyVec3("Move Axes", &linearFactor.x, 0.05f))
                    body->SetLinearFactor(Vec3(Maths::Clamp(linearFactor.x, 0.0f, 1.0f), Maths::Clamp(linearFactor.y, 0.0f, 1.0f), Maths::Clamp(linearFactor.z, 0.0f, 1.0f)));
                Vec3 angularFactor = body->GetAngularFactor();
                if(UIPropertyVec3("Turn Axes", &angularFactor.x, 0.05f))
                    body->SetAngularFactor(Vec3(Maths::Clamp(angularFactor.x, 0.0f, 1.0f), Maths::Clamp(angularFactor.y, 0.0f, 1.0f), Maths::Clamp(angularFactor.z, 0.0f, 1.0f)));

                int layer = (int)body->GetCollisionLayer();
                if(UIPropertyInt("Layer", &layer, 0.1f, 0, 15))
                    body->SetCollisionLayer((u16)Maths::Clamp(layer, 0, 15));

                // ---- shape ----
                UIPropertyHeading("Collision Shape");
                // the ones that can be made from nothing; a hull, compound or terrain comes from code or an import
                static const char* kShapes[]              = { "Cuboid", "Sphere", "Pyramid", "Capsule", "Mesh (static, from the model)" };
                static const CollisionShapeType kTypes[]  = { CollisionCuboid, CollisionSphere, CollisionPyramid, CollisionCapsule, CollisionMesh };
                const SharedPtr<CollisionShape>& shape    = body->GetCollisionShape();
                const u32 type                            = shape ? (u32)shape->GetType() : 0u;
                int shapeIndex                            = -1;
                for(int i = 0; i < 5; i++)
                    if(type == (u32)kTypes[i])
                        shapeIndex = i;

                if(shapeIndex < 0 && shape)
                {
                    const char* name = type == CollisionHull ? "Hull" : (type == CollisionCompound ? "Compound" : (type == CollisionTerrain ? "Terrain" : "Other"));
                    UIPropertyLabel("Type", Str8C((char*)name));
                }
                int picked = Maths::Max(shapeIndex, 0);
                if(UIPropertyDropdown(shapeIndex < 0 && shape ? "Replace With" : "Type", &picked, kShapes, 5) && (picked != shapeIndex || !shape))
                {
                    body->SetCollisionShape(kTypes[picked]);
                    // a mesh does not move; the simple shapes start at the model's size
                    if(kTypes[picked] == CollisionMesh)
                        body->SetIsStatic(true);
                    else
                        FitShapeToModel(entity, body);
                    body->CollisionShapeUpdated();
                    return;
                }

                if(type == CollisionCuboid)
                {
                    CuboidCollisionShape* cuboid = (CuboidCollisionShape*)shape.get();
                    Vec3 half                    = cuboid->GetHalfDimensions();
                    if(UIPropertyVec3("Half Size", &half.x, 0.02f))
                    {
                        cuboid->SetHalfDimensions(Vec3(Maths::Max(half.x, 0.001f), Maths::Max(half.y, 0.001f), Maths::Max(half.z, 0.001f)));
                        body->CollisionShapeUpdated();
                    }
                }
                else if(type == CollisionSphere)
                {
                    SphereCollisionShape* sphere = (SphereCollisionShape*)shape.get();
                    float radius                 = sphere->GetRadius();
                    if(UIPropertyFloat("Radius", &radius, 0.02f, 0.001f, 100000.0f, "%.3f"))
                    {
                        sphere->SetRadius(Maths::Max(radius, 0.001f));
                        body->CollisionShapeUpdated();
                    }
                }
                else if(type == CollisionPyramid)
                {
                    PyramidCollisionShape* pyramid = (PyramidCollisionShape*)shape.get();
                    Vec3 half                      = pyramid->GetHalfDimensions();
                    if(UIPropertyVec3("Half Size", &half.x, 0.02f))
                    {
                        pyramid->SetHalfDimensions(Vec3(Maths::Max(half.x, 0.001f), Maths::Max(half.y, 0.001f), Maths::Max(half.z, 0.001f)));
                        body->CollisionShapeUpdated();
                    }
                }
                else if(type == CollisionCapsule)
                {
                    CapsuleCollisionShape* capsule = (CapsuleCollisionShape*)shape.get();
                    float radius                   = capsule->GetRadius();
                    float height                   = capsule->GetHeight();
                    bool changed                   = UIPropertyFloat("Radius", &radius, 0.02f, 0.001f, 100000.0f, "%.3f");
                    changed |= UIPropertyFloat("Height", &height, 0.02f, 0.0f, 100000.0f, "%.3f");
                    if(changed)
                    {
                        capsule->SetRadius(Maths::Max(radius, 0.001f));
                        capsule->SetHeight(Maths::Max(height, 0.0f));
                        body->CollisionShapeUpdated();
                    }
                }

                else if(type == CollisionMesh)
                {
                    MeshCollisionShape* mesh = (MeshCollisionShape*)shape.get();
                    char tris[64];
                    snprintf(tris, sizeof(tris), "%u%s", mesh->GetTriangleCount(), mesh->IsBuilt() ? "" : " (building)");
                    UIPropertyLabel("Triangles", Str8C(tris));
                    // applied with the button: a rebuild of a big model is too slow to run on every drag step
                    static float s_Detail      = 1.0f;
                    static const void* s_Owner = nullptr;
                    if(s_Owner != mesh)
                    {
                        s_Owner  = mesh;
                        s_Detail = mesh->GetDetail();
                    }
                    UIPropertySlider("Detail", &s_Detail, 0.01f, 1.0f);
                    UIBeginPropertyRow("##meshrebuild");
                    if(UIButton(s_Detail != mesh->GetDetail() ? "Rebuild at this detail###meshrebuild" : "Rebuild###meshrebuild").clicked)
                    {
                        mesh->SetDetail(s_Detail);
                        mesh->RequestRebuild();
                    }
                    UIEndPropertyRow();
                }

                if(type == CollisionCuboid || type == CollisionSphere || type == CollisionPyramid || type == CollisionCapsule)
                {
                    UIBeginPropertyRow("##fitshape");
                    if(UIButton("Fit to Model###fitshape").clicked)
                        FitShapeToModel(entity, body);
                    UIEndPropertyRow();
                }

                const Maths::BoundingBox bounds = body->GetLocalBoundingBox();
                const Vec3 extent               = bounds.Max() - bounds.Min();
                char size[64];
                snprintf(size, sizeof(size), "%.2f  %.2f  %.2f", extent.x, extent.y, extent.z);
                UIPropertyLabel("Bounds", Str8C(size));
                UIPropertyLabel("At Rest", body->GetIsAtRest() ? Str8Lit("yes") : Str8Lit("no"));
            }

            // One material: colour and maps first, then the surface numbers.
            void DrawMaterialRows(Graphics::Material* material, u32 index)
            {
                Graphics::MaterialProperties properties = *material->GetProperties();
                bool changed                            = false;
                char label[64];
                char path[256];
                auto id = [&](const char* name)
                {
                    snprintf(label, sizeof(label), "%s###%s%u", name, name, index);
                    return label;
                };

                changed |= UIPropertyColour(id("Albedo"), &properties.albedoColour.x);
                if(TextureRow(id("Albedo Map"), material->GetTextures().albedo.get(), path, sizeof(path)))
                    material->SetAlbedoTexture(path);
                if(TextureRow(id("Normal Map"), material->GetTextures().normal.get(), path, sizeof(path)))
                    material->SetNormalTexture(path);

                changed |= UIPropertySlider(id("Roughness"), &properties.roughness, 0.0f, 1.0f);
                if(TextureRow(id("Roughness Map"), material->GetTextures().roughness.get(), path, sizeof(path)))
                    material->SetRoughnessTexture(path);
                changed |= UIPropertySlider(id("Metallic"), &properties.metallic, 0.0f, 1.0f);
                if(TextureRow(id("Metallic Map"), material->GetTextures().metallic.get(), path, sizeof(path)))
                    material->SetMetallicTexture(path);
                changed |= UIPropertySlider(id("Reflectance"), &properties.reflectance, 0.0f, 1.0f);

                changed |= UIPropertyColour(id("Emissive"), &properties.emissiveColour.x, false);
                changed |= UIPropertyFloat(id("Emissive Strength"), &properties.emissiveColour.w, 0.05f, 0.0f, 100000.0f, "%.2f");
                if(TextureRow(id("Emissive Map"), material->GetTextures().emissive.get(), path, sizeof(path)))
                    material->SetEmissiveTexture(path);
                if(TextureRow(id("Occlusion Map"), material->GetTextures().ao.get(), path, sizeof(path)))
                    material->SetAOTexture(path);

                changed |= UIPropertySlider(id("Alpha Cutoff"), &properties.alphaCutoff, 0.0f, 1.0f);
                if(changed)
                    material->SetMaterialProperites(properties);
            }

            // a free //Assets/Materials/<name>.lmat (folder made if needed)
            static std::string NewMaterialPath(const std::string& name)
            {
                char base[64] = { 0 };
                u32 n         = 0;
                for(char c : name)
                    if(n + 1 < sizeof(base) && (isalnum((unsigned char)c) || c == '_' || c == '-'))
                        base[n++] = c;
                if(n == 0)
                    snprintf(base, sizeof(base), "Material");

                ArenaTemp scratch = ScratchBegin(nullptr, 0);
                String8 folder;
                FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8Lit("//Assets/Materials"), &folder, true);
                if(folder.size)
                    FileSystem::CreateFolderIfDoesntExist(folder);
                ScratchEnd(scratch);

                char path[160];
                snprintf(path, sizeof(path), "//Assets/Materials/%s.lmat", base);
                for(u32 i = 1; FileSystem::Get().FileExistsVFS(Str8C(path)) && i < 1000; i++)
                    snprintf(path, sizeof(path), "//Assets/Materials/%s_%u.lmat", base, i);
                return path;
            }

            void DrawModel(Entity entity, void* component)
            {
                Graphics::ModelComponent* model = (Graphics::ModelComponent*)component;
                UIPropertyBool("Visible", &model->Visible);
                if(!model->ModelRef)
                {
                    UIPropertyLabel("Model", Str8Lit("not loaded"));
                    return;
                }

                // ---- what it is ----
                {
                    const std::string path = model->ModelRef->GetFilePath();
                    const char* name       = strrchr(path.c_str(), '/');
                    char picked[256]       = { 0 };
                    UIBeginPropertyRow("File");
                    UIPushStyle(StyleVar_TextColor, path.empty() ? UIGetToolTheme().TextDim : UIGetToolTheme().Text);
                    UILabelEllipsis("##modelfile", Str8C((char*)(path.empty() ? "built in" : (name ? name + 1 : path.c_str()))), 150.0f);
                    UIPopStyle(StyleVar_TextColor);
                    bool change = AssetDropTarget(nullptr, picked, sizeof(picked)) && EndsWithOneOf(picked, { ".glb", ".gltf", ".obj", ".fbx", ".lmesh" });
                    UIFlexSpacer();
                    change |= AssetPickButton("###modelpick", ".glb;.gltf;.obj;.fbx;.lmesh", picked, sizeof(picked));
                    UIEndPropertyRow();
                    if(change && picked[0])
                    {
                        model->LoadFromLibrary(picked);
                        return; // the meshes listed below belong to the old one
                    }
                }
                const auto& meshes = model->ModelRef->GetMeshes();
                u32 triangles      = 0, vertices = 0;
                for(u32 i = 0; i < (u32)meshes.Size(); i++)
                {
                    if(!meshes[i])
                        continue;
                    triangles += meshes[i]->GetStats().TriangleCount;
                    vertices += meshes[i]->GetStats().VertexCount;
                }
                // short counts: the column is narrow
                auto count = [](char* out, size_t size, u32 value)
                {
                    if(value >= 1000000)
                        snprintf(out, size, "%.1fM", (f32)value / 1000000.0f);
                    else if(value >= 10000)
                        snprintf(out, size, "%uk", value / 1000);
                    else
                        snprintf(out, size, "%u", value);
                };
                char tris[16], verts[16], text[96];
                count(tris, sizeof(tris), triangles);
                count(verts, sizeof(verts), vertices);
                snprintf(text, sizeof(text), "%u    %s tris    %s verts", (u32)meshes.Size(), tris, verts);
                UIPropertyLabel("Meshes", Str8C(text));

                // ---- materials: the ones that look the same are one entry, however many meshes wear them ----
                auto materialOf = [&](u32 i) -> Graphics::Material*
                {
                    if(!meshes[i])
                        return nullptr;
                    auto instance                = model->InstanceMaterials.find(i);
                    Graphics::Material* material = instance != model->InstanceMaterials.end() && instance->second ? instance->second.get() : meshes[i]->GetMaterial().get();
                    return material && material->GetProperties() ? material : nullptr;
                };
                // an import often makes one material object per mesh: same maps + same numbers = the same material here
                auto same = [](Graphics::Material* a, Graphics::Material* b)
                {
                    if(a == b)
                        return true;
                    const auto& ta = a->GetTextures();
                    const auto& tb = b->GetTextures();
                    return ta.albedo == tb.albedo && ta.normal == tb.normal && ta.metallic == tb.metallic && ta.roughness == tb.roughness
                        && ta.ao == tb.ao && ta.emissive == tb.emissive
                        && memcmp(a->GetProperties(), b->GetProperties(), sizeof(Graphics::MaterialProperties)) == 0;
                };

                constexpr u32 kMaxMaterials = 48;
                Graphics::Material* materials[kMaxMaterials];
                u32 users[kMaxMaterials];
                u32 firstMesh[kMaxMaterials];
                u32 materialCount = 0;
                u32 left          = 0;
                for(u32 i = 0; i < (u32)meshes.Size(); i++)
                {
                    Graphics::Material* material = materialOf(i);
                    if(!material)
                        continue;
                    u32 slot = 0;
                    while(slot < materialCount && !same(materials[slot], material))
                        slot++;
                    if(slot == materialCount)
                    {
                        if(materialCount >= kMaxMaterials)
                        {
                            left++;
                            continue;
                        }
                        materials[materialCount] = material;
                        users[materialCount]     = 0;
                        firstMesh[materialCount] = i;
                        materialCount++;
                    }
                    users[slot]++;
                }

                if(materialCount == 0)
                    return;
                snprintf(text, sizeof(text), materialCount == 1 ? "Material###modelmaterials" : "%u Materials###modelmaterials", materialCount);
                UIPropertyHeading(text);

                // other entities drawing the same model wear the same materials unless given their own
                u32 sharedWith = 0;
                if(Scene* scene = entity.GetScene())
                {
                    auto view = scene->GetRegistry().view<Graphics::ModelComponent>();
                    for(auto other : view)
                        if(other != entity.GetHandle() && view.get<Graphics::ModelComponent>(other).ModelRef == model->ModelRef)
                            sharedWith++;
                }
                for(u32 m = 0; m < materialCount; m++)
                {
                    Graphics::Material* material = materials[m];
                    Graphics::Texture2D* albedo  = material->GetTextures().albedo.get();

                    // one material needs no fold; several start shut past the first so the list reads as a list
                    bool open = true;
                    if(materialCount > 1)
                    {
                        // the best name going: the material's, the mesh's, the albedo file's, then a number
                        char name[96] = { 0 };
                        if(!material->GetName().empty())
                            snprintf(name, sizeof(name), "%s", material->GetName().c_str());
                        else if(!meshes[firstMesh[m]]->GetName().empty())
                            snprintf(name, sizeof(name), "%s", meshes[firstMesh[m]]->GetName().c_str());
                        else if(albedo && !albedo->GetFilepath().empty())
                        {
                            const char* file = strrchr(albedo->GetFilepath().c_str(), '/');
                            snprintf(name, sizeof(name), "%s", file ? file + 1 : albedo->GetFilepath().c_str());
                            if(char* dot = strrchr(name, '.'))
                                *dot = 0;
                        }
                        else
                            snprintf(name, sizeof(name), "Material %u", m + 1);

                        char title[160];
                        if(users[m] > 1)
                            snprintf(title, sizeof(title), "%s     x%u###material%u", name, users[m], m);
                        else
                            snprintf(title, sizeof(title), "%s###material%u", name, m);
                        open = UIPropertyFold(title, m == 0, albedo);
                    }
                    if(!open)
                        continue;

                    // edited through the first of the group; the rest are brought into line with it
                    const Graphics::MaterialProperties before = *material->GetProperties();
                    const Graphics::PBRMataterialTextures mapsBefore = material->GetTextures();

                    // the meshes of this entity wearing this entry
                    auto inGroup = [&](u32 i)
                    {
                        Graphics::Material* other = materialOf(i);
                        if(!other)
                            return false;
                        if(other == material)
                            return true;
                        const auto& t = other->GetTextures();
                        return t.albedo == mapsBefore.albedo && t.normal == mapsBefore.normal && t.metallic == mapsBefore.metallic && t.roughness == mapsBefore.roughness
                            && t.ao == mapsBefore.ao && t.emissive == mapsBefore.emissive && memcmp(other->GetProperties(), &before, sizeof(before)) == 0;
                    };
                    auto overrideAll = [&](const std::string& path)
                    {
                        for(u32 i = 0; i < (u32)meshes.Size(); i++)
                            if(meshes[i] && inGroup(i))
                                model->SetMaterialOverride(i, path);
                    };
                    auto found                     = model->MaterialOverrides.find(firstMesh[m]);
                    const std::string overridePath = found != model->MaterialOverrides.end() ? found->second : std::string();

                    // where it comes from: the model (shared, not saved) or a .lmat of this entity's
                    {
                        char rowLabel[48];
                        char pickId[32];
                        char picked[256] = { 0 };
                        snprintf(rowLabel, sizeof(rowLabel), "Material File###materialfile%u", m);
                        snprintf(pickId, sizeof(pickId), "###materialpick%u", m);
                        UIBeginPropertyRow(rowLabel);
                        UIPushStyle(StyleVar_TextColor, overridePath.empty() ? UIGetToolTheme().TextDim : UIGetToolTheme().Text);
                        const char* shown = overridePath.empty() ? "from the model" : strrchr(overridePath.c_str(), '/');
                        UILabelEllipsis("##materialfilehint", Str8C((char*)(shown && *shown == '/' ? shown + 1 : shown)), 120.0f);
                        UIPopStyle(StyleVar_TextColor);
                        bool change = AssetDropTarget(".lmat", picked, sizeof(picked));
                        UIFlexSpacer();
                        change |= AssetPickButton(pickId, ".lmat", picked, sizeof(picked));
                        UIEndPropertyRow();
                        if(change && picked[0])
                        {
                            overrideAll(picked);
                            continue;
                        }
                    }

                    {
                        char rowId[32];
                        snprintf(rowId, sizeof(rowId), "##materialactions%u", m);
                        UIBeginPropertyRow(rowId);
                        char buttonId[64];
                        if(overridePath.empty())
                        {
                            // a copy in a file of its own, for this entity only: edits stay off the other users and are saved
                            snprintf(buttonId, sizeof(buttonId), "Make Unique###makeunique%u", m);
                            if(UIButton(buttonId).clicked)
                            {
                                const std::string path = NewMaterialPath(material->GetName().empty() ? std::string("Material") : material->GetName());
                                if(Graphics::ModelComponent::WriteMaterialFile(*material, path))
                                    overrideAll(path);
                                else
                                    LERROR("Inspector: could not write %s", path.c_str());
                                UIEndPropertyRow();
                                continue;
                            }
                        }
                        else
                        {
                            snprintf(buttonId, sizeof(buttonId), "Save###savematerial%u", m);
                            if(UIButton(buttonId).clicked && !Graphics::ModelComponent::WriteMaterialFile(*material, overridePath))
                                LERROR("Inspector: could not write %s", overridePath.c_str());
                            snprintf(buttonId, sizeof(buttonId), "Use Model's###clearmaterial%u", m);
                            if(UIButton(buttonId).clicked)
                            {
                                overrideAll(std::string());
                                UIEndPropertyRow();
                                continue;
                            }
                        }
                        UIEndPropertyRow();
                    }
                    if(overridePath.empty())
                    {
                        if(sharedWith > 0)
                            snprintf(text, sizeof(text), "edits change %u other entit%s too, and are not saved", sharedWith, sharedWith == 1 ? "y" : "ies");
                        else
                            snprintf(text, sizeof(text), "edits are not saved: Make Unique to keep them");
                        UIPushStyle(StyleVar_TextColor, UIGetToolTheme().TextDim);
                        UIPropertyLabel("", Str8C(text));
                        UIPopStyle(StyleVar_TextColor);
                    }

                    DrawMaterialRows(material, m);
                    if(users[m] > 1)
                    {
                        const auto& mapsNow    = material->GetTextures();
                        const bool mapsChanged = mapsNow.albedo != mapsBefore.albedo || mapsNow.normal != mapsBefore.normal || mapsNow.metallic != mapsBefore.metallic
                            || mapsNow.roughness != mapsBefore.roughness || mapsNow.ao != mapsBefore.ao || mapsNow.emissive != mapsBefore.emissive;
                        const bool numbersChanged = memcmp(&before, material->GetProperties(), sizeof(before)) != 0;
                        if(mapsChanged || numbersChanged)
                        {
                            for(u32 i = 0; i < (u32)meshes.Size(); i++)
                            {
                                Graphics::Material* other = materialOf(i);
                                if(!other || other == material)
                                    continue;
                                // was it one of this group, as the group stood before the edit?
                                const auto& t = other->GetTextures();
                                if(t.albedo != mapsBefore.albedo || t.normal != mapsBefore.normal || t.metallic != mapsBefore.metallic || t.roughness != mapsBefore.roughness
                                   || t.ao != mapsBefore.ao || t.emissive != mapsBefore.emissive || memcmp(other->GetProperties(), &before, sizeof(before)) != 0)
                                    continue;
                                if(numbersChanged)
                                    other->SetMaterialProperites(*material->GetProperties());
                                if(mapsChanged)
                                    other->SetTextures(mapsNow);
                            }
                        }
                    }
                }
                if(left > 0)
                {
                    snprintf(text, sizeof(text), "+ %u more not listed", left);
                    UIPropertyLabel("", Str8C(text));
                }

                // ---- meshes: what each one wears, and a way to give it another ----
                snprintf(text, sizeof(text), meshes.Size() == 1 ? "Mesh###modelmeshes" : "%u Meshes###modelmeshes", (u32)meshes.Size());
                UIPropertyHeading(text);
                // shut by default on anything big: it is a long list
                if(meshes.Size() > 6 && !UIPropertyFold("Mesh list###modelmeshlist", false))
                    return;

                // the names the choice is made from (same order and naming as the list above)
                static char names[kMaxMaterials][72];
                const char* options[kMaxMaterials];
                for(u32 m = 0; m < materialCount; m++)
                {
                    Graphics::Texture2D* albedo = materials[m]->GetTextures().albedo.get();
                    if(!materials[m]->GetName().empty())
                        snprintf(names[m], sizeof(names[m]), "%s", materials[m]->GetName().c_str());
                    else if(!meshes[firstMesh[m]]->GetName().empty())
                        snprintf(names[m], sizeof(names[m]), "%s", meshes[firstMesh[m]]->GetName().c_str());
                    else if(albedo && !albedo->GetFilepath().empty())
                    {
                        const char* file = strrchr(albedo->GetFilepath().c_str(), '/');
                        snprintf(names[m], sizeof(names[m]), "%s", file ? file + 1 : albedo->GetFilepath().c_str());
                    }
                    else
                        snprintf(names[m], sizeof(names[m]), "Material %u", m + 1);
                    options[m] = names[m];
                }

                constexpr u32 kMaxRows = 64;
                const u32 rows         = Maths::Min((u32)meshes.Size(), kMaxRows);
                for(u32 i = 0; i < rows; i++)
                {
                    Graphics::Material* material = materialOf(i);
                    if(!material)
                        continue;
                    int slot = 0;
                    while(slot < (int)materialCount && !same(materials[slot], material))
                        slot++;
                    if(slot >= (int)materialCount)
                        continue;

                    char trisText[16];
                    count(trisText, sizeof(trisText), meshes[i]->GetStats().TriangleCount);
                    char label[128];
                    if(!meshes[i]->GetName().empty())
                        snprintf(label, sizeof(label), "%s  (%s)###mesh%u", meshes[i]->GetName().c_str(), trisText, i);
                    else
                        snprintf(label, sizeof(label), "Mesh %u  (%s)###mesh%u", i + 1, trisText, i);

                    int chosen = slot;
                    if(UIPropertyDropdown(label, &chosen, options, (int)materialCount) && chosen != slot && chosen >= 0 && chosen < (int)materialCount)
                    {
                        // on this entity only: the model's own material stays as it was for anything else using it
                        const u32 from = firstMesh[chosen];
                        auto saved     = model->MaterialOverrides.find(from);
                        if(saved != model->MaterialOverrides.end())
                            model->SetMaterialOverride(i, saved->second); // kept with the scene
                        else
                        {
                            auto instance                        = model->InstanceMaterials.find(from);
                            SharedPtr<Graphics::Material> shared = instance != model->InstanceMaterials.end() && instance->second ? instance->second : meshes[from]->GetMaterial();
                            model->MaterialOverrides.erase(i);
                            if(shared)
                                model->InstanceMaterials[i] = shared;
                        }
                    }
                }
                if(meshes.Size() > rows)
                {
                    snprintf(text, sizeof(text), "+ %u more not listed", (u32)meshes.Size() - rows);
                    UIPropertyLabel("", Str8C(text));
                }
            }

            void DrawLight2D(Entity, void* component)
            {
                Graphics::Light2D* light = (Graphics::Light2D*)component;

                static const char* kTypes[] = { "Point", "Spot", "Global" };
                int type                    = Maths::Clamp((int)light->Type, 0, 2);
                if(UIPropertyDropdown("Type", &type, kTypes, 3))
                    light->Type = (float)type;

                UIPropertyColour("Colour", &light->Colour.x, false);
                UIPropertyFloat("Intensity", &light->Intensity, 0.01f, 0.0f, 1000.0f, "%.2f");
                if(type != 2)
                {
                    UIPropertyFloat("Radius", &light->Radius, 0.05f, 0.0f, 100000.0f, "%.2f");
                    UIPropertyFloat("Height", &light->Height, 0.01f, 0.0f, 1000.0f, "%.2f");
                    UIPropertyFloat("Falloff", &light->Falloff, 0.01f, 0.01f, 16.0f, "%.2f");
                }
                if(type == 1)
                {
                    UIPropertyFloat("Inner Angle", &light->InnerAngle, 0.005f, -1.0f, 1.0f, "%.2f");
                    UIPropertyFloat("Outer Angle", &light->OuterAngle, 0.005f, -1.0f, 1.0f, "%.2f");
                }
            }

            void DrawSound(Entity, void* component)
            {
                SoundNode* node = ((SoundComponent*)component)->GetSoundNode();
                if(!node)
                    return;

                {
                    const std::string path = node->GetSound() ? node->GetSound()->GetFilePath() : std::string();
                    const char* name       = strrchr(path.c_str(), '/');
                    char picked[256]       = { 0 };
                    UIBeginPropertyRow("File");
                    UIPushStyle(StyleVar_TextColor, path.empty() ? UIGetToolTheme().TextDim : UIGetToolTheme().Text);
                    UILabelEllipsis("##soundfile", Str8C((char*)(path.empty() ? "none" : (name ? name + 1 : path.c_str()))), 150.0f);
                    UIPopStyle(StyleVar_TextColor);
                    bool change = AssetDropTarget(nullptr, picked, sizeof(picked)) && EndsWithOneOf(picked, { ".ogg", ".wav", ".mp3", ".m4a" });
                    UIFlexSpacer();
                    change |= AssetPickButton("###soundpick", ".ogg;.wav;.mp3;.m4a", picked, sizeof(picked));
                    UIEndPropertyRow();
                    if(change && picked[0])
                    {
                        const char* dot = strrchr(picked, '.');
                        if(SharedPtr<Sound> sound = Sound::Create(picked, dot ? dot + 1 : ""))
                            node->SetSound(sound);
                    }
                }

                float volume = node->GetVolume();
                if(UIPropertySlider("Volume", &volume, 0.0f, 1.0f))
                    node->SetVolume(volume);
                float pitch = node->GetPitch();
                if(UIPropertySlider("Pitch", &pitch, 0.01f, 4.0f))
                    node->SetPitch(pitch);
                float radius = node->GetRadius();
                if(UIPropertyFloat("Radius", &radius, 0.1f, 0.0f, 100000.0f, "%.1f"))
                    node->SetRadius(radius);
                float reference = node->GetReferenceDistance();
                if(UIPropertyFloat("Reference Distance", &reference, 0.05f, 0.0f, 100000.0f, "%.2f"))
                    node->SetReferenceDistance(reference);
                float rollOff = node->GetRollOffFactor();
                if(UIPropertyFloat("Roll Off", &rollOff, 0.01f, 0.0f, 100.0f, "%.2f"))
                    node->SetRollOffFactor(rollOff);

                bool looping = node->GetLooping();
                if(UIPropertyBool("Looping", &looping))
                    node->SetLooping(looping);
                bool paused = node->GetPaused();
                if(UIPropertyBool("Paused", &paused))
                    node->SetPaused(paused);
                bool global = node->GetIsGlobal();
                if(UIPropertyBool("Global", &global))
                    node->SetIsGlobal(global);
            }

            void DrawParticles(Entity, void* component)
            {
                ParticleEmitter* emitter = (ParticleEmitter*)component;

                int count = (int)emitter->GetParticleCount();
                if(UIPropertyInt("Max Particles", &count, 1.0f, 1, 100000))
                    emitter->SetParticleCount((u32)count);
                float rate = emitter->GetParticleRate();
                if(UIPropertyFloat("Rate", &rate, 0.005f, 0.0f, 100.0f, "%.3f"))
                    emitter->SetParticleRate(rate);
                int launch = (int)emitter->GetNumLaunchParticles();
                if(UIPropertyInt("Per Launch", &launch, 0.2f, 0, 10000))
                    emitter->SetNumLaunchParticles((u32)launch);

                float size = emitter->GetParticleSize();
                if(UIPropertyFloat("Size", &size, 0.005f, 0.0f, 1000.0f, "%.2f"))
                    emitter->SetParticleSize(size);
                float life = emitter->GetParticleLife();
                if(UIPropertyFloat("Life", &life, 0.01f, 0.0f, 1000.0f, "%.2f"))
                    emitter->SetParticleLife(life);
                float lifeSpread = emitter->GetLifeSpread();
                if(UIPropertyFloat("Life Spread", &lifeSpread, 0.01f, 0.0f, 1000.0f, "%.2f"))
                    emitter->SetLifeSpread(lifeSpread);

                Vec4 colour = emitter->GetInitialColour();
                if(UIPropertyColour("Colour", &colour.x))
                    emitter->SetInitialColour(colour);

                Vec3 velocity = emitter->GetInitialVelocity();
                if(UIPropertyVec3("Velocity", &velocity.x, 0.02f))
                    emitter->SetInitialVelocity(velocity);
                Vec3 velocitySpread = emitter->GetVelocitySpread();
                if(UIPropertyVec3("Velocity Spread", &velocitySpread.x, 0.02f))
                    emitter->SetVelocitySpread(velocitySpread);
                Vec3 spread = emitter->GetSpread();
                if(UIPropertyVec3("Position Spread", &spread.x, 0.02f))
                    emitter->SetSpread(spread);
                Vec3 gravity = emitter->GetGravity();
                if(UIPropertyVec3("Gravity", &gravity.x, 0.02f))
                    emitter->SetGravity(gravity);

                float fadeIn = emitter->GetFadeIn();
                if(UIPropertyFloat("Fade In", &fadeIn, 0.01f, 0.0f, 100.0f, "%.2f"))
                    emitter->SetFadeIn(fadeIn);
                float fadeOut = emitter->GetFadeOut();
                if(UIPropertyFloat("Fade Out", &fadeOut, 0.01f, 0.0f, 100.0f, "%.2f"))
                    emitter->SetFadeOut(fadeOut);

                static const char* kBlend[] = { "Additive", "Alpha", "Off" };
                int blend                   = (int)emitter->GetBlendType();
                if(UIPropertyDropdown("Blend", &blend, kBlend, 3))
                    emitter->SetBlendType((ParticleEmitter::BlendType)blend);
                static const char* kAlign[] = { "2D", "3D", "Off" };
                int align                   = (int)emitter->GetAlignedType();
                if(UIPropertyDropdown("Align", &align, kAlign, 3))
                    emitter->SetAlignedType((ParticleEmitter::AlignedType)align);

                bool sort = emitter->GetSortParticles();
                if(UIPropertyBool("Sort", &sort))
                    emitter->SetSortParticles(sort);
                bool depthWrite = emitter->GetDepthWrite();
                if(UIPropertyBool("Depth Write", &depthWrite))
                    emitter->SetDepthWrite(depthWrite);
                bool local = emitter->GetLocalSpace();
                if(UIPropertyBool("Local Space", &local))
                    emitter->SetLocalSpace(local);

                char path[256];
                if(TextureRow("Texture", (Graphics::Texture2D*)emitter->GetTexture().get(), path, sizeof(path)))
                    emitter->SetTextureFromFile(path);
            }

            void DrawDecal(Entity, void* component)
            {
                DecalComponent* decal = (DecalComponent*)component;

                char path[256];
                if(TextureRow("Texture", decal->Texture.get(), path, sizeof(path)))
                    decal->SetTexture(path);
                UIPropertyColour("Colour", &decal->Colour.x);
                UIPropertyFloat("Angle Fade", &decal->AngleFade, 0.005f, 0.06f, 1.0f, "%.2f");
                bool keepRoughness = decal->Roughness < 0.0f;
                if(UIPropertyBool("Keep Roughness", &keepRoughness))
                    decal->Roughness = keepRoughness ? -1.0f : 0.5f;
                if(!keepRoughness)
                    UIPropertyFloat("Roughness", &decal->Roughness, 0.005f, 0.0f, 1.0f, "%.2f");
                UIPropertyFloat("Emissive", &decal->Emissive, 0.01f, 0.0f, 100.0f, "%.2f");
                UIPropertyInt("Order", &decal->Order, 0.1f, -100, 100);
                UIPropertyLabel("Projects", Str8Lit("down the box's -Y, scale = half size"));
            }

            void DrawAxisConstraint(Entity entity, void* component)
            {
                AxisConstraintComponent* constraint = (AxisConstraintComponent*)component;

                u64 target = 0;
                if(EntityRow("Entity", entity.GetScene(), constraint->GetEntityID(), &target))
                    constraint->SetEntity(target);

                static const char* kAxes[] = { "X", "Y", "Z", "XY", "XZ", "YZ", "XYZ" };
                int axes                   = Maths::Clamp((int)constraint->GetAxes(), 0, 6);
                if(UIPropertyDropdown("Axes", &axes, kAxes, 7))
                    constraint->SetAxes((Axes)axes);
            }

            void DrawSpringConstraint(Entity entity, void* component)
            {
                SpringConstraintComponent* constraint = (SpringConstraintComponent*)component;

                u64 target = 0;
                if(EntityRow("Entity", entity.GetScene(), constraint->GetEntityID(), &target))
                    constraint->SetEntityID(target);
                if(EntityRow("Other Entity", entity.GetScene(), constraint->GetOtherEntityID(), &target))
                    constraint->SetOtherEntityID(target);

                float constant = constraint->GetConstant();
                if(UIPropertyFloat("Constant", &constant, 0.01f, 0.0f, 100000.0f, "%.2f"))
                    constraint->SetConstant(constant);
            }

            void DrawEnvironment(Entity, void* component)
            {
                Graphics::Environment* environment = (Graphics::Environment*)component;

                static const char* kModes[] = { "Textures", "Procedural" };
                int mode                    = environment->GetMode() == 0 ? 0 : 1;
                if(UIPropertyDropdown("Mode", &mode, kModes, 2))
                    environment->SetMode((u8)mode);

                if(mode == 0)
                {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%s", environment->GetFilePath().c_str());
                    if(UIPropertyText("File Path", buffer, sizeof(buffer)))
                        environment->SetFilePath(buffer);
                    snprintf(buffer, sizeof(buffer), "%s", environment->GetFileType().c_str());
                    if(UIPropertyText("File Type", buffer, sizeof(buffer)))
                        environment->SetFileType(buffer);

                    int width = (int)environment->GetWidth();
                    if(UIPropertyInt("Width", &width, 1.0f, 1, 8192))
                        environment->SetWidth((u32)width);
                    int height = (int)environment->GetHeight();
                    if(UIPropertyInt("Height", &height, 1.0f, 1, 8192))
                        environment->SetHeight((u32)height);
                    int mips = (int)environment->GetNumMips();
                    if(UIPropertyInt("Mips", &mips, 0.1f, 1, 16))
                        environment->SetNumMips((u32)mips);
                }
                else
                {
                    Vec4 horizon = environment->GetHorizonColour();
                    if(UIPropertyColour("Horizon", &horizon.x, false))
                        environment->SetHorizonColour(horizon);
                    Vec4 zenith = environment->GetZenithColour();
                    if(UIPropertyColour("Zenith", &zenith.x, false))
                        environment->SetZenithColour(zenith);

                    // w of each of these is a strength, the rest a direction or colour
                    Vec4 sun     = environment->GetSunDirection();
                    bool changed = UIPropertyVec3("Sun Direction", &sun.x, 0.01f);
                    changed |= UIPropertyFloat("Sun Intensity", &sun.w, 0.01f, 0.0f, 8.0f, "%.2f");
                    if(changed)
                        environment->SetSunDirection(sun);

                    Vec4 fog = environment->GetFogColour();
                    changed  = UIPropertyColour("Fog Colour", &fog.x, false);
                    changed |= UIPropertySlider("Fog Strength", &fog.w, 0.0f, 1.0f);
                    if(changed)
                        environment->SetFogColour(fog);
                    Vec4 fogParams = environment->GetFogParams();
                    changed        = UIPropertyFloat("Fog Density", &fogParams.x, 0.001f, 0.0f, 1.0f, "%.3f");
                    changed |= UIPropertyFloat("Fog Height Falloff", &fogParams.y, 0.001f, 0.0f, 1.0f, "%.3f");
                    changed |= UIPropertyFloat("Fog Start", &fogParams.z, 0.5f, 0.0f, 10000.0f, "%.1f");
                    changed |= UIPropertyFloat("Fog End", &fogParams.w, 0.5f, 0.0f, 10000.0f, "%.1f");
                    if(changed)
                        environment->SetFogParams(fogParams);

                    static const char* kClouds[] = { "Off", "Stylised", "Realistic" };
                    Vec4 clouds                  = environment->GetCloudParams();
                    int style                    = clouds.w < 0.0f ? 0 : (clouds.w < 0.5f ? 1 : 2);
                    changed                      = false;
                    if(UIPropertyDropdown("Clouds", &style, kClouds, 3))
                    {
                        clouds.w = style == 0 ? -1.0f : (style == 1 ? 0.0f : 1.0f);
                        changed  = true;
                    }
                    if(style != 0)
                    {
                        changed |= UIPropertySlider("Cloud Coverage", &clouds.x, 0.0f, 1.0f);
                        changed |= UIPropertySlider("Cloud Density", &clouds.y, 0.0f, 1.0f);
                        changed |= UIPropertyFloat("Cloud Speed", &clouds.z, 0.001f, 0.0f, 1.0f, "%.3f");

                        Vec4 cloudColour = environment->GetCloudColour();
                        bool tinted      = UIPropertyColour("Cloud Tint", &cloudColour.x, false);
                        tinted |= UIPropertyFloat("Cloud Shadow", &cloudColour.w, 0.005f, 0.0f, 1.0f, "%.2f");
                        if(tinted)
                            environment->SetCloudColour(cloudColour);

                        Vec4 wind  = environment->GetCloudWindDir();
                        Vec2 windXZ = Vec2(wind.x, wind.z);
                        bool blown = UIPropertyVec2("Wind", &windXZ.x, 0.01f);
                        blown |= UIPropertyFloat("Cloud Altitude", &wind.y, 0.005f, 0.0f, 1.0f, "%.2f");
                        blown |= UIPropertyFloat("Cloud Thickness", &wind.w, 0.005f, 0.0f, 2.0f, "%.2f");
                        if(blown)
                            environment->SetCloudWindDir(Vec4(windXZ.x, wind.y, windXZ.y, wind.w));
                    }
                    if(changed)
                        environment->SetCloudParams(clouds);

                    Vec4 stars = environment->GetStarParams();
                    changed    = UIPropertySlider("Star Density", &stars.x, 0.0f, 1.0f);
                    changed |= UIPropertyFloat("Star Brightness", &stars.y, 0.01f, 0.0f, 4.0f, "%.2f");
                    changed |= UIPropertyFloat("Star Twinkle", &stars.z, 0.01f, 0.0f, 4.0f, "%.2f");
                    if(changed)
                        environment->SetStarParams(stars);

                    Vec4 aurora = environment->GetAuroraParams();
                    changed     = UIPropertyFloat("Aurora", &aurora.x, 0.01f, 0.0f, 3.0f, "%.2f");
                    changed |= UIPropertyFloat("Aurora Height", &aurora.y, 0.005f, 0.0f, 1.0f, "%.2f");
                    changed |= UIPropertyFloat("Aurora Speed", &aurora.z, 0.01f, 0.0f, 3.0f, "%.2f");
                    if(changed)
                        environment->SetAuroraParams(aurora);
                    Vec4 auroraColour = environment->GetAuroraColour();
                    if(UIPropertyColour("Aurora Colour", &auroraColour.x, false))
                        environment->SetAuroraColour(auroraColour);
                }

                if(ActionRow("", "Reload"))
                    environment->Load();
            }

            void DrawTextureMatrix(Entity, void* component)
            {
                Mat4& matrix = ((TextureMatrixComponent*)component)->GetMatrix();
                Vec3 position, scale;
                Quat rotation;
                matrix.Decompose(position, rotation, scale);

                if(UIPropertyVec3("Position", &position.x, 0.01f))
                    Maths::SetTranslation(matrix, position);
                Vec3 euler = rotation.ToEuler();
                if(UIPropertyVec3("Rotation", &euler.x, 0.5f))
                    Maths::SetRotation(matrix, Vec3(Maths::Clamp(euler.x, -89.9f, 89.9f), euler.y, euler.z));
                if(UIPropertyVec3("Scale", &scale.x, 0.01f))
                    Maths::SetScale(matrix, scale);
            }

            void DrawCameraController(Entity, void* component)
            {
                DefaultCameraController* controller = (DefaultCameraController*)component;

                static const char* kTypes[] = { "FPS", "Third Person", "Simple", "2D", "Editor", "Custom" };
                int type                    = Maths::Clamp((int)controller->GetType(), 0, 5);
                if(UIPropertyDropdown("Type", &type, kTypes, 6))
                    controller->SetControllerType((DefaultCameraController::ControllerType)type);
            }

            void DrawTerrain(Entity entity, void* component)
            {
                TerrainComponent* terrain = (TerrainComponent*)component;

                // what Generate will build; follows the selected terrain until edited
                static int s_Grid         = 256;
                static float s_ScaleXZ    = 1.0f;
                static float s_Height     = 60.0f;
                static bool s_Collision   = true;
                static u32 s_For          = kNoEntity;
                const u32 id              = (u32)entt::to_integral(entity.GetHandle());
                if(s_For != id)
                {
                    s_For = id;
                    if(terrain->GridW > 1)
                        s_Grid = terrain->GridW;
                    if(terrain->ScaleXZ > 0.0f)
                        s_ScaleXZ = terrain->ScaleXZ;
                    if(terrain->ScaleY > 0.0f)
                        s_Height = terrain->ScaleY;
                }

                const bool built = terrain->GridW > 1;
                char size[48];
                snprintf(size, sizeof(size), built ? "%d x %d" : "not built", terrain->GridW, terrain->GridW);
                UIPropertyLabel("Grid", Str8C(size));
                if(!terrain->HeightmapPath.empty())
                    UIPropertyLabel("Heightmap", Str8C((char*)terrain->HeightmapPath.c_str()));

                UIPropertyInt("Grid Size", &s_Grid, 1.0f, 8, 1024);
                UIPropertyFloat("Cell Size", &s_ScaleXZ, 0.05f, 0.05f, 100.0f, "%.2f");
                UIPropertyFloat("Height", &s_Height, 0.5f, 0.0f, 1000.0f, "%.1f");
                UIPropertyBool("Collision", &s_Collision);

                if(ActionRow("", built ? "Regenerate" : "Generate"))
                {
                    EntityFactory::TerrainMaterial material;
                    material.noCollider = !s_Collision;
                    EntityFactory::BuildTerrainOnEntity(entity, s_Grid, s_ScaleXZ, s_Height, material);
                }
            }

            void DrawVoxelWorld(Entity, void* component)
            {
                VoxelWorldComponent* voxels = (VoxelWorldComponent*)component;
                if(UIPropertyInt("View Radius", &voxels->ViewRadius, 0.1f, 1, 16) && voxels->World)
                    voxels->World->SetViewRadius(voxels->ViewRadius);
                // streams back in from the noise next frame
                if(voxels->World && ActionRow("", "Regenerate"))
                    voxels->World->Clear();
            }

            void DrawText(Entity, void* component)
            {
                TextComponent* text = (TextComponent*)component;

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s", text->TextString.c_str());
                if(UIPropertyText("Text", buffer, sizeof(buffer)))
                    text->TextString = buffer;
                UIPropertyColour("Colour", &text->Colour.x);
                UIPropertyFloat("Max Width", &text->MaxWidth, 0.05f, 0.0f, 100000.0f, "%.2f");
                UIPropertyFloat("Line Spacing", &text->LineSpacing, 0.01f, -10.0f, 10.0f, "%.2f");
                UIPropertyFloat("Kerning", &text->Kerning, 0.005f, -2.0f, 2.0f, "%.3f");
                UIPropertySlider("Outline", &text->OutlineWidth, 0.0f, 1.0f);
                if(text->OutlineWidth > 0.0f)
                    UIPropertyColour("Outline Colour", &text->OutlineColour.x);
                if(text->FontHandle)
                {
                    const char* font = strrchr(text->FontHandle->GetFilePath().c_str(), '/');
                    UIPropertyLabel("Font", Str8C((char*)(font ? font + 1 : text->FontHandle->GetFilePath().c_str())));
                }
                else
                    UIPropertyLabel("Font", Str8Lit("default"));
            }

            const ComponentDesc* FindComponent(u32 typeId)
            {
                for(u32 i = 0; i < s_ComponentCount; i++)
                    if(s_Components[i].TypeId == typeId)
                        return &s_Components[i];
                return nullptr;
            }

            void OnEntityReplaced(u32 oldHandle, u32 newHandle)
            {
                if(oldHandle != kNoEntity && GetSelected() == oldHandle)
                    Select(newHandle, false);
            }
        }

        // ---- selection -----------------------------------------------------

        void SetSelectionHooks(const SelectionHooks& hooks)
        {
            s_Hooks = hooks;
        }

        u32 GetSelected()
        {
            return s_Hooks.Get ? s_Hooks.Get() : DebugMenu::Get().GetSelectedEntity();
        }

        void Select(u32 entity, bool additive)
        {
            if(s_Hooks.Set)
            {
                s_Hooks.Set(entity, additive);
                return;
            }
            if(entity == kNoEntity)
                DebugMenu::Get().ClearSelection();
            else
                DebugMenu::Get().SelectEntity(entity);
        }

        bool IsSelected(u32 entity)
        {
            return s_Hooks.IsSelected ? s_Hooks.IsSelected(entity) : (entity != kNoEntity && GetSelected() == entity);
        }

        // ---- components ----------------------------------------------------

        void RegisterComponent(const ComponentDesc& desc)
        {
            for(u32 i = 0; i < s_ComponentCount; i++)
            {
                if(s_Components[i].TypeId == desc.TypeId)
                {
                    s_Components[i] = desc;
                    return;
                }
            }
            if(s_ComponentCount < kMaxComponents)
                s_Components[s_ComponentCount++] = desc;
        }

        void Init()
        {
            if(s_Initialised)
                return;
            s_Initialised = true;

            RegisterComponent<Maths::Transform>("Transform", &DrawTransform, true);
            RegisterComponent<Camera>("Camera", &DrawCamera);
            RegisterComponent<Graphics::Light>("Light", &DrawLight, true);
            RegisterComponent<Graphics::Sprite>("Sprite", &DrawSprite);
            RegisterComponent<Graphics::AnimatedSprite>("Animated Sprite", &DrawAnimatedSprite);
            RegisterComponent<RigidBody2DComponent>("RigidBody 2D", &DrawRigidBody2D, false, true);
            RegisterComponent<RigidBody3DComponent>("RigidBody 3D", &DrawRigidBody3D, false, true);
            RegisterComponent<Graphics::ModelComponent>("Model", &DrawModel, false, true);
            RegisterComponent<TextComponent>("Text", &DrawText);
            RegisterComponent<Graphics::Light2D>("Light 2D", &DrawLight2D, true);
            RegisterComponent<SoundComponent>("Sound", &DrawSound);
            RegisterComponent<ParticleEmitter>("Particle Emitter", &DrawParticles);
            RegisterComponent<AxisConstraintComponent>("Axis Constraint", &DrawAxisConstraint, false, true);
            RegisterComponent<SpringConstraintComponent>("Spring Constraint", &DrawSpringConstraint, false, true);
            RegisterComponent<Graphics::Environment>("Environment", &DrawEnvironment);
            RegisterComponent<TextureMatrixComponent>("Texture Matrix", &DrawTextureMatrix);
            RegisterComponent<DecalComponent>("Decal", &DrawDecal);
            RegisterComponent<DefaultCameraController>("Camera Controller", &DrawCameraController);
            RegisterComponent<TerrainComponent>("Terrain", &DrawTerrain, false, true);
            RegisterComponent<VoxelWorldComponent>("Voxel World", &DrawVoxelWorld, false, true);
            RegisterComponent<Listener>("Listener", nullptr);

            // ones that need more than a default value to be of any use when added
            for(u32 i = 0; i < s_ComponentCount; i++)
            {
                ComponentDesc& desc = s_Components[i];
                if(desc.TypeId == (u32)entt::type_hash<Graphics::ModelComponent>::value())
                    desc.Add = [](Entity e)
                    {
                        if(!e.HasComponent<Graphics::ModelComponent>())
                            e.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);
                    };
                else if(desc.TypeId == (u32)entt::type_hash<RigidBody3DComponent>::value())
                    desc.Add = [](Entity e)
                    {
                        e.GetOrAddComponent<Maths::Transform>();
                        Maths::Transform& trans    = e.GetOrAddComponent<Maths::Transform>();
                        RigidBody3DComponent& body = e.GetOrAddComponent<RigidBody3DComponent>();
                        RigidBody3D* rb            = body.GetRigidBody();
                        if(!rb)
                            return;
                        // a new body sits at the origin and the physics sync would pull the entity there
                        rb->SetPosition(trans.GetWorldPosition());
                        rb->SetOrientation(trans.GetWorldOrientation());
                        if(!rb->GetCollisionShape())
                        {
                            rb->SetCollisionShape(CollisionShapeType::CollisionCuboid);
                            FitShapeToModel(e, rb);
                        }
                    };
            }
            // lives with the Lua code: the component cannot be touched without sol
            RegisterLuaScriptInspector();

            SceneUndo::SetEntityReplacedCallback(&OnEntityReplaced);
        }

        // ---- hierarchy -----------------------------------------------------

        void DrawHierarchy(const UIDockPanelContext& context)
        {
            UI_State* ui    = GetUIState();
            const f32 dpi   = context.DPIScale;
            Scene* scene    = CurrentScene();

            // toolbar: filter + add
            UIBeginRowFullWidth();
            {
                UI_Widget* row                       = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;

                UI_Interaction field = UITextField("##hierarchyfilter", s_HierarchyFilter, sizeof(s_HierarchyFilter), "Filter", 0.0f);
                if(field.widget)
                    field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);

                if(UIIconButton(ICON_MDI_PLUS "###hierarchyadd", 24.0f).clicked)
                    UIOpenPopup("##hierarchyadd");
                if(UIBeginPopup("##hierarchyadd"))
                {
                    if(UIMenuItem("Empty Entity") && scene)
                    {
                        Entity created = scene->CreateEntity("Entity");
                        SceneUndo::EntityCreated(created);
                        Select((u32)entt::to_integral(created.GetHandle()), false);
                    }
                    if(UIMenuItem("Child of Selected", nullptr, nullptr, scene && GetSelected() != kNoEntity) && scene)
                    {
                        Entity parent((entt::entity)GetSelected(), scene);
                        if(parent.Valid())
                        {
                            Entity created = scene->CreateEntity("Entity");
                            created.SetParent(parent);
                            SetExpanded(parent.GetHandle(), true);
                            SceneUndo::EntityCreated(created);
                            Select((u32)entt::to_integral(created.GetHandle()), false);
                        }
                    }
                    UIEndPopup();
                }
            }
            UIEndRow();

            if(!scene)
            {
                DimLabel("##hierarchynoscene", "No scene");
                return;
            }

            entt::registry& registry = scene->GetRegistry();
            const f32 rowLogical     = floorf(24.0f * Maths::Clamp(UIGetToolTheme().Density, 0.6f, 1.8f));
            const f32 rowHeight      = rowLogical * dpi;
            const f32 footer         = 22.0f * dpi;
            const f32 toolbar        = 36.0f * dpi;
            const f32 listHeight     = Maths::Max(context.Size.y - toolbar - footer, rowHeight * 2.0f);

            HierarchyFrame frame;
            frame.scene         = scene;
            frame.registry      = &registry;
            frame.index         = 0;
            frame.first         = Maths::Max(0, (i32)floorf(s_HierarchyScroll / rowHeight) - 1);
            frame.last          = frame.first + (i32)ceilf(listHeight / rowHeight) + 3;
            frame.dpi           = dpi;
            frame.rowLogical    = rowLogical;
            frame.reparentChild = kNoEntity;
            frame.reparentTo    = kNoEntity;
            frame.renameTo      = kNoEntity;

            BeginListArea("##hierarchylist", listHeight, &s_HierarchyScroll);
            ListSpacer(0x48544F50ull, (f32)frame.first * rowHeight);

            auto& storage = registry.storage<entt::entity>();
            if(s_HierarchyFilter[0])
            {
                // filtering flattens the tree: every match, whatever its parent
                char name[128];
                for(auto [e] : storage.each())
                {
                    EntityName(registry, e, name, sizeof(name));
                    if(!ContainsNoCase(name, s_HierarchyFilter))
                        continue;
                    if(frame.index >= frame.first && frame.index < frame.last)
                        HierarchyRow(frame, e, 0, false);
                    frame.index++;
                }
            }
            else
            {
                for(auto [e] : storage.each())
                {
                    const Hierarchy* hierarchy = registry.try_get<Hierarchy>(e);
                    const bool isRoot          = !hierarchy || hierarchy->Parent() == entt::null || !registry.valid(hierarchy->Parent());
                    if(isRoot)
                        HierarchyWalk(frame, e, 0);
                }
            }

            const i32 total = frame.index;
            ListSpacer(0x48424F54ull, (f32)Maths::Max(0, total - frame.last) * rowHeight);

            // the space under the last row: drop here to unparent, click to deselect
            {
                const f32 used     = (f32)total * rowHeight;
                UI_Widget* filler  = PushWidget(WidgetFlags_Clickable, Str8Lit(""), 0x4846494Cull,
                                                { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, Maths::Max(listHeight - used, 12.0f * dpi) });
                UIClearBorders(filler);
                UISetBackground(filler, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
                UI_Interaction interaction = HandleWidgetInteraction(filler);
                if(interaction.clicked)
                    Select(kNoEntity, false);
                if(interaction.right_clicked)
                {
                    s_ContextEntity = kNoEntity;
                    UIOpenPopupAt("##hierarchycontext", UIMousePosition());
                }
                u32 dropped = kNoEntity;
                if(UIDropTarget("lumos_entity", &dropped, sizeof(dropped)))
                {
                    frame.reparentChild = dropped;
                    frame.reparentTo    = kNoEntity;
                }
            }
            UIEndScrollArea();

            char count[48];
            snprintf(count, sizeof(count), "  %d shown / %u entities", total, (u32)storage.in_use());
            DimLabel("##hierarchycount", count);

            // ---- things asked for while the rows were being built ----
            u32 deleteEntity    = kNoEntity;
            u32 duplicateEntity = kNoEntity;

            u32 copyEntity   = kNoEntity;
            bool cutEntity   = false;
            bool pasteEntity = false;
            u32 pasteParent  = kNoEntity;

            if(UIBeginPopup("##hierarchycontext"))
            {
                const bool hasTarget = s_ContextEntity != kNoEntity && registry.valid((entt::entity)s_ContextEntity);

                // new things go under the row that was clicked
                auto create = [&](const char* name)
                {
                    Entity created = scene->CreateEntity(name);
                    created.GetOrAddComponent<Maths::Transform>();
                    if(hasTarget)
                    {
                        created.SetParent(Entity((entt::entity)s_ContextEntity, scene));
                        SetExpanded((entt::entity)s_ContextEntity, true);
                    }
                    return created;
                };
                auto created = [&](Entity entity)
                {
                    SceneUndo::EntityCreated(entity);
                    Select((u32)entt::to_integral(entity.GetHandle()), false);
                };

                if(UIBeginMenu(ICON_MDI_PLUS "  Create"))
                {
                    if(UIMenuItem("Empty"))
                        created(create("Entity"));
                    UIMenuSeparator();
                    static const struct
                    {
                        const char* Name;
                        Graphics::PrimitiveType Type;
                    } kPrimitives[] = {
                        { "Cube", Graphics::PrimitiveType::Cube },
                        { "Sphere", Graphics::PrimitiveType::Sphere },
                        { "Capsule", Graphics::PrimitiveType::Capsule },
                        { "Cylinder", Graphics::PrimitiveType::Cylinder },
                        { "Pyramid", Graphics::PrimitiveType::Pyramid },
                        { "Plane", Graphics::PrimitiveType::Plane },
                    };
                    for(const auto& primitive : kPrimitives)
                    {
                        if(UIMenuItem(primitive.Name))
                        {
                            Entity entity = create(primitive.Name);
                            entity.AddComponent<Graphics::ModelComponent>(primitive.Type);
                            created(entity);
                        }
                    }
                    UIMenuSeparator();
                    if(UIMenuItem("Light"))
                    {
                        Entity entity = create("Light");
                        entity.AddComponent<Graphics::Light>();
                        created(entity);
                    }
                    if(UIMenuItem("Camera"))
                    {
                        Entity entity = create("Camera");
                        entity.AddComponent<Camera>();
                        created(entity);
                    }
                    if(UIMenuItem("Sprite"))
                    {
                        Entity entity = create("Sprite");
                        entity.AddComponent<Graphics::Sprite>();
                        created(entity);
                    }
                    if(UIMenuItem("Text"))
                    {
                        Entity entity = create("Text");
                        entity.AddComponent<TextComponent>();
                        created(entity);
                    }
                    UIEndMenu();
                }
                UIMenuSeparator();
                if(UIMenuItem("Rename", "F2", nullptr, hasTarget))
                    s_RenameRequest = s_ContextEntity;
                if(UIMenuItem("Focus", "F", nullptr, hasTarget && s_Hooks.Focus))
                    s_Hooks.Focus(s_ContextEntity);
                UIMenuSeparator();
                if(UIMenuItem("Copy", "Ctrl C", nullptr, hasTarget))
                    copyEntity = s_ContextEntity;
                if(UIMenuItem("Cut", "Ctrl X", nullptr, hasTarget))
                {
                    copyEntity = s_ContextEntity;
                    cutEntity  = true;
                }
                if(UIMenuItem("Paste", "Ctrl V", nullptr, s_EntityClipboard.size > 0))
                {
                    pasteEntity = true;
                    pasteParent = hasTarget ? s_ContextEntity : kNoEntity;
                }
                if(UIMenuItem("Duplicate", "Ctrl D", nullptr, hasTarget))
                    duplicateEntity = s_ContextEntity;
                if(UIMenuItem("Unparent", nullptr, nullptr, hasTarget && Entity((entt::entity)s_ContextEntity, scene).GetParent().Valid()))
                {
                    frame.reparentChild = s_ContextEntity;
                    frame.reparentTo    = kNoEntity;
                }
                UIMenuSeparator();
                if(UIMenuItem("Delete", "Del", nullptr, hasTarget))
                    deleteEntity = s_ContextEntity;
                UIEndPopup();
            }

            // the same from the keyboard, on the selection
            if(context.Hovered && ui->FocusedTextInput == 0)
            {
                Input& input       = Input::Get();
                const u32 selected = GetSelected();
                const bool has     = selected != kNoEntity && registry.valid((entt::entity)selected);
                if(has && input.GetKeyPressedRaw(InputCode::Key::F2))
                    s_RenameRequest = selected;
                if(ControlHeld())
                {
                    if(has && input.GetKeyPressedRaw(InputCode::Key::C))
                        copyEntity = selected;
                    if(has && input.GetKeyPressedRaw(InputCode::Key::X))
                    {
                        copyEntity = selected;
                        cutEntity  = true;
                    }
                    if(has && input.GetKeyPressedRaw(InputCode::Key::D))
                        duplicateEntity = selected;
                    if(s_EntityClipboard.size > 0 && input.GetKeyPressedRaw(InputCode::Key::V))
                    {
                        pasteEntity = true;
                        pasteParent = has ? selected : kNoEntity;
                    }
                }
            }

            if(s_RenameRequest != kNoEntity && registry.valid((entt::entity)s_RenameRequest))
            {
                Select(s_RenameRequest, false);
                s_RenameEntity   = s_RenameRequest;
                s_RenameHadFocus = false;
                s_RenameClaimed  = false;
                snprintf(s_RenameBuffer, sizeof(s_RenameBuffer), "%s", Entity((entt::entity)s_RenameRequest, scene).GetName().c_str());
            }
            s_RenameRequest = kNoEntity;

            if(frame.renameTo != kNoEntity && registry.valid((entt::entity)frame.renameTo))
                SceneUndo::SetName(Entity((entt::entity)frame.renameTo, scene), s_RenameBuffer);

            if(copyEntity != kNoEntity && registry.valid((entt::entity)copyEntity))
            {
                if(!s_ClipboardArena)
                    s_ClipboardArena = ArenaAlloc(Megabytes(8));
                ArenaClear(s_ClipboardArena);
                s_EntityClipboard = scene->SerialiseEntityToString(s_ClipboardArena, Entity((entt::entity)copyEntity, scene));
                if(cutEntity)
                    deleteEntity = copyEntity;
            }

            if(pasteEntity && s_EntityClipboard.size > 0)
            {
                Entity pasted = scene->DeserialiseEntityFromString(s_EntityClipboard);
                if(pasted.Valid())
                {
                    // a copy, not the same thing twice (a cut one keeps its ids: the original is going)
                    if(!cutEntity)
                    {
                        RenewIds(registry, pasted.GetHandle());
                    }
                    if(pasteParent != kNoEntity && registry.valid((entt::entity)pasteParent) && pasteParent != deleteEntity)
                    {
                        pasted.SetParent(Entity((entt::entity)pasteParent, scene));
                        SetExpanded((entt::entity)pasteParent, true);
                    }
                    SceneUndo::EntityCreated(pasted);
                    Select((u32)entt::to_integral(pasted.GetHandle()), false);
                }
            }

            if(context.Hovered && ui->FocusedTextInput == 0 && GetSelected() != kNoEntity
               && (Input::Get().GetKeyPressedRaw(InputCode::Key::Delete) || Input::Get().GetKeyPressedRaw(InputCode::Key::Backspace)))
                deleteEntity = GetSelected();

            if(frame.reparentChild != kNoEntity && registry.valid((entt::entity)frame.reparentChild))
            {
                Entity child((entt::entity)frame.reparentChild, scene);
                Entity parent = frame.reparentTo != kNoEntity && registry.valid((entt::entity)frame.reparentTo)
                    ? Entity((entt::entity)frame.reparentTo, scene)
                    : Entity(entt::null, scene);
                SceneUndo::SetParent(child, parent);
                if(parent.Valid())
                    SetExpanded(parent.GetHandle(), true);
            }

            if(duplicateEntity != kNoEntity && registry.valid((entt::entity)duplicateEntity))
                scene->DuplicateEntity(Entity((entt::entity)duplicateEntity, scene));

            if(deleteEntity != kNoEntity && registry.valid((entt::entity)deleteEntity))
            {
                if(IsSelected(deleteEntity))
                    Select(kNoEntity, false);
                SceneUndo::DestroyEntity(Entity((entt::entity)deleteEntity, scene));
            }
        }

        // ---- inspector -----------------------------------------------------

        void DrawInspector(const UIDockPanelContext& context)
        {
            Init();

            UI_State* ui    = GetUIState();
            const f32 dpi   = context.DPIScale;
            Scene* scene    = CurrentScene();
            const u32 id    = GetSelected();

            if(!scene || id == kNoEntity || !scene->GetRegistry().valid((entt::entity)id))
            {
                DimLabel("##inspectornone", "Nothing selected");
                return;
            }

            entt::registry& registry = scene->GetRegistry();
            Entity entity((entt::entity)id, scene);
            const u64 uuid = entity.GetID();

            // ---- name / active ----
            {
                NameComponent& nameComponent = entity.GetOrAddComponent<NameComponent>();

                if(s_NameEditFor != id || (!s_NameWasFocused && nameComponent.name != s_NameEdit))
                {
                    snprintf(s_NameEdit, sizeof(s_NameEdit), "%s", nameComponent.name.c_str());
                    snprintf(s_NameOriginal, sizeof(s_NameOriginal), "%s", nameComponent.name.c_str());
                    s_NameEditFor    = id;
                    s_NameWasFocused = false;
                }

                UIBeginPropertyGrid("##inspectorheader");
                const bool edited = UIPropertyText("Name", s_NameEdit, sizeof(s_NameEdit));
                // ours when the focused field is typing into this buffer
                const bool focusedNow = ui->FocusedTextInput != 0 && ui->TextInputBuffer == s_NameEdit;

                if(edited)
                    nameComponent.name = s_NameEdit; // live, so the tree follows the typing

                // typing finished: one undo step from what it was to what it is
                if(s_NameWasFocused && ui->FocusedTextInput == 0 && strcmp(s_NameOriginal, s_NameEdit) != 0)
                {
                    nameComponent.name = s_NameOriginal;
                    SceneUndo::SetName(entity, s_NameEdit);
                    snprintf(s_NameOriginal, sizeof(s_NameOriginal), "%s", s_NameEdit);
                }
                s_NameWasFocused = focusedNow && ui->FocusedTextInput != 0;

                bool active = entity.Active();
                if(UIPropertyBool("Active", &active))
                    entity.SetActive(active);

                char idText[48];
                snprintf(idText, sizeof(idText), "%u   uuid %llu", id, (unsigned long long)uuid);
                UIPropertyLabel("ID", Str8C(idText));
                UIEndPropertyGrid();
            }

            // ---- components ----
            const ComponentDesc* removeDesc = nullptr;
            for(u32 i = 0; i < s_ComponentCount; i++)
            {
                const ComponentDesc& desc = s_Components[i];
                if(!desc.Has || !desc.Has(entity))
                    continue;

                UISpacer(4.0f);
                char headerId[96];
                snprintf(headerId, sizeof(headerId), "%s###component%u", desc.Name, desc.TypeId);
                bool removeClicked = false;
                const bool open    = UISectionHeader(headerId, true, desc.CanRemove ? &removeClicked : nullptr);
                if(removeClicked)
                    removeDesc = &desc;
                if(!open || !desc.Draw)
                    continue;

                void* component = desc.Get ? desc.Get(entity) : nullptr;
                if(!component)
                    continue;

                // every widget in the section edits this component: one scope covers their undo
                if(desc.PlainData)
                    UIPushUndoTarget(component, desc.Size, uuid, desc.TypeId);
                else
                    UIPushUndoCallbacks(&SceneUndo::BeginEntityEditByID, &SceneUndo::EndEntityEditByID, uuid);

                char gridId[48];
                snprintf(gridId, sizeof(gridId), "##grid%u", desc.TypeId);
                UIBeginPropertyGrid(gridId);
                desc.Draw(entity, component);
                UIEndPropertyGrid();

                UIPopUndoTarget();
            }

            // ---- add ----
            UISpacer(8.0f);
            UIBeginRowFullWidth();
            UIFlexSpacer();
            if(UIButton("Add Component###inspectoradd").clicked)
                UIOpenPopup("##inspectoraddmenu");
            UIFlexSpacer();
            UIEndRow();

            const ComponentDesc* addDesc = nullptr;
            static char s_AddFilter[48]  = { 0 };
            static bool s_AddWasOpen     = false;
            const bool addOpen           = UIBeginPopup("##inspectoraddmenu");
            if(addOpen)
            {
                // type to narrow the list; the field has the keyboard as soon as the list opens
                if(!s_AddWasOpen)
                {
                    s_AddFilter[0] = 0;
                    UIFocusNextTextInput();
                }
                UI_Interaction field = UITextField("###inspectoraddfilter", s_AddFilter, sizeof(s_AddFilter), "Search components", 220.0f);
                if(field.widget)
                    field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
                UIMenuSeparator();

                u32 listed                    = 0;
                const ComponentDesc* onlyOne = nullptr;
                for(u32 i = 0; i < s_ComponentCount; i++)
                {
                    const ComponentDesc& desc = s_Components[i];
                    if(!desc.Add || !ContainsNoCase(desc.Name, s_AddFilter))
                        continue;
                    // what it already has is shown, greyed: the list is the same every time
                    const bool has = desc.Has && desc.Has(entity);
                    char itemId[96];
                    snprintf(itemId, sizeof(itemId), "%s%s###add%u", desc.Name, has ? "   (added)" : "", desc.TypeId);
                    if(UIMenuItem(itemId, nullptr, nullptr, !has))
                        addDesc = &desc;
                    if(!has)
                    {
                        listed++;
                        onlyOne = &desc;
                    }
                }
                if(listed == 0)
                    UIMenuItem("Nothing matches###addnone", nullptr, nullptr, false);
                // Enter takes the match when the search has got it down to one
                else if(listed == 1 && s_AddFilter[0] && Input::Get().GetKeyPressedRaw(InputCode::Key::Enter))
                {
                    addDesc = onlyOne;
                    UICloseAllPopups();
                }
                UIEndPopup();
            }
            s_AddWasOpen = addOpen;

            // adding / removing changes the entity's shape: recorded as a whole-entity edit
            if(addDesc)
            {
                SceneUndo::BeginEntityEdit(entity);
                addDesc->Add(entity);
                SceneUndo::EndEntityEdit(entity, "Add Component");
            }
            if(removeDesc && removeDesc->Remove)
            {
                SceneUndo::BeginEntityEdit(entity);
                removeDesc->Remove(entity);
                SceneUndo::EndEntityEdit(entity, "Remove Component");
            }

            // a texture slot's right-click menu
            if(UIBeginPopup("##textureslotmenu"))
            {
                const char* name = strrchr(s_TextureMenuPath, '/');
                UIMenuItem(name ? name + 1 : s_TextureMenuPath, nullptr, nullptr, false);
                UIMenuSeparator();
                if(UIMenuItem(ICON_MDI_PENCIL "  Edit in Sprite Editor"))
                {
                    SpriteEditor::SetShapeMode(false);
                    SpriteEditor::Open(s_TextureMenuPath);
                }
                if(UIMenuItem(ICON_MDI_VECTOR_POLYGON "  Collision Shape") && SpriteEditor::Open(s_TextureMenuPath))
                    SpriteEditor::SetShapeMode(true);
                UIEndPopup();
            }
            (void)registry;
        }

        // ---- console -------------------------------------------------------

        void DrawConsole(const UIDockPanelContext& context)
        {
            UI_State* ui  = GetUIState();
            const f32 dpi = context.DPIScale;

            bool copyRequest = false;

            UIBeginRowFullWidth();
            {
                UI_Widget* row                        = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;

                if(UIIconButton(ICON_MDI_INFORMATION_OUTLINE "###consoleinfo", 24.0f, s_ConsoleLevels[0]).clicked)
                    s_ConsoleLevels[0] = !s_ConsoleLevels[0];
                UIPushStyle(StyleVar_TextColor, UIGetToolTheme().Warning);
                if(UIIconButton(ICON_MDI_ALERT_OUTLINE "###consolewarn", 24.0f, s_ConsoleLevels[1]).clicked)
                    s_ConsoleLevels[1] = !s_ConsoleLevels[1];
                UIPopStyle(StyleVar_TextColor);
                UIPushStyle(StyleVar_TextColor, UIGetToolTheme().Error);
                if(UIIconButton(ICON_MDI_ALERT_CIRCLE_OUTLINE "###consoleerror", 24.0f, s_ConsoleLevels[2]).clicked)
                    s_ConsoleLevels[2] = !s_ConsoleLevels[2];
                UIPopStyle(StyleVar_TextColor);

                UI_Interaction field = UITextField("##consolefilter", s_ConsoleFilter, sizeof(s_ConsoleFilter), "Filter", 0.0f);
                if(field.widget)
                    field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);

                if(UIIconButton(ICON_MDI_ARROW_DOWN "###consolefollow", 24.0f, s_ConsoleFollow).clicked)
                    s_ConsoleFollow = !s_ConsoleFollow;
                UITooltip("Follow new lines");
                copyRequest = UIIconButton(ICON_MDI_CONTENT_COPY "###consolecopy", 24.0f).clicked;
                UITooltip("Copy the selected lines (all of them with none selected)");
                if(UIIconButton(ICON_MDI_DELETE_OUTLINE "###consoleclear", 24.0f).clicked)
                {
                    Debug::Log::ClearRecent();
                    s_ConsoleSelA = s_ConsoleSelB = ~0ull;
                }
                UITooltip("Clear");
            }
            UIEndRow();

            const f32 rowHeight  = 18.0f * dpi;
            const f32 toolbar    = 36.0f * dpi;
            const f32 listHeight = Maths::Max(context.Size.y - toolbar, rowHeight * 2.0f);

            const bool hasSelection = s_ConsoleSelA != ~0ull;
            const u64 selLow        = Maths::Min(s_ConsoleSelA, s_ConsoleSelB);
            const u64 selHigh       = Maths::Max(s_ConsoleSelA, s_ConsoleSelB);

            auto shown = [&](const Debug::LogRecord& record)
            {
                const int group = record.level >= LogLevel::Error ? 2 : (record.level == LogLevel::Warning ? 1 : 0);
                if(!s_ConsoleLevels[group])
                    return false;
                return record.message && ContainsNoCase(record.message, s_ConsoleFilter);
            };

            // count first, so the newest lines can be kept in view
            const u32 count = Debug::Log::RecentCount();
            i32 total       = 0;
            for(u32 i = 0; i < count; i++)
                if(shown(Debug::Log::GetRecent(i)))
                    total++;

            const f32 maxScroll = Maths::Max(0.0f, (f32)total * rowHeight - listHeight);
            if(s_ConsoleFollow)
                s_ConsoleScroll = maxScroll;

            const i32 first = Maths::Max(0, (i32)floorf(s_ConsoleScroll / rowHeight) - 1);
            const i32 last  = first + (i32)ceilf(listHeight / rowHeight) + 3;

            const f32 before = s_ConsoleScroll;
            BeginListArea("##consolelist", listHeight, &s_ConsoleScroll);
            ListSpacer(0x43544F50ull, (f32)first * rowHeight);

            UIPushStyle(StyleVar_FontSize, 12.0f * dpi);
            i32 index = 0;
            for(u32 i = 0; i < count && index < last; i++)
            {
                const Debug::LogRecord record = Debug::Log::GetRecent(i);
                if(!shown(record))
                    continue;
                if(index >= first)
                {
                    const UIToolTheme& theme = UIGetToolTheme();
                    Vec4 colour              = theme.Text;
                    if(record.level >= LogLevel::Error)
                        colour = theme.Error;
                    else if(record.level == LogLevel::Warning)
                        colour = theme.Warning;
                    else if(record.level == LogLevel::Trace)
                        colour = theme.TextDim;

                    char rowId[40];
                    snprintf(rowId, sizeof(rowId), "###log%llu", (unsigned long long)record.index);
                    u64 hash;
                    HandleUIString(rowId, &hash);
                    const bool selected = hasSelection && record.index >= selLow && record.index <= selHigh;
                    // the row is the highlight; the text is a child so it sits in the middle of it
                    UI_Widget* line     = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""), hash,
                                                     { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, rowHeight });
                    line->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                    line->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
                    UIClearBorders(line);
                    UISetBackground(line, selected ? theme.Selection : Vec4(0.0f), selected ? theme.Selection : theme.Hover, theme.Selection);
                    PushParent(line);
                    UI_Widget* text = PushWidget(WidgetFlags_DrawText, Str8C((char*)record.message), 0x4C4F4754ull,
                                                 { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                    text->style_vars[StyleVar_Padding] = Vec4(0.0f);
                    UISetTextColour(text, colour);
                    PopParent(line);

                    UI_Interaction row = HandleWidgetInteraction(line);
                    if(row.clicked)
                    {
                        const bool shift = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift);
                        if(!shift || !hasSelection)
                            s_ConsoleSelA = record.index;
                        s_ConsoleSelB   = record.index;
                        s_ConsoleFollow = false;
                    }
                    if(row.right_clicked)
                    {
                        if(!selected)
                            s_ConsoleSelA = s_ConsoleSelB = record.index;
                        UIOpenPopupAt("##consolecontext", UIMousePosition());
                    }
                }
                index++;
            }
            UIPopStyle(StyleVar_FontSize);

            ListSpacer(0x43424F54ull, (f32)Maths::Max(0, total - last) * rowHeight);
            UIEndScrollArea();

            // scrolled up by hand: stop following until asked again
            if(s_ConsoleFollow && s_ConsoleScroll < before - 1.0f)
                s_ConsoleFollow = false;

            bool copyAll = false;
            if(UIBeginPopup("##consolecontext"))
            {
                if(UIMenuItem("Copy", "Ctrl C"))
                    copyRequest = true;
                if(UIMenuItem("Copy All"))
                    copyAll = true;
                UIMenuSeparator();
                if(UIMenuItem("Clear Selection", nullptr, nullptr, hasSelection))
                    s_ConsoleSelA = s_ConsoleSelB = ~0ull;
                UIEndPopup();
            }

            if(context.Hovered && ui->FocusedTextInput == 0 && ControlHeld() && Input::Get().GetKeyPressedRaw(InputCode::Key::C))
                copyRequest = true;
            if(context.Hovered && ui->FocusedTextInput == 0 && ControlHeld() && Input::Get().GetKeyPressedRaw(InputCode::Key::A) && count > 0)
            {
                s_ConsoleSelA = Debug::Log::GetRecent(0).index;
                s_ConsoleSelB = Debug::Log::GetRecent(count - 1).index;
            }

            if(copyRequest || copyAll)
            {
                const bool all    = copyAll || !hasSelection;
                ArenaTemp scratch = ScratchBegin(nullptr, 0);
                u64 bytes         = 1;
                for(u32 i = 0; i < count; i++)
                {
                    const Debug::LogRecord record = Debug::Log::GetRecent(i);
                    if(shown(record) && (all || (record.index >= selLow && record.index <= selHigh)))
                        bytes += strlen(record.message) + 1;
                }
                char* text = PushArrayNoZero(scratch.arena, char, bytes);
                u64 at     = 0;
                for(u32 i = 0; i < count; i++)
                {
                    const Debug::LogRecord record = Debug::Log::GetRecent(i);
                    if(!shown(record) || !(all || (record.index >= selLow && record.index <= selHigh)))
                        continue;
                    const u64 n = strlen(record.message);
                    if(at + n + 2 > bytes)
                        break; // the log grew underneath
                    memcpy(text + at, record.message, n);
                    at += n;
                    text[at++] = '\n';
                }
                text[at] = 0;
                Input::Get().SetClipboard(text);
                ScratchEnd(scratch);
            }
        }

        // ---- stats ---------------------------------------------------------

        void DrawStats(const UIDockPanelContext& context)
        {
            UI_State* ui      = GetUIState();
            const auto& stats = Engine::Get().Statistics();
            const f32 ms      = (f32)Engine::GetTimeStep().GetRawMillis();

            if(s_FrameStamp != ui->FrameIndex)
            {
                s_FrameStamp                = ui->FrameIndex;
                s_FrameTimes[s_FrameCursor] = ms;
                s_FrameCursor               = (s_FrameCursor + 1) % 120;
            }

            // oldest first for the plot
            f32 ordered[120];
            f32 worst = 0.0f;
            for(u32 i = 0; i < 120; i++)
            {
                ordered[i] = s_FrameTimes[(s_FrameCursor + i) % 120];
                worst      = Maths::Max(worst, ordered[i]);
            }

            const f32 width = Maths::Max(60.0f, context.Size.x / Maths::Max(context.DPIScale, 0.1f));
            UIPlotHistogram("##statsframes", ordered, 120, 0.0f, Maths::Max(33.3f, worst), width, 48.0f,
                            UIGetToolTheme().Accent, UIGetToolTheme().Field, 16.6f);

            char text[64];
            UIBeginPropertyGrid("##statsgrid", 0.5f);
            snprintf(text, sizeof(text), "%u", stats.FramesPerSecond);
            UIPropertyLabel("FPS", Str8C(text));
            snprintf(text, sizeof(text), "%.2f ms", ms);
            UIPropertyLabel("Frame", Str8C(text));
            snprintf(text, sizeof(text), "%u", stats.NumDrawCalls);
            UIPropertyLabel("Draw Calls", Str8C(text));
            snprintf(text, sizeof(text), "%u", stats.TriangleCount);
            UIPropertyLabel("Triangles", Str8C(text));
            snprintf(text, sizeof(text), "%u", stats.NumRenderedObjects);
            UIPropertyLabel("Rendered Objects", Str8C(text));
            snprintf(text, sizeof(text), "%u", stats.BoundPipelines);
            UIPropertyLabel("Pipelines Bound", Str8C(text));
            snprintf(text, sizeof(text), "%.0f / %.0f MB", stats.UsedGPUMemory, stats.TotalGPUMemory);
            UIPropertyLabel("GPU Memory", Str8C(text));
            if(Scene* scene = CurrentScene())
            {
                snprintf(text, sizeof(text), "%u", (u32)scene->GetRegistry().storage<entt::entity>().in_use());
                UIPropertyLabel("Entities", Str8C(text));
            }
            snprintf(text, sizeof(text), "%d / %d", UndoStepCount(), RedoStepCount());
            UIPropertyLabel("Undo / Redo", Str8C(text));
            UIEndPropertyGrid();
        }

        // ---- theme ------------------------------------------------------------

        void DrawTheme(const UIDockPanelContext& context)
        {
            UIToolTheme& theme = UIGetToolTheme();
            bool edited        = false;

            static const char* s_PresetNames[40];
            const i32 presetCount = Maths::Min(UIGetToolThemePresetCount(), 40);
            for(i32 i = 0; i < presetCount; i++)
                s_PresetNames[i] = UIGetToolThemePresetName(i);

            UIBeginPropertyGrid("##themepreset", 0.42f);
            int preset = UIGetToolThemePreset();
            if(UIPropertyDropdown("Preset", &preset, s_PresetNames, presetCount) && preset >= 0)
                UISetToolThemePreset(preset);
            if(UIGetToolThemePreset() < 0)
                UIPropertyLabel("", Str8Lit("edited"));
            UIEndPropertyGrid();

            UISpacer(4.0f);
            if(UISectionHeader("Base###themebase", true))
            {
                // these four define a theme; the button rebuilds the rest from them
                UIBeginPropertyGrid("##themebasegrid", 0.42f);
                edited |= UIPropertyColour("Canvas", &theme.Canvas.x, false);
                edited |= UIPropertyColour("Panel", &theme.Panel.x, false);
                edited |= UIPropertyColour("Text", &theme.Text.x, false);
                edited |= UIPropertyColour("Accent", &theme.Accent.x, false);
                UIEndPropertyGrid();
                if(UIButton("Rebuild the rest from these###themederive").clicked)
                {
                    UIDeriveToolTheme(theme, theme.Canvas, theme.Panel, theme.Text, theme.Accent);
                    edited = true;
                }
            }

            UISpacer(4.0f);
            if(UISectionHeader("Shape###themeshape", true))
            {
                static const char* kTabStyles[] = { "Pill", "Underline", "Block" };
                UIBeginPropertyGrid("##themeshapegrid", 0.42f);
                edited |= UIPropertyDropdown("Tabs", &theme.TabStyle, kTabStyles, 3);
                edited |= UIPropertyFloat("Panel Rounding", &theme.PanelRounding, 0.1f, 0.0f, 24.0f, "%.1f");
                edited |= UIPropertyFloat("Widget Rounding", &theme.WidgetRounding, 0.1f, 0.0f, 16.0f, "%.1f");
                edited |= UIPropertyFloat("Panel Gap", &theme.PanelGap, 0.1f, 0.0f, 24.0f, "%.1f");
                edited |= UIPropertyFloat("Outer Margin", &theme.OuterMargin, 0.1f, 0.0f, 32.0f, "%.1f");
                edited |= UIPropertyFloat("Tab Height", &theme.TabHeight, 0.1f, 20.0f, 48.0f, "%.0f");
                edited |= UIPropertyFloat("Font Size", &theme.FontSize, 0.05f, 9.0f, 24.0f, "%.1f");
                edited |= UIPropertyFloat("Density", &theme.Density, 0.005f, 0.6f, 1.8f, "%.2f");
                edited |= UIPropertyBool("Panel Border", &theme.PanelBorder);
                edited |= UIPropertyBool("Floating Toolbars", &theme.FloatingToolbars);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Colours###themecolours", false))
            {
                UIBeginPropertyGrid("##themecolourgrid", 0.42f);
                edited |= UIPropertyColour("Header", &theme.Header.x, false);
                edited |= UIPropertyColour("Popup", &theme.Popup.x);
                edited |= UIPropertyColour("Field", &theme.Field.x);
                edited |= UIPropertyColour("Hover", &theme.Hover.x);
                edited |= UIPropertyColour("Border", &theme.Border.x);
                edited |= UIPropertyColour("Text Dim", &theme.TextDim.x, false);
                edited |= UIPropertyColour("Accent Text", &theme.AccentText.x, false);
                edited |= UIPropertyColour("Selection", &theme.Selection.x);
                edited |= UIPropertyColour("Warning", &theme.Warning.x, false);
                edited |= UIPropertyColour("Error", &theme.Error.x, false);
                edited |= UIPropertyColour("Success", &theme.Success.x, false);
                edited |= UIPropertyColour("Axis X", &theme.AxisX.x, false);
                edited |= UIPropertyColour("Axis Y", &theme.AxisY.x, false);
                edited |= UIPropertyColour("Axis Z", &theme.AxisZ.x, false);
                UIEndPropertyGrid();
            }

            if(edited)
                UIMarkToolThemeEdited();
            (void)context;
        }

        // ---- assets -----------------------------------------------------------

        namespace
        {
            struct AssetEntry
            {
                char Name[96];
                bool Folder;
            };

            // Small previews of image files. One is loaded per frame; the least recently drawn makes room.
            struct Thumbnail
            {
                u64 PathHash = 0;
                u64 LastUsed = 0;
                SharedPtr<Graphics::Texture2D> Texture;
            };
            constexpr u32 kMaxThumbnails = 128;
            Thumbnail s_Thumbnails[kMaxThumbnails];
            u64 s_ThumbnailFrame      = 0;
            u64 s_ThumbnailLoadFrame  = 0;

            bool IsImageFile(const char* name)
            {
                const char* dot = strrchr(name, '.');
                if(!dot)
                    return false;
                static const char* kExtensions[] = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };
                for(const char* extension : kExtensions)
                {
                    const char* a = dot;
                    const char* b = extension;
                    while(*a && *b && Lower(*a) == *b)
                    {
                        a++;
                        b++;
                    }
                    if(!*a && !*b)
                        return true;
                }
                return false;
            }

            // null until it has been loaded (or when the file would not load)
            Graphics::Texture2D* GetThumbnail(const char* physicalPath)
            {
                u64 hash = 14695981039346656037ull;
                for(const char* c = physicalPath; *c; c++)
                    hash = (hash ^ (u8)*c) * 1099511628211ull;
                hash |= 1ull; // 0 = empty slot
                u32 oldest     = 0;
                for(u32 i = 0; i < kMaxThumbnails; i++)
                {
                    if(s_Thumbnails[i].PathHash == hash)
                    {
                        s_Thumbnails[i].LastUsed = s_ThumbnailFrame;
                        return s_Thumbnails[i].Texture.get();
                    }
                    if(s_Thumbnails[i].LastUsed < s_Thumbnails[oldest].LastUsed)
                        oldest = i;
                }
                if(s_ThumbnailLoadFrame == s_ThumbnailFrame)
                    return nullptr;
                s_ThumbnailLoadFrame = s_ThumbnailFrame;

                Thumbnail& slot = s_Thumbnails[oldest];
                slot.PathHash   = hash;
                slot.LastUsed   = s_ThumbnailFrame;
                slot.Texture    = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(physicalPath, physicalPath));
                return slot.Texture.get();
            }

            // One folder's listing, and the flat index of everything, in fixed storage.
            constexpr u32 kMaxFolderEntries = 512;
            constexpr u32 kMaxIndexEntries  = 4096;
            constexpr u32 kIndexPathSize    = 160;

            AssetEntry s_FolderEntries[kMaxFolderEntries];
            u32 s_FolderCount       = 0;
            char s_FolderPath[512]  = { 0 }; // relative to the assets root, "" = the root
            char s_ListedPath[512]  = { 1, 0 };
            u32 s_FolderAge         = 0;
            char s_AssetFilter[64]  = { 0 };
            f32 s_AssetScroll       = 0.0f;
            f32 s_AssetTileScale    = 1.0f; // 0.6 (list-like) .. 3 (big previews)
            char s_AssetSelected[96] = { 0 };

            Arena* s_IndexArena = nullptr;
            char* s_IndexPaths  = nullptr;
            u32 s_IndexCount    = 0;
            bool s_IndexBuilt   = false;

            AssetHooks s_AssetHooks;

            // A file operation waiting on a name or a yes: shown as a row under the toolbar.
            enum AssetAction
            {
                AssetAction_None = 0,
                AssetAction_NewFolder,
                AssetAction_NewScript,
                AssetAction_Rename,
                AssetAction_Delete
            };
            int s_AssetAction          = AssetAction_None;
            char s_AssetActionName[96] = { 0 };
            char s_AssetActionOn[96]   = { 0 }; // the entry being renamed / deleted
            bool s_AssetActionFocus    = false;
            // a file or folder picked up with Cut / Copy, waiting for Paste (physical path)
            char s_AssetClip[1100] = { 0 };
            bool s_AssetClipCut    = false;

            // Puts `from` into `folder` under its own name (or "name copy" when that is taken and it is a copy).
            bool PlaceInFolder(const std::filesystem::path& from, const std::filesystem::path& folder, bool move)
            {
                std::error_code error;
                if(!std::filesystem::exists(from, error) || !std::filesystem::is_directory(folder, error))
                    return false;
                // a folder cannot go inside itself
                const std::string fromText   = from.lexically_normal().string();
                const std::string folderText = folder.lexically_normal().string();
                if(folderText.compare(0, fromText.size(), fromText) == 0 && (folderText.size() == fromText.size() || folderText[fromText.size()] == '/'))
                    return false;

                std::filesystem::path to = folder / from.filename();
                if(move)
                {
                    if(to.lexically_normal() == from.lexically_normal())
                        return true; // already there
                    if(std::filesystem::exists(to, error))
                    {
                        LWARN("Assets: %s already has a %s", folderText.c_str(), from.filename().string().c_str());
                        return false;
                    }
                    std::filesystem::rename(from, to, error);
                    // the import settings beside a source file go with it
                    const std::filesystem::path meta = from.string() + ".meta";
                    std::error_code ignored;
                    if(!error && std::filesystem::exists(meta, ignored))
                        std::filesystem::rename(meta, to.string() + ".meta", ignored);
                }
                else
                {
                    for(int n = 1; std::filesystem::exists(to, error) && n < 100; n++)
                    {
                        char suffix[24];
                        snprintf(suffix, sizeof(suffix), n == 1 ? " copy" : " copy %d", n);
                        to = folder / (from.stem().string() + suffix + from.extension().string());
                    }
                    std::filesystem::copy(from, to, std::filesystem::copy_options::recursive, error);
                }
                if(error)
                    LWARN("Assets: %s", error.message().c_str());
                return !error;
            }

            void AssetRoot(char* out, size_t size)
            {
                const String8& root = FileSystem::Get().GetAssetPath();
                snprintf(out, size, "%.*s", (int)root.size, root.size ? (const char*)root.str : "");
            }

            void ListFolder()
            {
                char root[512];
                AssetRoot(root, sizeof(root));
                char full[1100];
                snprintf(full, sizeof(full), "%s%s%s", root, s_FolderPath[0] ? "/" : "", s_FolderPath);

                s_FolderCount = 0;
                std::error_code ec;
                for(const auto& item : std::filesystem::directory_iterator(full, ec))
                {
                    if(s_FolderCount >= kMaxFolderEntries)
                        break;
                    const std::string name = item.path().filename().string();
                    // (.meta = import settings beside a source file: not something to browse)
                    if(name.empty() || name[0] == '.' || (name.size() > 5 && name.compare(name.size() - 5, 5, ".meta") == 0))
                        continue;
                    AssetEntry& entry = s_FolderEntries[s_FolderCount++];
                    snprintf(entry.Name, sizeof(entry.Name), "%s", name.c_str());
                    entry.Folder = item.is_directory(ec);
                }

                // folders first, then by name
                for(u32 a = 1; a < s_FolderCount; a++)
                {
                    AssetEntry key = s_FolderEntries[a];
                    i32 b          = (i32)a - 1;
                    while(b >= 0 && (s_FolderEntries[b].Folder < key.Folder
                                     || (s_FolderEntries[b].Folder == key.Folder && strcmp(s_FolderEntries[b].Name, key.Name) > 0)))
                    {
                        s_FolderEntries[b + 1] = s_FolderEntries[b];
                        b--;
                    }
                    s_FolderEntries[b + 1] = key;
                }

                snprintf(s_ListedPath, sizeof(s_ListedPath), "%s", s_FolderPath);
                s_FolderAge = 0;
            }

            bool EndsWith(const char* name, const char* extension)
            {
                const size_t n = strlen(name);
                const size_t e = strlen(extension);
                if(e > n)
                    return false;
                for(size_t i = 0; i < e; i++)
                    if(Lower(name[n - e + i]) != extension[i])
                        return false;
                return true;
            }
        }

        namespace
        {
            // What kind of file it is, for the chip and the colour under the picture.
            struct AssetKind
            {
                const char* Abbrev;
                const char* Label;
                Vec4 Colour;
                bool Rendered; // model / material: the preview is rendered on request
                bool Scene;
            };

            AssetKind KindOf(const char* name)
            {
                auto any = [&](std::initializer_list<const char*> extensions)
                {
                    for(const char* extension : extensions)
                        if(EndsWith(name, extension))
                            return true;
                    return false;
                };
                if(any({ ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".gif", ".limg" }))
                    return { "TEX", "Texture", Vec4(0.82f, 0.20f, 0.33f, 1.0f), false, false };
                if(any({ ".hdr" }))
                    return { "HDR", "Environment", Vec4(0.82f, 0.18f, 0.30f, 1.0f), false, false };
                if(any({ ".glb", ".gltf", ".obj", ".fbx", ".lmesh" }))
                    return { "MSH", "Model", Vec4(0.18f, 0.82f, 0.76f, 1.0f), true, false };
                if(any({ ".lmat" }))
                    return { "MAT", "Material", Vec4(0.85f, 0.65f, 0.25f, 1.0f), true, false };
                if(any({ ".lsn" }))
                    return { "SCN", "Scene", Vec4(0.80f, 0.40f, 0.22f, 1.0f), false, true };
                if(any({ ".lprefab" }))
                    return { "PRE", "Prefab", Vec4(0.10f, 0.50f, 0.80f, 1.0f), false, false };
                if(any({ ".lua" }))
                    return { "LUA", "Script", Vec4(0.10f, 0.50f, 0.80f, 1.0f), false, false };
                if(any({ ".glsl", ".vert", ".frag", ".shader", ".comp" }))
                    return { "SHD", "Shader", Vec4(0.45f, 0.40f, 0.85f, 1.0f), false, false };
                if(any({ ".ogg", ".wav", ".mp3", ".m4a" }))
                    return { "AUD", "Audio", Vec4(0.20f, 0.80f, 0.50f, 1.0f), false, false };
                if(any({ ".ttf", ".otf" }))
                    return { "FNT", "Font", Vec4(0.60f, 0.19f, 0.32f, 1.0f), false, false };
                if(any({ ".json", ".txt", ".md", ".csv", ".ini", ".cfg", ".lmproj", ".meta" }))
                    return { "TXT", "Text", Vec4(0.55f, 0.58f, 0.62f, 1.0f), false, false };
                return { "FILE", "File", Vec4(0.55f, 0.58f, 0.62f, 1.0f), false, false };
            }
        }

        const char* AssetIcon(const char* name, bool folder)
        {
            if(folder)
                return ICON_MDI_FOLDER;
            if(EndsWith(name, ".png") || EndsWith(name, ".jpg") || EndsWith(name, ".jpeg") || EndsWith(name, ".tga") || EndsWith(name, ".hdr") || EndsWith(name, ".bmp"))
                return ICON_MDI_IMAGE;
            if(EndsWith(name, ".lua") || EndsWith(name, ".glsl") || EndsWith(name, ".vert") || EndsWith(name, ".frag") || EndsWith(name, ".shader"))
                return ICON_MDI_CODE_BRACES;
            if(EndsWith(name, ".lsn"))
                return ICON_MDI_MOVIE;
            if(EndsWith(name, ".glb") || EndsWith(name, ".gltf") || EndsWith(name, ".obj") || EndsWith(name, ".fbx") || EndsWith(name, ".lmesh"))
                return ICON_MDI_CUBE_OUTLINE;
            if(EndsWith(name, ".ogg") || EndsWith(name, ".wav") || EndsWith(name, ".mp3") || EndsWith(name, ".m4a"))
                return ICON_MDI_MUSIC_NOTE;
            if(EndsWith(name, ".lmat"))
                return ICON_MDI_PALETTE;
            if(EndsWith(name, ".lprefab"))
                return ICON_MDI_SHAPE;
            if(EndsWith(name, ".ttf") || EndsWith(name, ".otf"))
                return ICON_MDI_PENCIL;
            return ICON_MDI_FILE_OUTLINE;
        }

        void SetAssetFolder(const char* relativePath)
        {
            snprintf(s_FolderPath, sizeof(s_FolderPath), "%s", relativePath ? relativePath : "");
            s_AssetScroll = 0.0f;
        }

        float GetAssetTileScale() { return s_AssetTileScale; }
        void SetAssetTileScale(float scale) { s_AssetTileScale = Maths::Clamp(scale, 0.6f, 3.0f); }

        void ReleaseThumbnails()
        {
            for(Thumbnail& thumbnail : s_Thumbnails)
                thumbnail = Thumbnail();
            SpriteEditor::Release();
        }

        bool OpenAsset(const char* path)
        {
            if(!s_AssetHooks.Open || !path || !path[0])
                return false;
            s_AssetHooks.Open(path);
            return true;
        }

        bool TextureSlotRow(const char* label, Graphics::Texture2D* texture, char* outPath, u32 outSize)
        {
            return TextureRow(label, texture, outPath, outSize);
        }

        bool AssetDropTarget(const char* extension, char* outPath, u32 outSize)
        {
            char physical[256] = { 0 };
            if(!UIDropTarget(kAssetDragType, physical, sizeof(physical)) || !physical[0])
                return false;
            if(extension && extension[0])
            {
                const size_t length = strlen(physical), tail = strlen(extension);
                if(tail > length || strcasecmp(physical + length - tail, extension) != 0)
                    return false;
            }
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            String8 vfs       = FileSystem::Get().AbsolutePathToFileSystem(scratch.arena, Str8C(physical));
            snprintf(outPath, outSize, "%.*s", (int)vfs.size, (const char*)vfs.str);
            ScratchEnd(scratch);
            return outPath[0] != 0;
        }

        void SetAssetHooks(const AssetHooks& hooks)
        {
            s_AssetHooks = hooks;
        }

        // ---- asset picker ----
        namespace
        {
            u64 s_PickSlot         = 0; // the button whose list is open
            char s_PickFilter[64]  = { 0 };
            bool s_PickWasOpen     = false;

            // is name one of ".a;.b;.c"?
            bool HasOneOfExtensions(const char* name, const char* extensions)
            {
                const size_t length = strlen(name);
                const char* at      = extensions;
                while(at && *at)
                {
                    const char* end   = strchr(at, ';');
                    const size_t tail = end ? (size_t)(end - at) : strlen(at);
                    if(tail > 0 && tail <= length && strncasecmp(name + length - tail, at, tail) == 0)
                        return true;
                    at = end ? end + 1 : nullptr;
                }
                return false;
            }
        }

        bool AssetPickButton(const char* id, const char* extensions, char* outPath, u32 outSize)
        {
            UI_State* ui  = GetUIState();
            const f32 dpi = ui->DPIScale;

            u64 hash;
            HandleUIString(id, &hash);
            const u64 slot = UIHashCombine(GetCurrentParent()->hash, hash);

            char buttonId[96];
            snprintf(buttonId, sizeof(buttonId), ICON_MDI_MAGNIFY "%s", id);
            if(UIIconButton(buttonId, 22.0f, s_PickSlot == slot && UIIsPopupOpen("##assetpicker")).clicked)
            {
                s_PickSlot      = slot;
                s_PickFilter[0] = 0;
                s_PickWasOpen   = false;
                // files come and go: look again each time the list opens
                RefreshAssetIndex();
                UIOpenPopup("##assetpicker");
            }
            UITooltip("Choose from the project");

            if(s_PickSlot != slot)
                return false;
            if(!UIBeginPopup("##assetpicker"))
                return false;

            // type to narrow the list; the field has the keyboard as soon as it opens
            if(!s_PickWasOpen)
            {
                s_PickWasOpen = true;
                UIFocusNextTextInput();
            }
            UI_Interaction field = UITextField("###assetpickfilter", s_PickFilter, sizeof(s_PickFilter), "Search", 260.0f);
            if(field.widget)
                field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
            UIMenuSeparator();

            const u32 kShown   = 14;
            const u32 count    = GetAssetIndexCount();
            u32 matched        = 0;
            const char* chosen = nullptr;
            const char* onlyOne = nullptr;
            for(u32 i = 0; i < count; i++)
            {
                const char* path = GetAssetIndexPath(i);
                if(!HasOneOfExtensions(path, extensions) || !ContainsNoCase(path, s_PickFilter))
                    continue;
                matched++;
                onlyOne = path;
                if(matched > kShown)
                    continue;
                // the name, and the folder it is in at the far end
                const char* slash = strrchr(path, '/');
                char item[200];
                char folder[128];
                snprintf(item, sizeof(item), "%s###pick%u", slash ? slash + 1 : path, i);
                snprintf(folder, sizeof(folder), "%.*s", slash ? (int)Maths::Min<size_t>((size_t)(slash - path), sizeof(folder) - 1) : 0, path);
                if(UIMenuItem(item, folder[0] ? folder : nullptr))
                    chosen = path;
            }
            if(matched == 0)
                UIMenuItem("Nothing matches###picknone", nullptr, nullptr, false);
            else if(matched > kShown)
            {
                char more[64];
                snprintf(more, sizeof(more), "%u more - type to narrow###pickmore", matched - kShown);
                UIMenuItem(more, nullptr, nullptr, false);
            }
            // Enter takes the match when the search has got it down to one
            if(!chosen && matched == 1 && s_PickFilter[0] && Input::Get().GetKeyPressedRaw(InputCode::Key::Enter))
            {
                chosen = onlyOne;
                UICloseAllPopups();
            }
            UIEndPopup();

            if(!chosen)
                return false;
            snprintf(outPath, outSize, "//Assets/%s", chosen);
            s_PickSlot = 0;
            return true;
        }

        void RefreshAssetIndex()
        {
            if(!s_IndexArena)
            {
                s_IndexArena = ArenaAlloc((u64)kMaxIndexEntries * kIndexPathSize + Kilobytes(4));
                s_IndexPaths = PushArray(s_IndexArena, char, kMaxIndexEntries * kIndexPathSize);
            }
            s_IndexCount = 0;
            s_IndexBuilt = true;

            char root[512];
            AssetRoot(root, sizeof(root));
            const size_t rootLength = strlen(root);

            std::error_code ec;
            std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec);
            for(; !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
            {
                if(s_IndexCount >= kMaxIndexEntries)
                    break;
                const std::string name = it->path().filename().string();
                if(!name.empty() && name[0] == '.')
                {
                    if(it->is_directory(ec))
                        it.disable_recursion_pending();
                    continue;
                }
                // imported / cached copies of the things already listed
                if(it->is_directory(ec))
                {
                    if(name == "Cache" || name == "Imported")
                        it.disable_recursion_pending();
                    continue;
                }
                const std::string path = it->path().generic_string();
                if(path.size() <= rootLength + 1)
                    continue;
                snprintf(s_IndexPaths + (size_t)s_IndexCount * kIndexPathSize, kIndexPathSize, "%s", path.c_str() + rootLength + 1);
                s_IndexCount++;
            }
        }

        u32 GetAssetIndexCount()
        {
            if(!s_IndexBuilt)
                RefreshAssetIndex();
            return s_IndexCount;
        }

        const char* GetAssetIndexPath(u32 index)
        {
            return index < s_IndexCount ? s_IndexPaths + (size_t)index * kIndexPathSize : "";
        }

        void DrawAssets(const UIDockPanelContext& context)
        {
            UI_State* ui             = GetUIState();
            const f32 dpi            = context.DPIScale;
            const UIToolTheme& theme = UIGetToolTheme();

            // listings go stale: other programs write into the folder
            if(strcmp(s_ListedPath, s_FolderPath) != 0 || ++s_FolderAge > 180)
                ListFolder();

            char enterFolder[96] = { 0 };
            bool goUp            = false;
            // a folder of the path was pressed: how much of the path to keep (-1 = nothing pressed, 0 = the root)
            i32 jumpLength = -1;

            // ---- toolbar: up, where we are, filter, refresh ----
            UIBeginRowFullWidth();
            {
                UI_Widget* row                        = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;

                UIPushStyle(StyleVar_TextColor, s_FolderPath[0] ? theme.Text : theme.TextDim);
                if(UIIconButton(ICON_MDI_ARROW_UP "###assetsup", 24.0f).clicked && s_FolderPath[0])
                    goUp = true;
                UIPopStyle(StyleVar_TextColor);

                // the path, each folder a button back to itself
                {
                    auto crumb = [&](const char* text, u32 length, i32 keep, bool last)
                    {
                        UI_Widget* button = PushWidget(WidgetFlags_DrawText | WidgetFlags_DrawBackground | WidgetFlags_Clickable, Str8((u8*)text, length),
                                                       UIHashCombine(0x4352554D42ull, (u64)(keep + 1)), { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                        UIClearBorders(button);
                        button->style_vars[StyleVar_Padding]      = Vec4(5.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
                        button->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
                        UISetBackground(button, Vec4(0.0f), theme.Hover, theme.Selection);
                        // where we are reads as text; the way back reads as links
                        UISetTextColour(button, last ? theme.Text : theme.TextDim);
                        if(HandleWidgetInteraction(button).clicked && !last)
                            jumpLength = keep;
                    };
                    auto slash = [&](u32 index)
                    {
                        UI_Widget* mark = PushWidget(WidgetFlags_DrawText, Str8Lit("/"), UIHashCombine(0x43534C415348ull, (u64)index), { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                        mark->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * dpi, 0.0f, 0.0f);
                        Vec4 faint = theme.TextDim;
                        faint.w    = 0.6f;
                        UISetTextColour(mark, faint);
                    };

                    // deep paths keep the root and the last few folders
                    u32 parts = 0;
                    for(const char* c = s_FolderPath; *c; c++)
                        parts += (c == s_FolderPath || c[-1] == '/') ? 1u : 0u;
                    const u32 maxShown = 4;
                    const u32 skip     = parts > maxShown ? parts - maxShown : 0;

                    crumb("Assets", 6, 0, parts == 0);
                    if(skip > 0)
                    {
                        slash(1000);
                        UI_Widget* dots = PushWidget(WidgetFlags_DrawText, Str8Lit("..."), 0x43444F5453ull, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                        dots->style_vars[StyleVar_Padding] = Vec4(2.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
                        UISetTextColour(dots, theme.TextDim);
                    }
                    u32 index = 0;
                    for(const char* start = s_FolderPath; *start;)
                    {
                        const char* end = strchr(start, '/');
                        const u32 length = end ? (u32)(end - start) : (u32)strlen(start);
                        if(index >= skip)
                        {
                            slash(index);
                            crumb(start, length, (i32)(start - s_FolderPath) + (i32)length, index + 1 == parts);
                        }
                        index++;
                        if(!end)
                            break;
                        start = end + 1;
                    }
                }

                UI_Interaction field = UITextField("##assetsfilter", s_AssetFilter, sizeof(s_AssetFilter), "Filter", 0.0f);
                if(field.widget)
                    field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);

                // tile size: the slider, or Ctrl / Cmd + wheel over the tiles (a narrow panel keeps the room for the filter)
                if(context.Size.x > 460.0f * dpi)
                {
                    UIPushStyle(StyleVar_TextColor, theme.TextDim);
                    UILabelCStr("##assetssizeicon", ICON_MDI_IMAGE);
                    UIPopStyle(StyleVar_TextColor);
                    UIToolSlider("###assetssize", &s_AssetTileScale, 0.6f, 3.0f, 90.0f);
                    UITooltip("Tile size (Ctrl + scroll)");
                }

                if(UIIconButton(ICON_MDI_REFRESH "###assetsrefresh", 24.0f).clicked)
                {
                    ListFolder();
                    RefreshAssetIndex();
                }
                // make things here, and what can be done with the selected one (also on a right-click)
                if(UIIconButton(ICON_MDI_PLUS "###assetsadd", 24.0f).clicked)
                    UIOpenPopup("##assetsmenu");
                UITooltip("New, import, rename, delete");
            }
            UIEndRow();

            char folderPhysical[1100];
            {
                char root[512];
                AssetRoot(root, sizeof(root));
                snprintf(folderPhysical, sizeof(folderPhysical), "%s/%s", root, s_FolderPath);
            }
            bool openSelected = false;
            if(UIBeginPopup("##assetsmenu"))
            {
                auto begin = [](int action, const char* name, const char* on)
                {
                    s_AssetAction = action;
                    snprintf(s_AssetActionName, sizeof(s_AssetActionName), "%s", name);
                    snprintf(s_AssetActionOn, sizeof(s_AssetActionOn), "%s", on);
                    s_AssetActionFocus = true;
                };
                if(UIMenuItem(ICON_MDI_FOLDER "  New Folder"))
                    begin(AssetAction_NewFolder, "New Folder", "");
                if(UIMenuItem(ICON_MDI_PENCIL "  New Lua Script"))
                    begin(AssetAction_NewScript, "NewScript.lua", "");
                if(UIMenuItem(ICON_MDI_GRID "  New Sprite"))
                    SpriteEditor::New(32, 32);
#ifdef LUMOS_PLATFORM_IOS
                // copies picked files (Files, iCloud Drive) into this folder
                if(UIMenuItem(ICON_MDI_IMPORT "  Import Files..."))
                {
                    FileDialogs::ImportFiles("", folderPhysical);
                    s_ListedPath[0] = 1;
                }
#endif
                const bool has = s_AssetSelected[0] != 0;
                UIMenuSeparator();
                if(UIMenuItem("Open", nullptr, nullptr, has))
                    openSelected = true;
                if(has && IsImageFile(s_AssetSelected) && UIMenuItem(ICON_MDI_PENCIL "  Edit in Sprite Editor"))
                {
                    char physical[700];
                    snprintf(physical, sizeof(physical), "%s/%s", folderPhysical, s_AssetSelected);
                    SpriteEditor::Open(physical);
                }
                if(UIMenuItem("Rename", nullptr, nullptr, has))
                    begin(AssetAction_Rename, s_AssetSelected, s_AssetSelected);
                if(UIMenuItem("Duplicate", nullptr, nullptr, has))
                {
                    // "name copy.ext", then "name copy 2.ext"...
                    std::error_code error;
                    const std::filesystem::path from = std::filesystem::path(folderPhysical) / s_AssetSelected;
                    for(int n = 1; n < 100; n++)
                    {
                        char suffix[24];
                        snprintf(suffix, sizeof(suffix), n == 1 ? " copy" : " copy %d", n);
                        const std::filesystem::path to = from.parent_path() / (from.stem().string() + suffix + from.extension().string());
                        if(std::filesystem::exists(to, error))
                            continue;
                        std::filesystem::copy(from, to, std::filesystem::copy_options::recursive, error);
                        if(error)
                            LWARN("Assets: could not duplicate %s: %s", s_AssetSelected, error.message().c_str());
                        break;
                    }
                    s_ListedPath[0] = 1;
                }
                if(UIMenuItem("Delete...", nullptr, nullptr, has))
                    begin(AssetAction_Delete, "", s_AssetSelected);
                UIMenuSeparator();
                if(UIMenuItem("Cut", nullptr, nullptr, has))
                {
                    snprintf(s_AssetClip, sizeof(s_AssetClip), "%s/%s", folderPhysical, s_AssetSelected);
                    s_AssetClipCut = true;
                }
                if(UIMenuItem("Copy", nullptr, nullptr, has))
                {
                    snprintf(s_AssetClip, sizeof(s_AssetClip), "%s/%s", folderPhysical, s_AssetSelected);
                    s_AssetClipCut = false;
                }
                if(UIMenuItem("Paste Here", nullptr, nullptr, s_AssetClip[0] != 0))
                {
                    if(PlaceInFolder(s_AssetClip, folderPhysical, s_AssetClipCut) && s_AssetClipCut)
                        s_AssetClip[0] = 0;
                    s_ListedPath[0] = 1;
                    RefreshAssetIndex();
                }
                // models: the other ways of bringing one in
                if(has && KindOf(s_AssetSelected).Rendered && !EndsWith(s_AssetSelected, ".lmat") && (s_AssetHooks.ImportAsScene || s_AssetHooks.Reimport))
                {
                    char vfs[700];
                    snprintf(vfs, sizeof(vfs), "//Assets/%s%s%s", s_FolderPath, s_FolderPath[0] ? "/" : "", s_AssetSelected);
                    UIMenuSeparator();
                    if(s_AssetHooks.ImportAsScene && (EndsWith(s_AssetSelected, ".gltf") || EndsWith(s_AssetSelected, ".glb")) && UIMenuItem("Import as Scene"))
                        s_AssetHooks.ImportAsScene(vfs);
                    if(s_AssetHooks.Reimport && !EndsWith(s_AssetSelected, ".lmesh") && UIMenuItem("Reimport"))
                        s_AssetHooks.Reimport(vfs);
                }
                UIMenuSeparator();
                if(UIMenuItem("Copy Asset Path", nullptr, nullptr, has))
                {
                    char vfs[700];
                    snprintf(vfs, sizeof(vfs), "//Assets/%s%s%s", s_FolderPath, s_FolderPath[0] ? "/" : "", s_AssetSelected);
                    Input::Get().SetClipboard(vfs);
                }
#if !defined(LUMOS_PLATFORM_IOS) && !defined(LUMOS_PLATFORM_ANDROID)
                if(UIMenuItem("Show in File Manager"))
                    OS::Get().OpenFileLocation(std::string(folderPhysical) + (has ? std::string("/") + s_AssetSelected : std::string()));
#endif
                UIEndPopup();
            }

            // ---- the operation in hand: a name to type, or a delete to confirm ----
            f32 actionRow = 0.0f;
            if(s_AssetAction != AssetAction_None)
            {
                actionRow   = 36.0f * dpi;
                bool accept = false, cancel = false;
                UIBeginRowFullWidth();
                {
                    UI_Widget* row                        = ui->parents.Back();
                    row->style_vars[StyleVar_Padding]     = Vec4(8.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                    row->style_vars[StyleVar_ItemSpacing] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
                    row->flags |= WidgetFlags_CentreChildrenY;

                    if(s_AssetAction == AssetAction_Delete)
                    {
                        char question[160];
                        snprintf(question, sizeof(question), "Delete \"%s\"? This cannot be undone.", s_AssetActionOn);
                        UIPushStyle(StyleVar_TextColor, theme.Warning);
                        UILabelCStr("##assetsdeleteask", question);
                        UIPopStyle(StyleVar_TextColor);
                        UIFlexSpacer();
                        accept = UIButton("Delete###assetsactionok").clicked;
                    }
                    else
                    {
                        UILabelCStr("##assetsactionlabel", s_AssetAction == AssetAction_Rename ? "Rename to" : "Name");
                        if(s_AssetActionFocus)
                            UIFocusNextTextInput();
                        s_AssetActionFocus = false;
                        UI_Interaction name = UITextField("###assetsactionname", s_AssetActionName, sizeof(s_AssetActionName), "Name", 0.0f);
                        if(name.widget)
                            name.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
                        accept = UIButton("OK###assetsactionok").clicked
                            || (ui->FocusedTextInput != 0 && Input::Get().GetKeyPressedRaw(InputCode::Key::Enter));
                    }
                    cancel = UIButton("Cancel###assetsactioncancel").clicked || Input::Get().GetKeyPressedRaw(InputCode::Key::Escape);
                }
                UIEndRow();

                if(accept)
                {
                    std::error_code error;
                    const std::filesystem::path folder = folderPhysical;
                    const bool named = s_AssetActionName[0] && !strpbrk(s_AssetActionName, "/\\:");
                    if(s_AssetAction == AssetAction_Delete)
                    {
                        // a deleted scene file leaves the project's scene list too
                        const std::filesystem::path gone = folder / s_AssetActionOn;
                        if(gone.extension() == ".lsn")
                        {
                            SceneManager* manager = Application::Get().GetSceneManager();
                            const std::string name = gone.stem().string();
                            for(u32 i = 0; i < (u32)manager->GetScenes().Size(); i++)
                                if(manager->GetScenes()[i]->GetSceneName() == name)
                                {
                                    manager->RemoveScene(i);
                                    break;
                                }
                        }
                        std::filesystem::remove_all(gone, error);
                    }
                    else if(!named)
                        error = std::make_error_code(std::errc::invalid_argument);
                    else if(std::filesystem::exists(folder / s_AssetActionName, error))
                        error = std::make_error_code(std::errc::file_exists);
                    else if(s_AssetAction == AssetAction_NewFolder)
                        std::filesystem::create_directories(folder / s_AssetActionName, error);
                    else if(s_AssetAction == AssetAction_Rename)
                        std::filesystem::rename(folder / s_AssetActionOn, folder / s_AssetActionName, error);
                    else if(s_AssetAction == AssetAction_NewScript)
                    {
                        std::string file = s_AssetActionName;
                        if(file.size() < 4 || file.compare(file.size() - 4, 4, ".lua") != 0)
                            file += ".lua";
                        const std::string path = (folder / file).string();
                        static const char* kTemplate = "-- Called once when the scene starts\nfunction OnInit()\nend\n\n-- Called every frame; dt is in seconds\nfunction OnUpdate(dt)\nend\n";
                        if(FILE* out = fopen(path.c_str(), "w"))
                        {
                            fputs(kTemplate, out);
                            fclose(out);
                            snprintf(s_AssetActionName, sizeof(s_AssetActionName), "%s", file.c_str());
                        }
                        else
                            error = std::make_error_code(std::errc::io_error);
                    }

                    if(error)
                        LWARN("Assets: %s", error.message().c_str());
                    else
                    {
                        if(s_AssetAction == AssetAction_Delete)
                            s_AssetSelected[0] = 0;
                        else
                            snprintf(s_AssetSelected, sizeof(s_AssetSelected), "%s", s_AssetActionName);
                        s_AssetAction = AssetAction_None;
                    }
                    s_ListedPath[0] = 1;
                    RefreshAssetIndex();
                }
                if(cancel)
                    s_AssetAction = AssetAction_None;
            }

            // ---- tiles ----
            const f32 toolbar    = 36.0f * dpi;
            const f32 listHeight = Maths::Max(context.Size.y - toolbar - actionRow, 40.0f * dpi);
            if(context.Hovered && ControlHeld())
            {
                const f32 wheel = Input::Get().GetScrollOffset();
                if(wheel != 0.0f)
                    s_AssetTileScale += wheel * 0.1f;
            }
            s_AssetTileScale = Maths::Clamp(s_AssetTileScale, 0.6f, 3.0f);

            // the picture grows with the scale; the name keeps its size
            // a card per file: the picture across the top, a line in the type's colour, name and kind under it
            const f32 iconBox    = floorf(theme.FontSize * 5.2f * dpi * s_AssetTileScale);
            const f32 iconSize   = iconBox * 0.5f;
            const f32 tilePad    = floorf(5.0f * dpi);
            const f32 tileW      = iconBox + tilePad * 2.0f;
            const f32 tileH      = floorf(iconBox + tilePad * 2.0f + theme.FontSize * 2.6f * dpi + 10.0f * dpi);
            const f32 gap        = floorf(6.0f * dpi);
            const bool flipRendered = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            bool previewAsked    = false;
            const f32 margin     = floorf(8.0f * dpi);
            const i32 columns    = Maths::Max(1, (i32)floorf((context.Size.x - margin * 2.0f - 10.0f * dpi + gap) / (tileW + gap)));

            // what passes the filter
            s_ThumbnailFrame++;

            static u16 s_Shown[kMaxFolderEntries];
            u32 shown = 0;
            for(u32 i = 0; i < s_FolderCount; i++)
                if(ContainsNoCase(s_FolderEntries[i].Name, s_AssetFilter))
                    s_Shown[shown++] = (u16)i;

            const i32 rows      = (i32)((shown + (u32)columns - 1) / (u32)columns);
            const f32 rowStride = tileH + gap;
            const i32 firstRow  = Maths::Max(0, (i32)floorf(s_AssetScroll / rowStride) - 1);
            const i32 lastRow   = Maths::Min(rows, firstRow + (i32)ceilf(listHeight / rowStride) + 3);

            BeginListArea("##assetslist", listHeight, &s_AssetScroll);
            UI_Widget* listArea = ui->parents.Back();
            bool overTile       = false;
            ui->parents.Back()->style_vars[StyleVar_Padding] = Vec4(margin, 0.0f, 0.0f, 0.0f);
            ListSpacer(0x41544F50ull, (f32)firstRow * rowStride);

            for(i32 r = firstRow; r < lastRow; r++)
            {
                UI_Widget* rowWidget = PushWidget(WidgetFlags_StackHorizontally, Str8Lit(""), UIHashCombine(0x41524F57ull, (u64)r),
                                                  { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, rowStride });
                rowWidget->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                rowWidget->style_vars[StyleVar_ItemSpacing] = Vec4(gap, 0.0f, 0.0f, 0.0f);
                PushParent(rowWidget);

                for(i32 col = 0; col < columns; col++)
                {
                    const u32 at = (u32)(r * columns + col);
                    if(at >= shown)
                        break;
                    const AssetEntry& entry = s_FolderEntries[s_Shown[at]];
                    const bool selected     = strcmp(s_AssetSelected, entry.Name) == 0;

                    char id[128];
                    snprintf(id, sizeof(id), "###asset%s", entry.Name);
                    u64 hash;
                    HandleUIString(id, &hash);

                    UI_Widget* tile = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_StackVertically,
                                                 Str8Lit(""), hash, { SizeKind_Pixels, tileW }, { SizeKind_Pixels, tileH });
                    UIClearBorders(tile);
                    tile->style_vars[StyleVar_Padding]      = Vec4(tilePad, tilePad, 0.0f, 0.0f);
                    tile->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f, 3.0f * dpi, 0.0f, 0.0f);
                    tile->style_vars[StyleVar_CornerRadius] = Vec4((theme.WidgetRounding + 3.0f) * dpi, 0.0f, 0.0f, 0.0f);
                    // folders sit on the panel; files are cards
                    const Vec4 rest = selected ? theme.Selection : (entry.Folder ? Vec4(0.0f) : theme.Field);
                    UISetBackground(tile, rest, selected ? theme.Selection : theme.Hover, theme.Selection);

                    UI_Interaction interaction = HandleWidgetInteraction(tile);

                    char root[512];
                    AssetRoot(root, sizeof(root));
                    char physical[256];
                    snprintf(physical, sizeof(physical), "%s/%s%s%s", root, s_FolderPath, s_FolderPath[0] ? "/" : "", entry.Name);

                    // the picture: the file itself, a saved shot of a scene, or a rendered preview of a model / material
                    const AssetKind kind           = entry.Folder ? AssetKind { "", "Folder", theme.Accent, false, false } : KindOf(entry.Name);
                    Graphics::Texture2D* thumbnail = nullptr;
                    bool flipThumbnail             = false;
                    if(!entry.Folder && IsImageFile(entry.Name))
                        thumbnail = GetThumbnail(physical);
                    else if(kind.Scene)
                    {
                        // the editor saves one beside the scenes when a scene is saved
                        char shot[700];
                        const char* dot = strrchr(entry.Name, '.');
                        snprintf(shot, sizeof(shot), "%s/Scenes/Cache/%.*s.png", root, dot ? (int)(dot - entry.Name) : (int)strlen(entry.Name), entry.Name);
                        if(FileSystem::FileExists(Str8C(shot)))
                        {
                            thumbnail     = GetThumbnail(shot);
                            flipThumbnail = flipRendered;
                        }
                    }
                    else if(kind.Rendered && s_AssetHooks.Preview && !previewAsked)
                    {
                        char vfs[700];
                        char png[700];
                        snprintf(vfs, sizeof(vfs), "//Assets/%s%s%s", s_FolderPath, s_FolderPath[0] ? "/" : "", entry.Name);
                        if(s_AssetHooks.Preview(vfs, png, sizeof(png)))
                        {
                            thumbnail     = GetThumbnail(png);
                            flipThumbnail = flipRendered;
                        }
                        else
                            previewAsked = true; // one request a frame is plenty
                    }

                    PushParent(tile);
                    {
                        const f32 box    = iconBox;
                        UI_Widget* frame = PushWidget(WidgetFlags_CentreChildrenX | WidgetFlags_CentreChildrenY | WidgetFlags_StackVertically | (entry.Folder ? 0 : WidgetFlags_DrawBackground),
                                                      Str8Lit(""), 0x54464D45ull, { SizeKind_Pixels, box }, { SizeKind_Pixels, box });
                        UIClearBorders(frame);
                        frame->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                        frame->style_vars[StyleVar_CornerRadius]    = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
                        frame->style_vars[StyleVar_BackgroundColor] = theme.Canvas;
                        PushParent(frame);
                        if(thumbnail && thumbnail->GetWidth() > 0 && thumbnail->GetHeight() > 0)
                        {
                            // not clickable, so the tile keeps the press
                            const f32 aspect = (f32)thumbnail->GetWidth() / (f32)thumbnail->GetHeight();
                            const Vec2 size  = aspect >= 1.0f ? Vec2(box, box / aspect) : Vec2(box * aspect, box);
                            // a wide picture sits in the middle of the square (centring does nothing along the stack)
                            if(size.y < box - 1.0f)
                            {
                                UI_Widget* above = PushWidget(0, Str8Lit(""), 0x54414256ull, { SizeKind_Pixels, 1.0f }, { SizeKind_Pixels, floorf((box - size.y) * 0.5f) });
                                above->style_vars[StyleVar_Padding] = Vec4(0.0f);
                            }
                            UI_Widget* image = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x54484D42ull,
                                                          { SizeKind_Pixels, floorf(size.x) }, { SizeKind_Pixels, floorf(size.y) });
                            UIClearBorders(image);
                            image->texture                          = thumbnail;
                            image->FlipV                            = flipThumbnail;
                            image->style_vars[StyleVar_Padding]      = Vec4(0.0f);
                            image->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
                            UISetBackground(image, Vec4(1.0f), Vec4(1.0f), Vec4(1.0f));
                        }
                        else
                        {
                            UI_Widget* above = PushWidget(0, Str8Lit(""), 0x54414256ull, { SizeKind_Pixels, 1.0f }, { SizeKind_Pixels, floorf((box - iconSize * 1.2f) * 0.5f) });
                            above->style_vars[StyleVar_Padding] = Vec4(0.0f);
                            UIPushStyle(StyleVar_FontSize, iconSize);
                            UIPushStyle(StyleVar_TextColor, entry.Folder ? theme.Accent : kind.Colour);
                            UILabel("###icon", Str8C((char*)AssetIcon(entry.Name, entry.Folder)));
                            UIPopStyle(StyleVar_TextColor);
                            UIPopStyle(StyleVar_FontSize);
                        }

                        PopParent(frame);

                        // a line in the type's colour
                        if(!entry.Folder)
                        {
                            UI_Widget* line = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x544C4E45ull, { SizeKind_Pixels, box }, { SizeKind_Pixels, Maths::Max(2.0f, floorf(2.0f * dpi)) });
                            UIClearBorders(line);
                            line->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                            line->style_vars[StyleVar_CornerRadius]    = Vec4(1.0f * dpi, 0.0f, 0.0f, 0.0f);
                            line->style_vars[StyleVar_BackgroundColor] = kind.Colour;
                        }

                        UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.9f * dpi);
                        UIPushStyle(StyleVar_TextColor, theme.Text);
                        UILabelEllipsis("###name", Str8C((char*)entry.Name), box / dpi - 4.0f);
                        UIPopStyle(StyleVar_TextColor);
                        UIPopStyle(StyleVar_FontSize);

                        UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.74f * dpi);
                        UIPushStyle(StyleVar_TextColor, theme.TextDim);
                        UILabel("###kind", Str8C((char*)kind.Label));
                        UIPopStyle(StyleVar_TextColor);
                        UIPopStyle(StyleVar_FontSize);

                        // (on the tile, not the picture's frame: that one centres its children, floating ones too)
                        // what it is, in the corner of the picture
                        if(!entry.Folder && iconBox > 56.0f * dpi)
                        {
                            UI_Widget* chip = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                                         Str8C((char*)kind.Abbrev), 0x54434850ull, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                            UIClearBorders(chip);
                            chip->relative_position                    = Vec2(tilePad + 4.0f * dpi, tilePad + 4.0f * dpi);
                            chip->style_vars[StyleVar_FontSize]        = Vec4(theme.FontSize * 0.68f * dpi, 0.0f, 0.0f, 1.0f);
                            chip->style_vars[StyleVar_Padding]         = Vec4(4.0f * dpi, 1.5f * dpi, 0.0f, 0.0f);
                            chip->style_vars[StyleVar_CornerRadius]    = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
                            chip->style_vars[StyleVar_BackgroundColor] = kind.Colour;
                            // dark text on a bright chip, light on a dark one
                            const f32 luminance = 0.299f * kind.Colour.x + 0.587f * kind.Colour.y + 0.114f * kind.Colour.z;
                            UISetTextColour(chip, luminance > 0.55f ? Vec4(0.07f, 0.06f, 0.05f, 1.0f) : Vec4(0.93f, 0.91f, 0.86f, 1.0f));
                        }
                    }
                    PopParent(tile);

                    // no double tap on a touch screen: a second tap on the selected one opens it
                    const bool openByTap = interaction.clicked && selected && ui->ClickOnRelease;
                    if(interaction.clicked)
                        snprintf(s_AssetSelected, sizeof(s_AssetSelected), "%s", entry.Name);
                    overTile |= interaction.hovering;
                    if(interaction.right_clicked)
                    {
                        snprintf(s_AssetSelected, sizeof(s_AssetSelected), "%s", entry.Name);
                        UIOpenPopupAt("##assetsmenu", UIMousePosition());
                    }
                    if(thumbnail)
                    {
                        char caption[200];
                        snprintf(caption, sizeof(caption), "%s\n%u x %u", entry.Name, thumbnail->GetWidth(), thumbnail->GetHeight());
                        // (rendered previews are stored upside down on some back ends: the big one would be too)
                        if(!flipThumbnail)
                            UIImageTooltip(interaction, thumbnail, caption);
                    }

                    if(interaction.double_clicked || openByTap || (openSelected && selected))
                    {
                        if(entry.Folder)
                            snprintf(enterFolder, sizeof(enterFolder), "%s", entry.Name);
                        else if(s_AssetHooks.Open)
                            s_AssetHooks.Open(physical);
                    }

                    // files can be dragged out (onto the scene view, a property...) - and onto a folder, which moves them there
                    if(!entry.Folder)
                        UIDragSource(kAssetDragType, physical, sizeof(physical), Str8C((char*)entry.Name));
                    else
                    {
                        char dropped[256] = { 0 };
                        if(UIDropTarget(kAssetDragType, dropped, sizeof(dropped)) && dropped[0])
                        {
                            PlaceInFolder(dropped, physical, true);
                            s_ListedPath[0] = 1;
                            RefreshAssetIndex();
                        }
                    }
                }
                PopParent(rowWidget);
            }

            ListSpacer(0x41424F54ull, (f32)Maths::Max(0, rows - lastRow) * rowStride);
            UIEndScrollArea();

            // the gaps between and under the tiles: the folder's own menu, nothing selected
            if(!overTile && !UIIsPopupOpen("##assetsmenu"))
            {
                const Vec2 pointer = UIMousePosition();
                const bool inside  = pointer.x >= listArea->position.x && pointer.x < listArea->position.x + listArea->size.x
                    && pointer.y >= listArea->position.y && pointer.y < listArea->position.y + listArea->size.y;
                // not through a drawer or popup lying over the panel
                bool free = ui->hot_widget == 0;
                for(UI_Widget* w = listArea; w && !free; w = w->parent)
                    free = w->hash == ui->hot_widget;
                if(inside && free)
                {
                    if(Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonRight) || UILongPressed())
                    {
                        s_AssetSelected[0] = 0;
                        UIOpenPopupAt("##assetsmenu", UIMousePosition());
                    }
                    else if(Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
                        s_AssetSelected[0] = 0;
                }
            }

            if(shown == 0)
                DimLabel("##assetsempty", s_FolderCount == 0 ? "  Empty folder" : "  Nothing matches");

            // folder changes last: the listing was in use above
            if(jumpLength >= 0)
            {
                s_FolderPath[Maths::Min((u32)jumpLength, (u32)sizeof(s_FolderPath) - 1)] = 0;
                s_AssetScroll    = 0.0f;
                s_AssetFilter[0] = 0;
            }
            else if(goUp)
            {
                char* slash = strrchr(s_FolderPath, '/');
                if(slash)
                    *slash = 0;
                else
                    s_FolderPath[0] = 0;
                s_AssetScroll = 0.0f;
            }
            else if(enterFolder[0])
            {
                const size_t length = strlen(s_FolderPath);
                snprintf(s_FolderPath + length, sizeof(s_FolderPath) - length, "%s%s", length ? "/" : "", enterFolder);
                s_AssetScroll   = 0.0f;
                s_AssetFilter[0] = 0;
            }
        }

        // ---- scene / project settings -------------------------------------------

        static bool PropertyUInt(const char* label, u32* value, u32 minimum, u32 maximum)
        {
            int v = (int)*value;
            if(UIPropertyInt(label, &v, 0.1f, (int)minimum, (int)maximum))
            {
                *value = (u32)v;
                return true;
            }
            return false;
        }

        void DrawSceneSettings(const UIDockPanelContext& context)
        {
            Scene* scene = CurrentScene();
            if(!scene)
            {
                DimLabel("##scenesettingsnone", "No scene");
                return;
            }

            Scene::SceneSettings& settings      = scene->GetSettings();
            Scene::SceneRenderSettings& render = settings.RenderSettings;

            UIPropertyLabel("Scene", Str8C((char*)scene->GetSceneName().c_str()));

            // plain data throughout: one snapshot scope covers every row
            UIPushUndoTarget(&settings, sizeof(settings));

            UISpacer(4.0f);
            if(UISectionHeader("Rendering###scenerender", true))
            {
                UIBeginPropertyGrid("##scenerendergrid", 0.5f);
                UIPropertyBool("3D Renderer", &render.Renderer3DEnabled);
                UIPropertyBool("2D Renderer", &render.Renderer2DEnabled);
                UIPropertyBool("Skybox", &render.SkyboxRenderEnabled);
                UIPropertyBool("Debug Draw", &render.DebugRenderEnabled);
                UIPropertyBool("Depth Pre-Pass", &render.DepthPrePass);
                UIPropertyBool("Wireframe", &render.Wireframe);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Shadows###sceneshadows", true))
            {
                UIBeginPropertyGrid("##sceneshadowgrid", 0.5f);
                UIPropertyBool("Enabled", &render.ShadowsEnabled);
                PropertyUInt("Cascades", &render.m_ShadowMapNum, 1, 4);
                UIPropertyFloat("Split Lambda", &render.m_CascadeSplitLambda, 0.005f, 0.0f, 1.0f, "%.2f");
                UIPropertyFloat("Light Size", &render.m_LightSize, 0.01f, 0.0f, 20.0f, "%.2f");
                UIPropertyFloat("Bias", &render.m_InitialBias, 0.0001f, 0.0f, 0.1f, "%.4f");
                UIPropertyFloat("Fade", &render.m_ShadowFade, 0.5f, 0.0f, 1000.0f, "%.1f");
                UIPropertyInt("Max Spot Shadows", &render.MaxSpotShadows, 0.1f, 0, 16);
                if(render.MaxSpotShadows > 0)
                {
                    static const char* kTiles[] = { "256", "512", "1024", "2048" };
                    int tile = 0;
                    while(tile < 3 && (256 << tile) < render.SpotShadowMaxResolution)
                        tile++;
                    if(UIPropertyDropdown("Spot Max Resolution", &tile, kTiles, 4))
                        render.SpotShadowMaxResolution = 256 << tile;
                    UIPropertyFloat("Spot Shadow Distance", &render.SpotShadowDistance, 0.5f, 1.0f, 10000.0f, "%.0f");
                }
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Tone###scenetone", true))
            {
                static const char* kToneMaps[] = { "None", "Linear", "Simple Reinhard", "Luma Reinhard", "White Preserving Reinhard",
                                                   "Uncharted 2", "ACES", "AgX", "Khronos Neutral" };
                int tone = Maths::Clamp((int)render.m_ToneMapIndex, 0, 8);
                UIBeginPropertyGrid("##scenetonegrid", 0.5f);
                if(UIPropertyDropdown("Tone Map", &tone, kToneMaps, 9))
                    render.m_ToneMapIndex = (u32)tone;
                UIPropertyFloat("Exposure", &render.m_Exposure, 0.01f, 0.0f, 20.0f, "%.2f");
                UIPropertyFloat("Brightness", &render.Brightness, 0.005f, -1.0f, 1.0f, "%.2f");
                UIPropertyFloat("Contrast", &render.Contrast, 0.005f, 0.0f, 3.0f, "%.2f");
                UIPropertyFloat("Saturation", &render.Saturation, 0.005f, 0.0f, 3.0f, "%.2f");
                UIPropertyBool("Adaptive Exposure", &render.AdaptiveExposure);
                if(render.AdaptiveExposure)
                {
                    // the picture is pushed towards this average brightness, within the log range
                    UIPropertySlider("Target Brightness", &render.AdaptiveTargetLuminance, 0.01f, 1.0f, "%.2f");
                    UIPropertySlider("Darkest (log)", &render.AdaptiveMinLogLum, -20.0f, 0.0f, "%.1f");
                    UIPropertySlider("Brightest (log)", &render.AdaptiveMaxLogLum, 0.0f, 20.0f, "%.1f");
                    UIPropertySlider("Adapt to Bright", &render.AdaptiveSpeedUp, 0.01f, 10.0f, "%.2f");
                    UIPropertySlider("Adapt to Dark", &render.AdaptiveSpeedDown, 0.01f, 10.0f, "%.2f");
                }
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Post Process###scenepost", true))
            {
                UIBeginPropertyGrid("##scenepostgrid", 0.5f);
                UIPropertyBool("Bloom", &render.BloomEnabled);
                if(render.BloomEnabled)
                {
                    UIPropertyFloat("Bloom Intensity", &render.m_BloomIntensity, 0.01f, 0.0f, 10.0f, "%.2f");
                    UIPropertyFloat("Bloom Threshold", &render.BloomThreshold, 0.01f, 0.0f, 10.0f, "%.2f");
                }
                UIPropertyBool("SSAO", &render.SSAOEnabled);
                if(render.SSAOEnabled)
                {
                    UIPropertyFloat("SSAO Radius", &render.SSAOSampleRadius, 0.01f, 0.0f, 16.0f, "%.2f");
                    UIPropertyFloat("SSAO Strength", &render.SSAOStrength, 0.01f, 0.0f, 16.0f, "%.2f");
                }
                UIPropertyBool("Reflections (SSR)", &render.SSREnabled);
                if(render.SSREnabled)
                {
                    UIPropertyFloat("SSR Strength", &render.SSRStrength, 0.01f, 0.0f, 4.0f, "%.2f");
                    UIPropertyFloat("SSR Distance", &render.SSRMaxDistance, 0.1f, 0.0f, 500.0f, "%.1f");
                }
                UIPropertyBool("Contact Shadows", &render.ContactShadowsEnabled);
                if(render.ContactShadowsEnabled)
                    UIPropertyFloat("Contact Length", &render.ContactShadowLength, 0.005f, 0.01f, 4.0f, "%.2f");
                UIPropertyBool("Volumetric Fog", &render.VolumetricFogEnabled);
                if(render.VolumetricFogEnabled)
                {
                    UIPropertyFloat("Fog Density", &render.VolumetricFogDensity, 0.0005f, 0.0f, 1.0f, "%.4f");
                    UIPropertyFloat("Fog Falloff", &render.VolumetricFogFalloff, 0.001f, 0.0f, 4.0f, "%.3f");
                    UIPropertyFloat("Fog Base Height", &render.VolumetricFogBaseHeight, 0.1f, -10000.0f, 10000.0f, "%.1f");
                    UIPropertyColour("Fog Colour", &render.VolumetricFogColour.x, false);
                    UIPropertyFloat("Fog Anisotropy", &render.VolumetricFogAnisotropy, 0.01f, -0.95f, 0.95f, "%.2f");
                    UIPropertyFloat("Fog Sun", &render.VolumetricFogSun, 0.01f, 0.0f, 16.0f, "%.2f");
                    UIPropertyFloat("Fog Ambient", &render.VolumetricFogAmbient, 0.01f, 0.0f, 16.0f, "%.2f");
                    UIPropertyFloat("Fog Distance", &render.VolumetricFogDistance, 1.0f, 1.0f, 10000.0f, "%.0f");
                }
                UIPropertyBool("Global Illumination", &render.GIEnabled);
                if(render.GIEnabled)
                {
                    UIPropertyFloat("GI Probe Spacing", &render.GIProbeSpacing, 0.05f, 0.25f, 64.0f, "%.2f");
                    UIPropertyFloat("GI Intensity", &render.GIIntensity, 0.01f, 0.0f, 8.0f, "%.2f");
                    UIPropertyFloat("GI Bounce", &render.GIBounce, 0.01f, 0.0f, 1.0f, "%.2f");
                }
                UIPropertyBool("TAA", &render.TAAEnabled);
                UIPropertyBool("FXAA", &render.FXAAEnabled);
                UIPropertyBool("SMAA", &render.SMAAEnabled);
                UIPropertyBool("Debanding", &render.DebandingEnabled);
                UIPropertyBool("Sharpen", &render.SharpenEnabled);
                UIPropertyBool("Depth of Field", &render.DepthOfFieldEnabled);
                if(render.DepthOfFieldEnabled)
                {
                    UIPropertyFloat("DoF Strength", &render.DepthOfFieldStrength, 0.01f, 0.0f, 10.0f, "%.2f");
                    UIPropertyFloat("DoF Distance", &render.DepthOfFieldDistance, 0.5f, 0.0f, 10000.0f, "%.1f");
                }
                UIPropertyBool("Motion Blur", &render.MotionBlurEnabled);
                if(render.MotionBlurEnabled)
                    UIPropertyFloat("Blur Strength", &render.MotionBlurStrength, 0.01f, 0.0f, 4.0f, "%.2f");
                UIPropertyBool("Vignette", &render.VignetteEnabled);
                if(render.VignetteEnabled)
                {
                    UIPropertyFloat("Vignette Intensity", &render.VignetteIntensity, 0.005f, 0.0f, 2.0f, "%.2f");
                    UIPropertyFloat("Vignette Smoothness", &render.VignetteSmoothness, 0.005f, 0.0f, 2.0f, "%.2f");
                }
                UIPropertyBool("Chromatic Aberration", &render.ChromaticAberationEnabled);
                UIPropertyBool("Film Grain", &render.FilmicGrainEnabled);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Physics###scenephysics", false))
            {
                UIBeginPropertyGrid("##scenephysicsgrid", 0.5f);
                UIPropertyBool("3D Physics", &settings.PhysicsEnabled3D);
                UIPropertyVec3("3D Gravity", &settings.Physics3DSettings.Gravity.x, 0.05f);
                UIPropertyFloat("3D Damping", &settings.Physics3DSettings.Dampening, 0.0001f, 0.0f, 1.0f, "%.4f");
                PropertyUInt("3D Velocity Iterations", &settings.Physics3DSettings.VelocityIterations, 1, 64);
                PropertyUInt("3D Position Iterations", &settings.Physics3DSettings.PositionIterations, 1, 64);
                UIPropertyBool("2D Physics", &settings.PhysicsEnabled2D);
                UIPropertyFloat("2D Gravity", &settings.Physics2DSettings.Gravity, 0.05f, 0.0f, 0.0f, "%.2f");
                UIPropertyBool("Audio", &settings.AudioEnabled);
                UIEndPropertyGrid();
            }

            UIPopUndoTarget();
            (void)context;
        }

        void DrawProjectSettings(const UIDockPanelContext& context)
        {
            Application::ProjectSettings& project = Application::Get().GetProjectSettings();

            // text fields edit a copy: the settings hold std::strings
            auto textRow = [](const char* label, std::string& value)
            {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s", value.c_str());
                if(UIPropertyText(label, buffer, sizeof(buffer)))
                    value = buffer;
            };

            if(UISectionHeader("Identity###projectidentity", true))
            {
                UIBeginPropertyGrid("##projectidentitygrid", 0.45f);
                UIPropertyLabel("Name", Str8C((char*)project.m_ProjectName.c_str()));
                textRow("Title", project.Title);
                textRow("Version", project.Version);
                textRow("Build Number", project.BuildNumber);
                textRow("Bundle Identifier", project.BundleIdentifier);
                textRow("Start Scene", project.StartScene);
                textRow("App Script", project.AppScript);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Window###projectwindow", true))
            {
                int width  = (int)project.Width;
                int height = (int)project.Height;
                UIBeginPropertyGrid("##projectwindowgrid", 0.45f);
                if(UIPropertyInt("Width", &width, 1.0f, 64, 16384))
                    project.Width = (u32)width;
                if(UIPropertyInt("Height", &height, 1.0f, 64, 16384))
                    project.Height = (u32)height;
                UIPropertyBool("Fullscreen", &project.Fullscreen);
                UIPropertyBool("VSync", &project.VSync);
                UIPropertyBool("Borderless", &project.Borderless);
                UIPropertyBool("Hide Title Bar", &project.HideTitleBar);
                UIPropertyBool("Show Console", &project.ShowConsole);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Assets###projectassets", true))
            {
                UIBeginPropertyGrid("##projectassetsgrid", 0.45f);
                UIPropertyBool("Auto Import Meshes", &project.AutoImportMeshes);
                textRow("Icon", project.IconPath);
                textRow("Splash", project.SplashPath);
                UIPropertyColour("Splash Background", project.SplashBGColour, false);
                UIEndPropertyGrid();
            }

            UISpacer(4.0f);
            if(UISectionHeader("Mobile###projectmobile", false))
            {
                static const char* kFamilies[] = { "iPhone", "iPad", "Both" };
                int family = Maths::Clamp(project.DeviceFamily - 1, 0, 2);
                UIBeginPropertyGrid("##projectmobilegrid", 0.45f);
                if(UIPropertyDropdown("Devices", &family, kFamilies, 3))
                    project.DeviceFamily = family + 1;
                textRow("Minimum iOS", project.MinIOSVersion);
                UIPropertyBool("Hide Status Bar", &project.StatusBarHidden);
                UIEndPropertyGrid();
            }
            (void)context;
        }

        void RegisterPanels(UIDockSpace* space, bool consoleAsDrawer)
        {
            Init();

            UIDockPanelDesc hierarchy;
            hierarchy.Name  = "Hierarchy";
            hierarchy.Title = ICON_MDI_FORMAT_LIST_BULLETED " Hierarchy";
            hierarchy.Draw  = &DrawHierarchy;
            hierarchy.Slot  = UIDockSlot_Left;
            hierarchy.Flags = UIDockPanel_NoScroll | UIDockPanel_NoPadding; // lists its rows itself
            UIDockRegisterPanel(space, hierarchy);

            UIDockPanelDesc inspector;
            inspector.Name  = "Inspector";
            inspector.Title = ICON_MDI_INFORMATION_OUTLINE " Inspector";
            inspector.Draw  = &DrawInspector;
            inspector.Slot  = UIDockSlot_Right;
            UIDockRegisterPanel(space, inspector);

            UIDockPanelDesc console;
            console.Name   = "Console";
            console.Title  = ICON_MDI_CONSOLE " Console";
            console.Draw   = &DrawConsole;
            console.Slot   = UIDockSlot_Bottom;
            console.Flags  = UIDockPanel_NoScroll | UIDockPanel_NoPadding;
            console.Drawer = consoleAsDrawer;
            UIDockRegisterPanel(space, console);

            UIDockPanelDesc stats;
            stats.Name  = "Stats";
            stats.Title = ICON_MDI_CHART_LINE " Stats";
            stats.Draw  = &DrawStats;
            stats.Slot  = UIDockSlot_RightBottom;
            stats.Open  = false;
            UIDockRegisterPanel(space, stats);

            UIDockPanelDesc assets;
            assets.Name   = "Assets";
            assets.Title  = ICON_MDI_FOLDER " Assets";
            assets.Draw   = &DrawAssets;
            assets.Slot   = UIDockSlot_Bottom;
            assets.Flags  = UIDockPanel_NoScroll | UIDockPanel_NoPadding;
            assets.Drawer = consoleAsDrawer;
            UIDockRegisterPanel(space, assets);

            UIDockPanelDesc sceneSettings;
            sceneSettings.Name  = "Scene Settings";
            sceneSettings.Title = ICON_MDI_TUNE " Scene Settings";
            sceneSettings.Draw  = &DrawSceneSettings;
            sceneSettings.Slot  = UIDockSlot_Right;
            sceneSettings.Open  = false;
            UIDockRegisterPanel(space, sceneSettings);

            UIDockPanelDesc projectSettings;
            projectSettings.Name  = "Project Settings";
            projectSettings.Title = ICON_MDI_WRENCH " Project Settings";
            projectSettings.Draw  = &DrawProjectSettings;
            projectSettings.Slot  = UIDockSlot_Right;
            projectSettings.Open  = false;
            UIDockRegisterPanel(space, projectSettings);

            UIDockPanelDesc sprite;
            sprite.Name  = "Sprite Editor";
            sprite.Title = ICON_MDI_PENCIL " Sprite Editor";
            sprite.Draw  = &SpriteEditor::DrawPanel;
            sprite.Slot  = UIDockSlot_Centre;
            sprite.Open  = false;
            sprite.Flags = UIDockPanel_NoScroll | UIDockPanel_NoPadding; // sizes its canvas itself
            UIDockRegisterPanel(space, sprite);
            SpriteEditor::SetDock(space);

            UIDockPanelDesc theme;
            theme.Name  = "Theme";
            theme.Title = ICON_MDI_PALETTE " Theme";
            theme.Draw  = &DrawTheme;
            theme.Slot  = UIDockSlot_Right;
            theme.Open  = false;
            UIDockRegisterPanel(space, theme);
        }
    }

    // ---- in-game workspace -------------------------------------------------

    namespace DebugWorkspace
    {
        namespace
        {
            UIDockSpace* s_Space = nullptr;
            bool s_Visible       = false;

            String8 LayoutPath(Arena* arena)
            {
                return PushStr8F(arena, "%sDebugWorkspace.json", Application::Get().WritableRoot().c_str());
            }

            void Save()
            {
                if(!s_Space)
                    return;
                ArenaTemp scratch = ScratchBegin(0, 0);
                UIDockSaveLayout(s_Space, LayoutPath(scratch.arena));
                ScratchEnd(scratch);
            }
        }

        void SetVisible(bool visible)
        {
            if(visible == s_Visible)
                return;

            if(visible && !s_Space)
            {
                s_Space = UIDockSpaceCreate("debugworkspace");
                UIDockSetCentralPassthrough(s_Space, true);
                ToolPanels::RegisterPanels(s_Space, true);

                ArenaTemp scratch = ScratchBegin(0, 0);
                UIDockLoadLayout(s_Space, LayoutPath(scratch.arena));
                ScratchEnd(scratch);
            }

            if(!visible)
                Save();
            s_Visible = visible;
        }

        bool IsVisible() { return s_Visible; }
        void Toggle() { SetVisible(!s_Visible); }
        UIDockSpace* GetDockSpace() { return s_Space; }

        void OnUI()
        {
            if(!s_Visible || !s_Space)
                return;
            const UI_Widget& root = GetUIState()->root_parent;
            UIDockSpaceDraw(s_Space, Vec4(root.position.x, root.position.y, root.size.x, root.size.y));
        }

        void Shutdown()
        {
            if(s_Visible)
                Save();
            UIDockSpaceDestroy(s_Space);
            s_Space   = nullptr;
            s_Visible = false;
            ToolPanels::ReleaseThumbnails();
        }
    }
}
