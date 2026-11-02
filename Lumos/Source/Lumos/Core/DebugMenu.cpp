#include "Precompiled.h"
#include "DebugMenu.h"

#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/LMLog.h"
#include "Core/OS/Input.h"
#include "Core/OS/OS.h"
#include "Core/OS/Memory.h"
#include "Core/OS/Window.h"
#include "Graphics/UI.h"
#include "Maths/MathsUtilities.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Audio/AudioManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/EntityManager.h"
#include "Scene/Entity.h"
#include "Scene/SceneGraph.h"
#include "Scene/Component/Components.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/Light.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/SceneRenderer.h"
#include "Graphics/Gizmo.h"
#include "Core/Undo.h"
#include "Scene/SceneUndo.h"
#include "Core/ToolPanels.h"
#include "Maths/Transform.h"
#include "Maths/BoundingBox.h"
#include "Maths/Rect.h"
#include "ImGui/IconsMaterialDesignIcons.h"

namespace Lumos
{
    namespace
    {
        constexpr float kPaletteWidth  = 660.0f; // logical px, clamped to the viewport
        constexpr float kRowHeight     = 30.0f;
        constexpr float kRowSpacing    = 2.0f;
        constexpr u32 kMaxResults      = 200;

        inline char LowerChar(char c) { return (c >= 'A' && c <= 'Z') ? char(c + 32) : c; }

        inline bool IsWordBreak(char c)
        {
            return c == ' ' || c == ':' || c == '/' || c == '-' || c == '_' || c == '.';
        }

        // Subsequence match with sublime-style bonuses. Returns false when a
        // pattern character can't be found in order.
        bool FuzzyMatch(const char* pattern, const char* str, i32& outScore, u64 maskBits[2], u32 maskOffset)
        {
            outScore    = 0;
            maskBits[0] = 0;
            maskBits[1] = 0;

            if(!pattern || !pattern[0])
                return true;

            i32 score       = 0;
            u32 si          = 0;
            u32 lastMatch   = 0;
            bool firstMatch = true;

            for(u32 pi = 0; pattern[pi]; pi++)
            {
                const char pc = LowerChar(pattern[pi]);
                if(pc == ' ')
                    continue;

                bool found = false;
                while(str[si])
                {
                    if(LowerChar(str[si]) == pc)
                    {
                        found = true;
                        break;
                    }
                    si++;
                }
                if(!found)
                    return false;

                if(firstMatch)
                {
                    score -= (i32)si * 2; // matches near the front rank higher
                    if(si == 0)
                        score += 20;
                    firstMatch = false;
                }
                else if(si == lastMatch + 1)
                {
                    score += 12; // consecutive run
                }
                else
                {
                    score -= (i32)(si - lastMatch - 1);
                }

                if(si > 0 && IsWordBreak(str[si - 1]))
                    score += 15; // start of a word

                if(str[si] == pattern[pi])
                    score += 2; // exact case

                if(si >= maskOffset)
                {
                    const u32 bit = si - maskOffset;
                    if(bit < 128)
                        maskBits[bit >> 6] |= (1ull << (bit & 63));
                }

                lastMatch = si;
                si++;
            }

            score += 10; // matched at all
            outScore = score;
            return true;
        }

        Vec4 LevelColour(LogLevel level)
        {
            switch(level)
            {
            case LogLevel::Warning: return Vec4(1.0f, 0.78f, 0.35f, 1.0f);
            case LogLevel::Error:
            case LogLevel::Fatal:   return Vec4(1.0f, 0.45f, 0.42f, 1.0f);
            case LogLevel::Trace:   return Vec4(0.62f, 0.72f, 0.85f, 1.0f);
            default:                return Vec4(0.82f, 0.84f, 0.88f, 1.0f);
            }
        }

        const Vec4 kAccent     = Vec4(0.42f, 0.72f, 1.00f, 1.0f);
        const Vec4 kBadgeBG    = Vec4(1.0f, 1.0f, 1.0f, 0.10f);
        const Vec4 kBadgeText  = Vec4(0.80f, 0.84f, 0.90f, 1.0f);
        const Vec4 kOnBG       = Vec4(0.25f, 0.70f, 0.42f, 0.85f);
        const Vec4 kOffBG      = Vec4(1.0f, 1.0f, 1.0f, 0.08f);
        const Vec4 kDimText    = Vec4(0.70f, 0.73f, 0.78f, 0.75f);
        const Vec4 kPanelBG    = Vec4(0.05f, 0.06f, 0.09f, 0.82f);

        Scene* CurrentScene()
        {
            return Application::Get().GetSceneManager() ? Application::Get().GetSceneManager()->GetCurrentScene() : nullptr;
        }

        Scene::SceneRenderSettings* RenderSettings()
        {
            Scene* scene = CurrentScene();
            return scene ? &scene->GetSettings().RenderSettings : nullptr;
        }

        // ---- scene / camera helpers ----
        struct ActiveCam
        {
            Camera* cam            = nullptr;
            Maths::Transform* tf   = nullptr;
            entt::entity entity    = entt::null;
        };

        ActiveCam GetActiveCam()
        {
            ActiveCam out;
            Scene* scene = CurrentScene();
            if(!scene)
                return out;
            // Same pick as SceneRenderer::BeginScene - a multi-component view can
            // order differently, and driving a camera that isn't being rendered
            // looks exactly like the fly camera being broken.
            auto& reg = scene->GetRegistry();
            auto view = reg.view<Camera>();
            if(view.empty())
                return out;
            out.entity = view.front();
            out.cam    = &view.get<Camera>(out.entity);
            out.tf     = reg.try_get<Maths::Transform>(out.entity);
            if(!out.tf)
                out.cam = nullptr;
            return out;
        }

        // A local-transform write only reaches the renderer through the scene
        // graph pass, which has already run by the time the overlay updates -
        // GetWorldMatrix() just returns the cached matrix. Refresh it here or the
        // edit is invisible this frame, and gone the next once the game rewrites
        // its own local transform.
        void RefreshWorldMatrix(entt::registry& reg, entt::entity e, Maths::Transform* tf)
        {
            if(!tf)
                return;
            if(Hierarchy* h = reg.try_get<Hierarchy>(e))
            {
                const entt::entity parent = h->Parent();
                if(parent != entt::null && reg.valid(parent))
                {
                    if(Maths::Transform* parentTf = reg.try_get<Maths::Transform>(parent))
                    {
                        tf->SetWorldMatrix(parentTf->GetWorldMatrix());
                        return;
                    }
                }
            }
            tf->SetWorldMatrix();
        }

        Vec2 UIMouse()
        {
            UI_State* ui = GetUIState();
            return Input::Get().GetMousePosition() * ui->InputScale - ui->InputOffset;
        }

        // Layout-space pixels, matching UIMouse (y down). Mirrors the flipY
        // convention in Camera::GetScreenRay.
        bool WorldToScreen(ActiveCam& c, const Vec3& world, Vec2& out)
        {
            if(!c.cam || !c.tf)
                return false;

            const Mat4 viewProj = c.cam->GetProjectionMatrix() * c.tf->GetWorldMatrix().Inverse();
            const Vec4 clip     = viewProj * Vec4(world.x, world.y, world.z, 1.0f);
            if(Maths::Abs(clip.w) < 0.00001f)
                return false;

            const Vec3 ndc = Vec3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
            const Vec2 fb  = GetUIState()->FrameBufferSize;
            out            = Vec2((ndc.x * 0.5f + 0.5f) * fb.x, (0.5f - ndc.y * 0.5f) * fb.y);
            return true;
        }

        // Pixels covered by one world unit at `world`, for constant-size handles.
        float PixelsPerWorldUnit(ActiveCam& c, const Vec3& world)
        {
            Vec2 a, b;
            if(!WorldToScreen(c, world, a))
                return 0.0f;
            if(!WorldToScreen(c, world + c.tf->GetRightDirection(), b))
                return 0.0f;
            return (b - a).Length();
        }

        float DistanceToSegment(const Vec2& p, const Vec2& a, const Vec2& b)
        {
            const Vec2 ab = b - a;
            const float len2 = ab.x * ab.x + ab.y * ab.y;
            if(len2 < 0.0001f)
                return (p - a).Length();
            float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
            t       = Maths::Clamp(t, 0.0f, 1.0f);
            return (p - (a + ab * t)).Length();
        }

        String8 EntityLabel(Arena* arena, entt::registry& reg, entt::entity e)
        {
            NameComponent* name = reg.try_get<NameComponent>(e);
            if(name && !name->name.empty())
                return PushStr8F(arena, "%s", name->name.c_str());
            return PushStr8F(arena, "Entity %u", (u32)entt::to_integral(e));
        }

    // World bounds of whatever the entity actually renders. Returns false when
    // there is nothing with a size to draw around.
    static bool SelectionBounds(entt::registry& reg, entt::entity e, Maths::BoundingBox& out)
    {
        Maths::Transform* tf = reg.try_get<Maths::Transform>(e);
        if(!tf)
            return false;

        const Mat4 world = tf->GetWorldMatrix();
        bool any         = false;

        if(Graphics::ModelComponent* model = reg.try_get<Graphics::ModelComponent>(e))
        {
            if(model->ModelRef)
            {
                for(auto& mesh : model->ModelRef->GetMeshes())
                {
                    if(!mesh)
                        continue;
                    Maths::BoundingBox bb = mesh->GetBoundingBox().Transformed(world);
                    if(!any)
                    {
                        out = bb;
                        any = true;
                    }
                    else
                        out.Merge(bb);
                }
            }
        }

        if(Graphics::Sprite* sprite = reg.try_get<Graphics::Sprite>(e))
        {
            Maths::BoundingBox bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetScale())).Transformed(world);
            if(!any) { out = bb; any = true; }
            else out.Merge(bb);
        }

        if(Graphics::AnimatedSprite* sprite = reg.try_get<Graphics::AnimatedSprite>(e))
        {
            Maths::BoundingBox bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetScale())).Transformed(world);
            if(!any) { out = bb; any = true; }
            else out.Merge(bb);
        }

        if(!any)
        {
            if(Graphics::Light* light = reg.try_get<Graphics::Light>(e))
            {
                const Vec3 p = tf->GetWorldPosition();
                const float r = Maths::Max(0.1f, light->Radius);
                out           = Maths::BoundingBox(p - Vec3(r), p + Vec3(r));
                any           = true;
            }
        }

        return any;
    }

    }

    void DebugMenu::Init()
    {
        if(m_Initialised)
            return;

        m_Arena       = ArenaAlloc(Kilobytes(256));
        m_Initialised = true;
        m_ToggleKey   = (i32)InputCode::Key::F1;

        for(u32 i = 0; i < kFrameHistory; i++)
            m_FrameTimes[i] = 16.0f;

        RegisterBuiltIns();
    }

    void DebugMenu::Shutdown()
    {
        DebugWorkspace::Shutdown();
        m_Commands.Clear();
        m_Results.Clear();
        if(m_Arena)
        {
            ArenaRelease(m_Arena);
            m_Arena = nullptr;
        }
        m_Initialised = false;
    }

    DebugCommand* DebugMenu::Find(const char* name)
    {
        String8 needle = Str8C((char*)name);
        for(size_t i = 0; i < m_Commands.Size(); i++)
        {
            if(Str8Match(m_Commands[i].Name, needle, MatchFlags(0)))
                return &m_Commands[i];
        }
        return nullptr;
    }

    void DebugMenu::RegisterAction(const char* name, const char* category, Function<void()> action, const char* hint, i32 shortcutKey)
    {
        if(!m_Initialised)
            Init();

        DebugCommand* existing = Find(name);
        DebugCommand cmd;
        cmd.Name     = PushStr8Copy(m_Arena, name);
        cmd.Category = PushStr8Copy(m_Arena, category ? category : "General");
        cmd.Hint     = hint ? PushStr8Copy(m_Arena, hint) : String8{ 0 };
        cmd.Type     = DebugCommandType::Action;
        cmd.Action   = action;
        cmd.Shortcut = shortcutKey;
        cmd.IsScript = m_NextIsScript;

        if(existing)
            *existing = cmd;
        else
            m_Commands.PushBack(cmd);

        m_LastQuery[0] = '\x01'; // force the result list to rebuild
    }

    void DebugMenu::RegisterToggle(const char* name, const char* category, Function<bool()> get, Function<void(bool)> set, i32 shortcutKey)
    {
        if(!m_Initialised)
            Init();

        DebugCommand* existing = Find(name);
        DebugCommand cmd;
        cmd.Name     = PushStr8Copy(m_Arena, name);
        cmd.Category = PushStr8Copy(m_Arena, category ? category : "General");
        cmd.Type     = DebugCommandType::Toggle;
        cmd.GetBool  = get;
        cmd.SetBool  = set;
        cmd.Shortcut = shortcutKey;
        cmd.IsScript = m_NextIsScript;

        if(existing)
            *existing = cmd;
        else
            m_Commands.PushBack(cmd);

        m_LastQuery[0] = '\x01';
    }

    void DebugMenu::RegisterFloat(const char* name, const char* category, Function<float()> get, Function<void(float)> set, float minValue, float maxValue)
    {
        if(!m_Initialised)
            Init();

        DebugCommand* existing = Find(name);
        DebugCommand cmd;
        cmd.Name     = PushStr8Copy(m_Arena, name);
        cmd.Category = PushStr8Copy(m_Arena, category ? category : "General");
        cmd.Type     = DebugCommandType::Float;
        cmd.GetFloat = get;
        cmd.SetFloat = set;
        cmd.Min      = minValue;
        cmd.Max      = maxValue;
        cmd.IsScript = m_NextIsScript;

        if(existing)
            *existing = cmd;
        else
            m_Commands.PushBack(cmd);

        m_LastQuery[0] = '\x01';
    }

    void DebugMenu::RegisterBool(const char* name, const char* category, bool* value)
    {
        RegisterToggle(
            name, category,
            [value]() { return *value; },
            [value](bool v) { *value = v; });
    }

    void DebugMenu::RegisterFloatPtr(const char* name, const char* category, float* value, float minValue, float maxValue)
    {
        RegisterFloat(
            name, category,
            [value]() { return *value; },
            [value](float v) { *value = v; },
            minValue, maxValue);
    }

    void DebugMenu::Unregister(const char* name)
    {
        String8 needle = Str8C((char*)name);
        m_Commands.RemoveIf([&needle](const DebugCommand& c)
                            { return Str8Match(c.Name, needle, MatchFlags(0)); });
        m_Results.Clear();
        m_LastQuery[0] = '\x01';
    }

    void DebugMenu::UnregisterScriptCommands()
    {
        m_Commands.RemoveIf([](const DebugCommand& c) { return c.IsScript; });
        m_Results.Clear();
        m_Selected     = 0;
        m_LastQuery[0] = '\x01';
    }

    bool DebugMenu::Run(const char* name)
    {
        DebugCommand* cmd = Find(name);
        if(!cmd)
            return false;
        RunCommand(*cmd);
        return true;
    }

    void DebugMenu::RunCommandAt(u32 index)
    {
        if(index < m_Commands.Size())
            RunCommand(m_Commands[index]);
    }

    bool DebugMenu::IsCommandOn(u32 index)
    {
        if(index >= m_Commands.Size())
            return false;
        DebugCommand& command = m_Commands[index];
        return command.Type == DebugCommandType::Toggle && command.GetBool && command.GetBool();
    }

    bool DebugMenu::SetToggle(const char* name, bool value)
    {
        DebugCommand* cmd = Find(name);
        if(!cmd || cmd->Type != DebugCommandType::Toggle || !cmd->SetBool)
            return false;
        cmd->SetBool(value);
        return true;
    }

    void DebugMenu::RunCommand(DebugCommand& cmd)
    {
        cmd.UseCount++;
        cmd.LastUsed = m_UseCounter++;

        switch(cmd.Type)
        {
        case DebugCommandType::Action:
            if(cmd.Action)
                cmd.Action();
            snprintf(m_ToastText, sizeof(m_ToastText), "%.*s", (int)cmd.Name.size, (const char*)cmd.Name.str);
            break;
        case DebugCommandType::Toggle:
            if(cmd.GetBool && cmd.SetBool)
            {
                const bool next = !cmd.GetBool();
                cmd.SetBool(next);
                snprintf(m_ToastText, sizeof(m_ToastText), "%.*s  %s", (int)cmd.Name.size, (const char*)cmd.Name.str, next ? "ON" : "OFF");
            }
            break;
        case DebugCommandType::Float:
            if(cmd.GetFloat)
                snprintf(m_ToastText, sizeof(m_ToastText), "%.*s  %.3g",
                         (int)cmd.Name.size, (const char*)cmd.Name.str, cmd.GetFloat());
            break; // value itself is changed with the arrow keys / tweak slider
        }
        m_ToastTime = 1.8f;
    }

    void DebugMenu::OpenPalette()
    {
        m_PaletteOpen  = true;
        m_Query[0]     = 0;
        m_LastQuery[0] = '\x01';
        m_Selected     = 0;
        m_ScrolledTo   = 0;
        m_Scroll       = 0.0f;
        m_ClaimFocus   = true;
    }

    void DebugMenu::ClosePalette()
    {
        m_PaletteOpen = false;
        if(m_BlockedKeyboard)
        {
            Input::Get().SetKeyboardBlocked(false);
            m_BlockedKeyboard = false;
        }
    }

    void DebugMenu::SetEnabled(bool enabled)
    {
        if(enabled == m_Enabled)
            return;
        m_Enabled = enabled;
        if(enabled)
            return;

        // switched off (the editor leaving play mode): hand everything back as it was
        ClosePalette();
        SetFlyCamera(false);
        for(u32 i = 0; i < DebugPanel_Count; i++)
            m_Panels[i] = false;
        m_StepFrames = 0;
        Engine::GetTimeStep().SetTimeScale(1.0);
        if(m_ExplorerWasOpen)
        {
            m_ExplorerWasOpen = false;
            SetPassCutoff(-1);
            if(Scene::SceneRenderSettings* rs = RenderSettings())
                rs->DebugMode = 0;
        }
        if(m_PhysicsWasOpen)
        {
            m_PhysicsWasOpen = false;
            if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
            {
                physics->SetManualStep(false);
                physics->SetDebugDrawFlags(m_PhysicsSavedFlags);
            }
        }
    }

    void DebugMenu::TogglePalette()
    {
        if(m_PaletteOpen)
            ClosePalette();
        else
            OpenPalette();
    }

    void DebugMenu::SetPanelVisible(DebugPanel panel, bool visible)
    {
        if(panel < DebugPanel_Count)
            m_Panels[panel] = visible;
    }

    Graphics::SceneRenderer* DebugMenu::ExplorerRenderer() const
    {
        return m_ExplorerRenderer ? m_ExplorerRenderer : Graphics::SceneRenderer::GetGameRenderer();
    }

    void DebugMenu::DrawFrameExplorerDocked(Graphics::SceneRenderer* renderer, const Vec2& size)
    {
        // a different renderer than last time: the old one goes back to a whole frame
        if(m_ExplorerRenderer && m_ExplorerRenderer != renderer)
            m_ExplorerRenderer->SetPassCutoff(-1);
        m_ExplorerRenderer = renderer;
        m_DockedSize       = size;
        DrawFrameExplorer();
        m_DockedSize = Vec2(0.0f);
    }

    void DebugMenu::DrawPhysicsExplorerDocked(const Vec2& size)
    {
        m_DockedSize = size;
        DrawPhysicsExplorer();
        m_DockedSize = Vec2(0.0f);
    }

    void DebugMenu::CloseFrameExplorer()
    {
        if(!m_ExplorerWasOpen)
            return;
        m_ExplorerWasOpen = false;
        SetPassCutoff(-1);
        if(Scene::SceneRenderSettings* rs = RenderSettings())
            rs->DebugMode = 0;
        m_ExplorerRenderer = nullptr;
    }

    void DebugMenu::ClosePhysicsExplorer()
    {
        if(!m_PhysicsWasOpen)
            return;
        m_PhysicsWasOpen = false;
        if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
        {
            physics->SetManualStep(false);
            physics->SetDebugDrawFlags(m_PhysicsSavedFlags);
        }
    }

    void DebugMenu::SetPassCutoff(i32 pass)
    {
        if(Graphics::SceneRenderer* renderer = ExplorerRenderer())
            renderer->SetPassCutoff(pass);
    }

    i32 DebugMenu::GetPassCutoff() const
    {
        Graphics::SceneRenderer* renderer = ExplorerRenderer();
        return renderer ? renderer->GetPassCutoff() : -1;
    }

    void DebugMenu::StepPass(i32 direction)
    {
        Graphics::SceneRenderer* renderer = ExplorerRenderer();
        if(!renderer || direction == 0)
            return;

        direction = direction > 0 ? 1 : -1;
        i32 pass  = renderer->GetPassCutoff();
        if(pass < 0)
            pass = direction > 0 ? -1 : (i32)Graphics::RenderPass_Count;

        // passes that are off this frame would be a step with nothing to see
        for(pass += direction; pass >= 0 && pass < (i32)Graphics::RenderPass_Count; pass += direction)
        {
            if(renderer->GetPassRecord((Graphics::RenderPassID)pass).State != Graphics::RenderPassState::Skipped)
            {
                renderer->SetPassCutoff(pass);
                return;
            }
        }
    }

    void DebugMenu::RefreshResults()
    {
        m_Results.Clear();

        // Query is matched against "Category Name" so "phy coll" finds
        // "Physics 3D / Collision Volumes"; highlights only cover the name.
        char combined[192];
        for(size_t i = 0; i < m_Commands.Size(); i++)
        {
            const DebugCommand& cmd = m_Commands[i];

            const int catLen = (int)Maths::Min<u64>(cmd.Category.size, 80);
            const int nameLen = (int)Maths::Min<u64>(cmd.Name.size, 100);
            snprintf(combined, sizeof(combined), "%.*s %.*s",
                     catLen, (const char*)cmd.Category.str,
                     nameLen, (const char*)cmd.Name.str);

            i32 score = 0;
            u64 mask[2];
            if(!FuzzyMatch(m_Query, combined, score, mask, (u32)catLen + 1))
                continue;

            // Recently used bubbles up, especially with an empty query.
            score += (i32)Maths::Min<u32>(cmd.UseCount, 5) * 4;
            if(cmd.LastUsed)
                score += 3;

            Result r;
            r.index       = (u32)i;
            r.score       = score;
            r.maskBits[0] = mask[0];
            r.maskBits[1] = mask[1];
            m_Results.PushBack(r);
        }

        // Insertion sort - the list is short and mostly ordered frame to frame.
        for(size_t i = 1; i < m_Results.Size(); i++)
        {
            Result key = m_Results[i];
            size_t j   = i;
            while(j > 0 && m_Results[j - 1].score < key.score)
            {
                m_Results[j] = m_Results[j - 1];
                j--;
            }
            m_Results[j] = key;
        }

        if(m_Selected >= (i32)m_Results.Size())
            m_Selected = (i32)m_Results.Size() - 1;
        if(m_Selected < 0)
            m_Selected = 0;
    }

    void DebugMenu::OnUpdate(float dt)
    {
        if(!m_Initialised)
            Init();

        // Frame time history (raw, so spikes are visible).
        const float ms = (float)Engine::GetTimeStep().GetRawMillis();
        m_FrameTimes[m_FrameCursor] = ms;
        m_FrameCursor               = (m_FrameCursor + 1) % kFrameHistory;
        m_SmoothedMs                = m_SmoothedMs * 0.92f + ms * 0.08f;

        if(m_ToastTime > 0.0f)
            m_ToastTime -= dt;

        // "Step One Frame" runs one update at normal speed, then pauses again.
        if(m_StepFrames > 0)
        {
            m_StepFrames--;
            if(m_StepFrames == 0)
                Engine::GetTimeStep().SetTimeScale(0.0);
        }

        if(!m_Enabled)
        {
            if(m_BlockedKeyboard)
            {
                Input::Get().SetKeyboardBlocked(false);
                m_BlockedKeyboard = false;
            }
            return;
        }

        Input& input = Input::Get();

        const bool ctrl = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
            || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);

        const bool openPressed = (m_ToggleKey >= 0 && input.GetKeyPressedRaw((InputCode::Key)m_ToggleKey))
            || (ctrl && input.GetKeyPressedRaw(InputCode::Key::P));

        if(openPressed)
            TogglePalette();

        // Edits made through the overlay (gizmo, inspector) can be stepped back.
        if(ctrl && !m_PaletteOpen && GetUIState()->FocusedTextInput == 0 && Application::Get().GetAppType() != AppType::Editor)
        {
            const bool shift = input.GetKeyHeldRaw(InputCode::Key::LeftShift) || input.GetKeyHeldRaw(InputCode::Key::RightShift);
            if(input.GetKeyPressedRaw(InputCode::Key::Z))
            {
                if(shift)
                    Lumos::Redo();
                else
                    Lumos::Undo();
            }
            else if(input.GetKeyPressedRaw(InputCode::Key::Y))
                Lumos::Redo();
        }

        // Bound shortcuts fire whenever the palette is closed.
        if(!m_PaletteOpen)
        {
            for(size_t i = 0; i < m_Commands.Size(); i++)
            {
                DebugCommand& cmd = m_Commands[i];
                if(cmd.Shortcut >= 0 && cmd.Shortcut != m_ToggleKey && input.GetKeyPressedRaw((InputCode::Key)cmd.Shortcut))
                    RunCommand(cmd);
            }
        }

        if(m_PaletteOpen)
        {
            if(input.GetKeyPressedRaw(InputCode::Key::Escape))
            {
                ClosePalette();
            }
            else
            {
                const i32 count = (i32)m_Results.Size();
                if(count > 0 && m_Results[Maths::Clamp(m_Selected, 0, count - 1)].index < m_Commands.Size())
                {
                    if(input.GetKeyPressedRaw(InputCode::Key::Down))
                        m_Selected = (m_Selected + 1) % count;
                    if(input.GetKeyPressedRaw(InputCode::Key::Up))
                        m_Selected = (m_Selected + count - 1) % count;
                    if(input.GetKeyPressedRaw(InputCode::Key::PageDown))
                        m_Selected = Maths::Min(m_Selected + 8, count - 1);
                    if(input.GetKeyPressedRaw(InputCode::Key::PageUp))
                        m_Selected = Maths::Max(m_Selected - 8, 0);

                    const u32 cmdIndex = m_Results[m_Selected].index;
                    if(cmdIndex >= m_Commands.Size())
                        return; // list changed under us mid-frame; next frame rebuilds it
                    DebugCommand& cmd = m_Commands[cmdIndex];

                    if(cmd.Type == DebugCommandType::Float && cmd.GetFloat && cmd.SetFloat)
                    {
                        const float step = (cmd.Max - cmd.Min) / 20.0f;
                        if(input.GetKeyPressedRaw(InputCode::Key::Right))
                            cmd.SetFloat(Maths::Clamp(cmd.GetFloat() + step, cmd.Min, cmd.Max));
                        if(input.GetKeyPressedRaw(InputCode::Key::Left))
                            cmd.SetFloat(Maths::Clamp(cmd.GetFloat() - step, cmd.Min, cmd.Max));
                    }

                    if(input.GetKeyPressedRaw(InputCode::Key::Enter) || input.GetKeyPressedRaw(InputCode::Key::KPEnter))
                    {
                        RunCommand(cmd);
                        // Actions close the palette; toggles stay open so a few
                        // can be flipped in a row.
                        if(cmd.Type == DebugCommandType::Action)
                            ClosePalette();
                    }
                }
            }
        }

        // The palette owns the keyboard while it is up; so does the fly camera,
        // otherwise WASD drives the player as well as the camera. Same while a
        // field in the entity panels has focus - typing a filter shouldn't also
        // play the game.
        const bool typingInPanel = GetUIState()->FocusedTextInput != 0
            && (m_Panels[DebugPanel_Entities] || m_Panels[DebugPanel_Inspector] || DebugWorkspace::IsVisible());
        const bool wantBlock = m_PaletteOpen || m_FlyCamera || typingInPanel;
        if(wantBlock != m_BlockedKeyboard)
        {
            Input::Get().SetKeyboardBlocked(wantBlock);
            m_BlockedKeyboard = wantBlock;
        }
    }

    void DebugMenu::OnUI()
    {
        if(!m_Enabled || !m_Initialised)
            return;

        // under the HUD panels and the palette, so those stay on top
        DebugWorkspace::OnUI();

        if(m_Panels[DebugPanel_Stats])
            DrawStats();
        if(m_Panels[DebugPanel_FrameGraph])
            DrawFrameGraph();
        if(m_Panels[DebugPanel_Log])
            DrawLog();
        if(m_Panels[DebugPanel_Toggles])
            DrawToggles();
        if(m_Panels[DebugPanel_Tweaks])
            DrawTweaks();
        // the docked tree and inspector replace these two
        if(m_Panels[DebugPanel_Entities] && !DebugWorkspace::IsVisible())
            DrawEntities();
        if(m_Panels[DebugPanel_Inspector] && !DebugWorkspace::IsVisible())
            DrawInspector();
        if(m_Panels[DebugPanel_FrameExplorer])
            DrawFrameExplorer();
        else if(m_ExplorerWasOpen && !m_ExplorerRenderer)
            CloseFrameExplorer(); // back to a normal frame (a docked one is closed by whoever docks it)

        if(m_Panels[DebugPanel_PhysicsExplorer])
            DrawPhysicsExplorer();
        else if(m_PhysicsWasOpen)
        {
            // closed: hand the simulation back as it was
            m_PhysicsWasOpen = false;
            if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
            {
                physics->SetManualStep(false);
                physics->SetDebugDrawFlags(m_PhysicsSavedFlags);
            }
        }

        if(m_PaletteOpen)
            DrawPalette();

        if(m_FlyCamera)
            DrawFlyHint();

        if(m_ToastTime > 0.0f && m_ToastText[0])
            DrawToast();
    }

    void DebugMenu::RegisterBuiltIns()
    {
        // ---- Overlay panels ----
        auto panelToggle = [this](const char* name, DebugPanel panel, i32 key)
        {
            RegisterToggle(
                name, "Overlay",
                [this, panel]() { return m_Panels[panel]; },
                [this, panel](bool v) { m_Panels[panel] = v; },
                key);
        };

        panelToggle("Stats HUD", DebugPanel_Stats, (i32)InputCode::Key::F2);
        panelToggle("Frame Graph", DebugPanel_FrameGraph, (i32)InputCode::Key::F3);
        panelToggle("Console", DebugPanel_Log, (i32)InputCode::Key::F4);
        panelToggle("Debug Draw Panel", DebugPanel_Toggles, (i32)InputCode::Key::F5);
        panelToggle("Tweaks Panel", DebugPanel_Tweaks, (i32)InputCode::Key::F6);
        panelToggle("Entity List", DebugPanel_Entities, (i32)InputCode::Key::F7);
        panelToggle("Inspector", DebugPanel_Inspector, (i32)InputCode::Key::F8);
        // not F11: macOS takes it for Show Desktop before the app sees it
        panelToggle("Frame Explorer", DebugPanel_FrameExplorer, (i32)InputCode::Key::Backslash);

        // ---- Learn ----
        RegisterAction("Next Render Pass", "Learn", [this]()
                       {
                           m_Panels[DebugPanel_FrameExplorer] = true;
                           StepPass(1);
                       }, "]", (i32)InputCode::Key::RightBracket);
        RegisterAction("Previous Render Pass", "Learn", [this]()
                       {
                           m_Panels[DebugPanel_FrameExplorer] = true;
                           StepPass(-1);
                       }, "[", (i32)InputCode::Key::LeftBracket);
        RegisterAction("Show Full Frame", "Learn", [this]() { SetPassCutoff(-1); });

        panelToggle("Physics Explorer", DebugPanel_PhysicsExplorer, (i32)InputCode::Key::F12);

        // Docked tools around the game: entity tree, inspector, console drawer.
        RegisterToggle(
            "Workspace", "Overlay",
            []() { return DebugWorkspace::IsVisible(); },
            [](bool v) { DebugWorkspace::SetVisible(v); },
            (i32)InputCode::Key::F11);
        RegisterAction("Step Physics Stage", "Learn", [this]()
                       {
                           m_Panels[DebugPanel_PhysicsExplorer] = true;
                           if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
                           {
                               physics->SetManualStep(true);
                               physics->StepStages(1);
                           }
                       });
        RegisterAction("Step Physics Substep", "Learn", [this]()
                       {
                           m_Panels[DebugPanel_PhysicsExplorer] = true;
                           if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
                           {
                               physics->SetManualStep(true);
                               physics->StepSubstep();
                           }
                       });
        RegisterAction("Resume Physics", "Learn", []()
                       {
                           if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
                               physics->SetManualStep(false);
                       });

        RegisterAction("Hide All Overlays", "Overlay", [this]()
                       {
                           for(u32 i = 0; i < DebugPanel_Count; i++)
                               m_Panels[i] = false;
                       });

        // ---- Renderer ----
        auto renderToggle = [this](const char* name, bool Scene::SceneRenderSettings::* member)
        {
            RegisterToggle(
                name, "Render",
                [member]()
                {
                    Scene::SceneRenderSettings* rs = RenderSettings();
                    return rs ? rs->*member : false;
                },
                [member](bool v)
                {
                    Scene::SceneRenderSettings* rs = RenderSettings();
                    if(rs)
                        rs->*member = v;
                });
        };

        renderToggle("3D Renderer", &Scene::SceneRenderSettings::Renderer3DEnabled);
        renderToggle("2D Renderer", &Scene::SceneRenderSettings::Renderer2DEnabled);
        renderToggle("Debug Lines", &Scene::SceneRenderSettings::DebugRenderEnabled);
        renderToggle("Skybox", &Scene::SceneRenderSettings::SkyboxRenderEnabled);
        renderToggle("Shadows", &Scene::SceneRenderSettings::ShadowsEnabled);
        renderToggle("Depth Pre-Pass", &Scene::SceneRenderSettings::DepthPrePass);
        renderToggle("Occlusion Culling", &Scene::SceneRenderSettings::OcclusionCulling);
        renderToggle("Bloom", &Scene::SceneRenderSettings::BloomEnabled);
        renderToggle("SSAO", &Scene::SceneRenderSettings::SSAOEnabled);
        renderToggle("SSR", &Scene::SceneRenderSettings::SSREnabled);
        renderToggle("FXAA", &Scene::SceneRenderSettings::FXAAEnabled);
        renderToggle("SMAA", &Scene::SceneRenderSettings::SMAAEnabled);
        renderToggle("TAA", &Scene::SceneRenderSettings::TAAEnabled);
        renderToggle("Contact Shadows", &Scene::SceneRenderSettings::ContactShadowsEnabled);
        renderToggle("Volumetric Fog", &Scene::SceneRenderSettings::VolumetricFogEnabled);
        renderToggle("Global Illumination", &Scene::SceneRenderSettings::GIEnabled);
        renderToggle("Depth of Field", &Scene::SceneRenderSettings::DepthOfFieldEnabled);
        renderToggle("Motion Blur", &Scene::SceneRenderSettings::MotionBlurEnabled);
        renderToggle("Vignette", &Scene::SceneRenderSettings::VignetteEnabled);
        renderToggle("Filmic Grain", &Scene::SceneRenderSettings::FilmicGrainEnabled);
        renderToggle("Chromatic Aberration", &Scene::SceneRenderSettings::ChromaticAberationEnabled);
        renderToggle("Sharpen", &Scene::SceneRenderSettings::SharpenEnabled);
        renderToggle("Debanding", &Scene::SceneRenderSettings::DebandingEnabled);

        RegisterFloat(
            "Exposure", "Render",
            []()
            {
                Scene::SceneRenderSettings* rs = RenderSettings();
                return rs ? rs->m_Exposure : 1.0f;
            },
            [](float v)
            {
                Scene::SceneRenderSettings* rs = RenderSettings();
                if(rs)
                    rs->m_Exposure = v;
            },
            0.0f, 5.0f);

        RegisterFloat(
            "Renderer Scale", "Render",
            []() { return Application::Get().GetQualitySettings().RendererScale; },
            [](float v)
            {
                Application::Get().GetQualitySettings().DynamicResolution = false;
                Application::Get().GetQualitySettings().RendererScale = Maths::Clamp(v, 0.25f, 2.0f);
                Application::Get().OnSceneViewSizeUpdated(Application::Get().GetWindow()->GetWidth(),
                                                          Application::Get().GetWindow()->GetHeight());
            },
            0.25f, 2.0f);

        RegisterToggle(
            "Auto Resolution", "Render",
            []() { return Application::Get().GetQualitySettings().DynamicResolution; },
            [](bool v) { Application::Get().GetQualitySettings().DynamicResolution = v; });

        RegisterFloat(
            "Quality Level", "Render",
            []() { return (float)Application::Get().GetQualitySettings().GeneralLevel; },
            [](float v)
            {
                Application::Get().SetQualityLevel((int)(v + 0.5f));
                Application::Get().GetQualitySettings().DynamicResolution = false;
            },
            0.0f, 2.0f);

        // ---- Physics debug draw ----
        // Turning any of these on also switches the debug line pass back on,
        // otherwise nothing shows up and it looks broken.
        auto physFlag3D = [this](const char* name, u32 bit)
        {
            RegisterToggle(
                name, "Physics 3D",
                [bit]()
                {
                    LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                    return p && (p->GetDebugDrawFlags() & bit) != 0;
                },
                [bit](bool v)
                {
                    LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                    if(!p)
                        return;
                    u32 flags = p->GetDebugDrawFlags();
                    p->SetDebugDrawFlags(v ? (flags | bit) : (flags & ~bit));
                    Scene::SceneRenderSettings* rs = RenderSettings();
                    if(v && rs)
                        rs->DebugRenderEnabled = true;
                });
        };

        physFlag3D("Collision Volumes", PhysicsDebugFlags::COLLISIONVOLUMES);
        physFlag3D("Collision Normals", PhysicsDebugFlags::COLLISIONNORMALS);
        physFlag3D("Contact Manifolds", PhysicsDebugFlags::MANIFOLD);
        physFlag3D("Constraints", PhysicsDebugFlags::CONSTRAINT);
        physFlag3D("AABBs", PhysicsDebugFlags::AABB);
        physFlag3D("Bounding Radius", PhysicsDebugFlags::BOUNDING_RADIUS);
        physFlag3D("Linear Velocity", PhysicsDebugFlags::LINEARVELOCITY);
        physFlag3D("Linear Force", PhysicsDebugFlags::LINEARFORCE);
        physFlag3D("Broadphase", PhysicsDebugFlags::BROADPHASE);
        physFlag3D("Broadphase Pairs", PhysicsDebugFlags::BROADPHASE_PAIRS);

        RegisterToggle(
            "Pause Physics", "Physics 3D",
            []()
            {
                LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                return p && p->IsPaused();
            },
            [](bool v)
            {
                LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                if(p)
                    p->SetPaused(v);
            });

        RegisterAction("All Debug Draw Off", "Physics 3D", []()
                       {
                           LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                           if(p)
                               p->SetDebugDrawFlags(0);
                       });

        auto physFlag2D = [this](const char* name, u32 bit)
        {
            RegisterToggle(
                name, "Physics 2D",
                [bit]()
                {
                    B2PhysicsEngine* p = Application::Get().GetSystem<B2PhysicsEngine>();
                    return p && (p->GetDebugDrawFlags() & bit) != 0;
                },
                [bit](bool v)
                {
                    B2PhysicsEngine* p = Application::Get().GetSystem<B2PhysicsEngine>();
                    if(!p)
                        return;
                    u32 flags = p->GetDebugDrawFlags();
                    p->SetDebugDrawFlags(v ? (flags | bit) : (flags & ~bit));
                    Scene::SceneRenderSettings* rs = RenderSettings();
                    if(v && rs)
                        rs->DebugRenderEnabled = true;
                });
        };

        physFlag2D("Collision Volumes 2D", PhysicsDebugFlags2D::COLLISIONVOLUMES2D);
        physFlag2D("Collision Normals 2D", PhysicsDebugFlags2D::COLLISIONNORMALS2D);
        physFlag2D("Contact Manifolds 2D", PhysicsDebugFlags2D::MANIFOLD2D);
        physFlag2D("Constraints 2D", PhysicsDebugFlags2D::CONSTRAINT2D);
        physFlag2D("AABBs 2D", PhysicsDebugFlags2D::AABB2D);
        physFlag2D("Linear Velocity 2D", PhysicsDebugFlags2D::LINEARVELOCITY2D);

        RegisterToggle(
            "Pause Physics 2D", "Physics 2D",
            []()
            {
                B2PhysicsEngine* p = Application::Get().GetSystem<B2PhysicsEngine>();
                return p && p->IsPaused();
            },
            [](bool v)
            {
                B2PhysicsEngine* p = Application::Get().GetSystem<B2PhysicsEngine>();
                if(p)
                    p->SetPaused(v);
            });

        // ---- Scene ----
        RegisterAction("Next Scene", "Scene", []()
                       {
                           if(Application::Get().GetSceneManager())
                               Application::Get().GetSceneManager()->SwitchScene();
                       });

        RegisterAction("Reload Scene", "Scene", []()
                       {
                           SceneManager* sm = Application::Get().GetSceneManager();
                           if(sm)
                               sm->SwitchScene((int)sm->GetCurrentSceneIndex());
                       });

        // ---- Engine ----
        RegisterAction("Take Screenshot", "Engine", []()
                       {
                           const std::string path = Application::Get().GetProjectSettings().m_ProjectRoot + "screenshot.png";
                           Application::Get().RequestScreenshot(path);
                           LINFO("Screenshot queued: %s", path.c_str());
                       });

        RegisterAction("Toggle Fullscreen", "Engine", []()
                       {
                           static bool fullscreen = Application::Get().GetProjectSettings().Fullscreen;
                           fullscreen             = !fullscreen;
                           OS::Get().SetWindowFullscreen(fullscreen);
                       });

        RegisterAction("Clear Console", "Engine", []() { Debug::Log::ClearRecent(); });

        RegisterAction("Cycle UI Theme", "Engine", []()
                       {
                           UITheme next = (UITheme)((GetUIState()->CurrentTheme + 1) % UITheme_Count);
                           UISetTheme(next);
                           LINFO("UI theme: %s", UIGetThemeName(next));
                       });

        RegisterFloat(
            "Fixed Timestep (ms)", "Time",
            []() { return (float)Engine::GetTimeStep().GetFixedTimestep(); },
            [](float v) { Engine::GetTimeStep().SetFixedTimestep(v < 0.5f ? 0.0 : (double)v); },
            0.0f, 100.0f);

        RegisterFloat(
            "Target FPS", "Time",
            []() { return Engine::Get().TargetFrameRate(); },
            [](float v) { Engine::Get().SetTargetFrameRate(Maths::Max(1.0f, v)); },
            10.0f, 240.0f);

        RegisterAction("Quit", "Engine", []() { Application::Get().SetAppState(AppState::Closing); });

        // ---- Scene tools: selection, gizmo, fly camera ----
        RegisterToggle(
            "Fly Camera", "View",
            [this]() { return m_FlyCamera; },
            [this](bool v) { SetFlyCamera(v); },
            (i32)InputCode::Key::F9);

        RegisterToggle(
            "Selection Bounds", "View",
            [this]() { return m_ShowSelectionBounds; },
            [this](bool v)
            {
                m_ShowSelectionBounds = v;
                Scene::SceneRenderSettings* rs = RenderSettings();
                if(v && rs)
                    rs->DebugRenderEnabled = true;
            });

        RegisterToggle(
            "Gizmo", "View",
            [this]() { return m_GizmoEnabled; },
            [this](bool v) { m_GizmoEnabled = v; });

        RegisterAction("Gizmo: Move", "View", [this]() { m_GizmoOperation = GizmoOperation_Translate; });
        RegisterAction("Gizmo: Rotate", "View", [this]() { m_GizmoOperation = GizmoOperation_Rotate; });
        RegisterAction("Gizmo: Scale", "View", [this]() { m_GizmoOperation = GizmoOperation_Scale; });
        RegisterAction("Gizmo: Universal", "View", [this]() { m_GizmoOperation = GizmoOperation_Universal; });
        RegisterToggle(
            "Gizmo Local Space", "View",
            [this]() { return m_GizmoLocal; },
            [this](bool v) { m_GizmoLocal = v; });
        RegisterToggle(
            "Gizmo Snap", "View",
            [this]() { return m_GizmoSnap; },
            [this](bool v) { m_GizmoSnap = v; });

        RegisterAction("Undo", "Edit", []() { Lumos::Undo(); }, "Ctrl+Z");
        RegisterAction("Redo", "Edit", []() { Lumos::Redo(); }, "Ctrl+Shift+Z");

        RegisterAction("Fly Camera: Auto", "View", [this]()
                       {
                           SetFlyMode(0);
                           if(!m_FlyCamera)
                               SetFlyCamera(true);
                       });
        RegisterAction("Fly Camera: 2D", "View", [this]()
                       {
                           SetFlyMode(1);
                           if(!m_FlyCamera)
                               SetFlyCamera(true);
                       });
        RegisterAction("Fly Camera: 3D", "View", [this]()
                       {
                           SetFlyMode(2);
                           if(!m_FlyCamera)
                               SetFlyCamera(true);
                       });

        RegisterFloat(
            "Fly Speed", "View",
            [this]() { return m_FlySpeed; },
            [this](float v) { m_FlySpeed = v; },
            0.5f, 200.0f);

        RegisterAction("Focus Selected", "Scene", [this]()
                       {
                           Scene* scene = CurrentScene();
                           if(!scene || m_SelectedEntity == 0xFFFFFFFFu)
                               return;
                           auto& reg              = scene->GetRegistry();
                           const entt::entity sel = (entt::entity)m_SelectedEntity;
                           if(!reg.valid(sel))
                               return;
                           Maths::Transform* target = reg.try_get<Maths::Transform>(sel);
                           ActiveCam cam            = GetActiveCam();
                           if(!target || !cam.tf || !cam.cam)
                               return;

                           const Vec3 p = target->GetWorldPosition();
                           if(cam.cam->IsOrthographic())
                               cam.tf->SetLocalPosition(Vec3(p.x, p.y, cam.tf->GetLocalPosition().z));
                           else
                               cam.tf->SetLocalPosition(p - cam.tf->GetForwardDirection() * 6.0f);
                           RefreshWorldMatrix(reg, cam.entity, cam.tf);
                           // A game camera script will pull it back next frame
                           // unless the fly camera owns the camera.
                           if(m_FlyCamera)
                               m_FlyPos = cam.tf->GetLocalPosition();
                       });

        RegisterAction("Delete Selected", "Scene", [this]()
                       {
                           Scene* scene = CurrentScene();
                           if(!scene || m_SelectedEntity == 0xFFFFFFFFu)
                               return;
                           const entt::entity sel = (entt::entity)m_SelectedEntity;
                           if(!scene->GetRegistry().valid(sel))
                               return;
                           SceneUndo::DestroyEntity(Entity(sel, scene));
                           ClearSelection();
                       });

        RegisterAction("Clear Selection", "Scene", [this]() { ClearSelection(); });

        RegisterAction("Copy Camera Transform", "Scene", []()
                       {
                           ActiveCam cam = GetActiveCam();
                           if(!cam.tf)
                               return;
                           const Vec3 p = cam.tf->GetLocalPosition();
                           const Vec3 r = cam.tf->GetLocalOrientation().ToEuler();
                           char buffer[160];
                           snprintf(buffer, sizeof(buffer),
                                    "Vec3.new(%.3f, %.3f, %.3f), Vec3.new(%.2f, %.2f, %.2f)",
                                    p.x, p.y, p.z, r.x, r.y, r.z);
                           Input::Get().SetClipboard(buffer);
                           LINFO("Camera: %s", buffer);
                       });

        RegisterAction("Log Scene Stats", "Scene", []()
                       {
                           Scene* scene = CurrentScene();
                           if(!scene)
                               return;
                           auto& reg = scene->GetRegistry();
                           LINFO("Scene '%s': %u entities, %u sprites, %u lights, %u models",
                                 scene->GetSceneName().c_str(),
                                 (u32)reg.storage<entt::entity>().in_use(),
                                 (u32)reg.view<Graphics::Sprite>().size(),
                                 (u32)reg.view<Graphics::Light>().size(),
                                 (u32)reg.view<Graphics::ModelComponent>().size());
                       });

        // ---- Time ----
        RegisterToggle(
            "Pause Game", "Time",
            []() { return Engine::GetTimeStep().GetTimeScale() == 0.0; },
            [](bool v) { Engine::GetTimeStep().SetTimeScale(v ? 0.0 : 1.0); },
            (i32)InputCode::Key::F10);

        RegisterAction("Step One Frame", "Time", [this]()
                       {
                           Engine::GetTimeStep().SetTimeScale(1.0);
                           m_StepFrames = 1; // paused again after the next update
                       });

        RegisterFloat(
            "Time Scale", "Time",
            []() { return (float)Engine::GetTimeStep().GetTimeScale(); },
            [](float v) { Engine::GetTimeStep().SetTimeScale((double)v); },
            0.0f, 3.0f);

        // ---- Audio ----
        RegisterToggle(
            "Mute Audio", "Audio",
            []()
            {
                AudioManager* audio = Application::Get().GetSystem<AudioManager>();
                return audio && audio->GetPaused();
            },
            [](bool v)
            {
                AudioManager* audio = Application::Get().GetSystem<AudioManager>();
                if(audio)
                    audio->SetPaused(v);
            });

        RegisterFloat(
            "Master Volume", "Audio",
            []()
            {
                AudioManager* audio = Application::Get().GetSystem<AudioManager>();
                return audio ? audio->GetBusVolume("Master") : 1.0f;
            },
            [](float v)
            {
                AudioManager* audio = Application::Get().GetSystem<AudioManager>();
                if(audio)
                    audio->SetBusVolume("Master", v);
            },
            0.0f, 1.0f);
    }

    void DebugMenu::SelectEntity(u32 entityId)
    {
        m_SelectedEntity = entityId;
        m_NameEditFor    = 0xFFFFFFFFu; // reload the name buffer
        // the workspace has its own inspector
        if(entityId != 0xFFFFFFFFu && !DebugWorkspace::IsVisible())
            m_Panels[DebugPanel_Inspector] = true;
    }

    void DebugMenu::DrawEntities()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        Scene* scene = CurrentScene();

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginOverlay("debugentities", SizeKind_Pixels, Maths::Min(300.0f * dpi, view.x * 0.4f), SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_TopLeft, 12.0f * dpi, 12.0f * dpi);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabelCStr("entitiestitle", "Entities");
        UIPopStyle(StyleVar_TextColor);

        UISearchField("entityfilter", m_EntityFilter, sizeof(m_EntityFilter), "Filter...", 0.0f);
        UISeparator();

        if(!scene)
        {
            UILabelCStr("noscene", "No scene");
            UIEndPanel();
            return;
        }

        auto& reg       = scene->GetRegistry();
        auto& storage   = reg.storage<entt::entity>();
        const u32 total = (u32)storage.in_use();
        u32 shown       = 0;

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 1.0f * dpi));
        UIBeginScrollArea("entitylist", Maths::Min(380.0f * dpi, view.y * 0.42f), &m_EntityScroll);
        {
            UI_Widget* area                           = GetUIState()->parents.Back();
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
            area->style_vars[StyleVar_Padding]         = Vec4(10.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        }

        // Names are formatted into a stack buffer and only copied to the frame
        // arena for rows that actually get built - a tile-heavy scene has
        // thousands of entities, and a string each was enough to exhaust it.
        char nameBuf[128];
        for(auto [e] : storage.each())
        {
            if(shown >= 300) // a big scene would cost more to list than it is worth
                break;

            const u32 id            = (u32)entt::to_integral(e);
            const NameComponent* nc = reg.try_get<NameComponent>(e);
            if(nc && !nc->name.empty())
                snprintf(nameBuf, sizeof(nameBuf), "%s", nc->name.c_str());
            else
                snprintf(nameBuf, sizeof(nameBuf), "Entity %u", id);

            if(m_EntityFilter[0])
            {
                i32 score = 0;
                u64 mask[2];
                if(!FuzzyMatch(m_EntityFilter, nameBuf, score, mask, 0))
                    continue;
            }

            shown++;
            const String8 label = PushStr8Copy(fa, nameBuf);
            const bool current = (id == m_SelectedEntity);

            String8 rowId      = PushStr8F(fa, "row###entrow%u", id);
            UI_Interaction row = UIBeginSelectableRow((const char*)rowId.str, current, 24.0f);

            String8 nameId = PushStr8F(fa, "n###entname%u", id);
            UILabelEllipsis((const char*)nameId.str, label, 190.0f);
            UIFlexSpacer();

            String8 idId = PushStr8F(fa, "i###entid%u", id);
            UIBadge((const char*)idId.str, PushStr8F(fa, "%u", id), kOffBG, kDimText);

            UIEndSelectableRow();

            if(row.clicked)
                SelectEntity(id);
        }

        if(shown == 0)
        {
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelCStr("entitynone", "No matching entities");
            UIPopStyle(StyleVar_TextColor);
        }

        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabel("entitycount", PushStr8F(fa, "%u shown / %u total", shown, total));
        UIPopStyle(StyleVar_TextColor);

        UIEndPanel();
    }

    void DebugMenu::DrawInspector()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        Scene* scene = CurrentScene();
        if(!scene)
            return;

        auto& reg              = scene->GetRegistry();
        const entt::entity sel = (entt::entity)m_SelectedEntity;
        if(m_SelectedEntity == 0xFFFFFFFFu || !reg.valid(sel))
            return;

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginOverlay("debuginspector", SizeKind_Pixels, Maths::Min(340.0f * dpi, view.x * 0.42f), SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_MiddleRight, 12.0f * dpi, 0.0f);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIBeginRowFullWidth();
        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabelCStr("inspectortitle", "Inspector");
        UIPopStyle(StyleVar_TextColor);
        UIFlexSpacer();
        if(UIButton("X###inspectorclose").clicked)
            ClearSelection();
        UIEndRow();
        UISeparator();

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginScrollArea("inspectorbody", Maths::Min(460.0f * dpi, view.y * 0.62f), &m_InspectorScroll);
        {
            UI_Widget* area                           = GetUIState()->parents.Back();
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
            area->style_vars[StyleVar_Padding]         = Vec4(10.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        }

        auto header = [&](const char* title)
        {
            UISpacer(4.0f);
            UIPushStyle(StyleVar_TextColor, kAccent);
            String8 id = PushStr8F(fa, "h###insphdr%s", title);
            UILabelCStr((const char*)id.str, title);
            UIPopStyle(StyleVar_TextColor);
        };

        // --- name ---
        {
            if(m_NameEditFor != m_SelectedEntity)
            {
                String8 label = EntityLabel(fa, reg, sel);
                snprintf(m_NameEdit, sizeof(m_NameEdit), "%.*s", (int)label.size, (const char*)label.str);
                m_NameEditFor = m_SelectedEntity;
            }

            UIBeginRowFullWidth();
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelCStr("insplabelname", "Name");
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            UIBadge("id###inspid", PushStr8F(fa, "id %u", m_SelectedEntity), kOffBG, kDimText);
            UIEndRow();

            if(UISearchField("inspname", m_NameEdit, sizeof(m_NameEdit), "unnamed", 0.0f).clicked)
            {
            }
            NameComponent* nameComp = reg.try_get<NameComponent>(sel);
            if(nameComp && nameComp->name != m_NameEdit)
                nameComp->name = m_NameEdit;
        }

        // --- transform ---
        if(Maths::Transform* tf = reg.try_get<Maths::Transform>(sel))
        {
            header("Transform");

            bool moved = false;

            Vec3 pos = tf->GetLocalPosition();
            if(UIDragVec3Row("insppos", Str8Lit("Position"), &pos.x, 0.02f))
            {
                tf->SetLocalPosition(pos);
                moved = true;
            }

            Vec3 euler = tf->GetLocalOrientation().ToEuler();
            if(UIDragVec3Row("insprot", Str8Lit("Rotation"), &euler.x, 0.5f))
            {
                tf->SetLocalOrientation(Quat(euler));
                moved = true;
            }

            Vec3 scale = tf->GetLocalScale();
            if(UIDragVec3Row("inspscale", Str8Lit("Scale"), &scale.x, 0.01f))
            {
                tf->SetLocalScale(scale);
                moved = true;
            }

            if(moved)
                RefreshWorldMatrix(reg, sel, tf);
        }

        // --- sprite ---
        if(Graphics::Sprite* sprite = reg.try_get<Graphics::Sprite>(sel))
        {
            header("Sprite");
            Vec4 colour = sprite->GetColour();
            if(UIColorEdit4("Colour###inspspritecol", &colour.x))
                sprite->SetColour(colour);

            Vec2 scale = sprite->GetScale();
            bool changed = UIDragFloatRow("inspspritew", Str8Lit("Width"), &scale.x, 0.01f);
            changed |= UIDragFloatRow("inspspriteh", Str8Lit("Height"), &scale.y, 0.01f);
            if(changed)
                sprite->SetScale(scale);
        }

        if(Graphics::AnimatedSprite* anim = reg.try_get<Graphics::AnimatedSprite>(sel))
        {
            header("Animated Sprite");
            UILabel("inspanimstate", PushStr8F(fa, "State: %s", anim->GetState().c_str()));
            Vec4 colour = anim->GetColour();
            if(UIColorEdit4("Colour###inspanimcol", &colour.x))
                anim->SetColour(colour);
        }

        // --- light ---
        if(Graphics::Light* light = reg.try_get<Graphics::Light>(sel))
        {
            header("Light");
            static const char* kTypes[] = { "Directional", "Spot", "Point" };
            const int typeIndex         = Maths::Clamp((int)light->Type, 0, 2);
            UILabel("insplighttype", PushStr8F(fa, "Type: %s", kTypes[typeIndex]));
            UIColorEdit4("Colour###insplightcol", &light->Colour.x);
            UIDragFloatRow("insplightint", Str8Lit("Intensity"), &light->Intensity, 1.0f, 0.0f, 0.0f, "%.1f");
            UIDragFloatRow("insplightrad", Str8Lit("Radius"), &light->Radius, 0.05f, 0.0f, 0.0f, "%.2f");
            if(typeIndex == 1)
                UIDragFloatRow("insplightang", Str8Lit("Angle"), &light->Angle, 0.01f, 0.0f, 0.0f, "%.2f");
        }

        // --- camera ---
        if(Camera* cam = reg.try_get<Camera>(sel))
        {
            header("Camera");
            bool ortho = cam->IsOrthographic();
            if(UICheckbox("Orthographic###inspcamortho", &ortho).clicked)
                cam->SetIsOrthographic(!cam->IsOrthographic());

            if(cam->IsOrthographic())
            {
                float scale = cam->GetScale();
                if(UIDragFloatRow("inspcamscale", Str8Lit("Scale"), &scale, 0.05f, 0.01f, 1000.0f, "%.2f"))
                    cam->SetScale(scale);
            }
            else
            {
                float fov = cam->GetFOV();
                if(UIDragFloatRow("inspcamfov", Str8Lit("FOV"), &fov, 0.2f, 1.0f, 179.0f, "%.1f"))
                    cam->SetFOV(fov);
            }

            float nearP = cam->GetNear();
            if(UIDragFloatRow("inspcamnear", Str8Lit("Near"), &nearP, 0.01f, 0.001f, 1000.0f, "%.3f"))
                cam->SetNear(nearP);
            float farP = cam->GetFar();
            if(UIDragFloatRow("inspcamfar", Str8Lit("Far"), &farP, 1.0f, 0.01f, 100000.0f, "%.1f"))
                cam->SetFar(farP);
        }

        // --- physics ---
        if(RigidBody2DComponent* rb = reg.try_get<RigidBody2DComponent>(sel))
        {
            header("RigidBody 2D");
            if(RigidBody2D* body = rb->GetRigidBody())
            {
                Vec2 p = body->GetPosition();
                if(UIDragFloatRow("insprb2x", Str8Lit("Position X"), &p.x, 0.02f) ||
                   UIDragFloatRow("insprb2y", Str8Lit("Position Y"), &p.y, 0.02f))
                    body->SetPosition(p);

                Vec2 v = body->GetLinearVelocity();
                if(UIDragFloatRow("insprb2vx", Str8Lit("Velocity X"), &v.x, 0.05f) ||
                   UIDragFloatRow("insprb2vy", Str8Lit("Velocity Y"), &v.y, 0.05f))
                    body->SetLinearVelocity(v);

                UILabel("insprb2static", PushStr8F(fa, "Static: %s   Angle %.1f",
                                                   body->GetIsStatic() ? "yes" : "no",
                                                   Maths::ToDegrees(body->GetAngle())));
            }
        }

        if(RigidBody3DComponent* rb = reg.try_get<RigidBody3DComponent>(sel))
        {
            header("RigidBody 3D");
            if(RigidBody3D* body = rb->GetRigidBody())
            {
                Vec3 p = body->GetPosition();
                if(UIDragVec3Row("insprb3pos", Str8Lit("Position"), &p.x, 0.02f))
                    body->SetPosition(p);

                Vec3 v = body->GetLinearVelocity();
                if(UIDragVec3Row("insprb3vel", Str8Lit("Velocity"), &v.x, 0.05f))
                    body->SetLinearVelocity(v);

                const float invMass = body->GetInverseMass();
                UILabel("insprb3mass", PushStr8F(fa, "Mass %.2f   Static: %s",
                                                 invMass > 0.0f ? 1.0f / invMass : 0.0f,
                                                 body->GetIsStatic() ? "yes" : "no"));
            }
        }

        // --- misc, read-only ---
        if(Graphics::ModelComponent* model = reg.try_get<Graphics::ModelComponent>(sel))
        {
            header("Model");
            if(model->ModelRef)
                UILabel("inspmodel", PushStr8F(fa, "%u meshes", (u32)model->ModelRef->GetMeshes().Size()));
        }

        if(TextComponent* text = reg.try_get<TextComponent>(sel))
        {
            header("Text");
            UILabelEllipsis("insptext", PushStr8F(fa, "%s", text->TextString.c_str()), 280.0f);
            UIColorEdit4("Colour###insptextcol", &text->Colour.x);
        }

        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);

        UISeparator();
        UIBeginRowFullWidth();
        if(UIButton("Focus###inspfocus").clicked)
            Run("Focus Selected");
        UIFlexSpacer();
        if(UIButton("Delete###inspdelete").clicked)
            Run("Delete Selected");
        UIEndRow();

        UIEndPanel();
    }

    // ---- gizmo -------------------------------------------------------------

    void DebugMenu::UpdateGizmo()
    {
        Scene* scene = CurrentScene();
        if((!m_GizmoEnabled && !m_ShowSelectionBounds) || !scene || m_SelectedEntity == 0xFFFFFFFFu)
            return;

        auto& reg              = scene->GetRegistry();
        const entt::entity sel = (entt::entity)m_SelectedEntity;
        if(!reg.valid(sel))
            return;

        Maths::Transform* tf = reg.try_get<Maths::Transform>(sel);
        ActiveCam cam        = GetActiveCam();
        if(!tf || !cam.cam || cam.entity == sel)
            return;

        UI_State* ui = GetUIState();

        if(m_GizmoEnabled)
        {
            GizmoView view = TransformGizmo::ViewFromCamera(cam.cam, cam.tf, ui->FrameBufferSize);

            GizmoInput input;
            input.Mouse   = UIMouse();
            input.Down    = Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft);
            input.Pressed = Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft);
            input.Enabled = !UIIsMouseOverUI() && !m_FlyLooking;

            GizmoSettings settings;
            settings.DPIScale = ui->DPIScale;
            settings.Snap     = m_GizmoSnap;

            // 2D games only have two axes worth dragging; the gizmo hides the one facing the camera.
            TransformGizmo::ManipulateEntities(scene, &sel, 1, view, input, m_GizmoOperation,
                                      m_GizmoLocal ? GizmoSpace::Local : GizmoSpace::World, settings);
        }

        if(m_ShowSelectionBounds)
        {
            Maths::BoundingBox bounds;
            if(SelectionBounds(reg, sel, bounds))
            {
                const float size = (bounds.Max() - bounds.Min()).Length();
                DebugRenderer::DebugDraw(bounds, Vec4(1.0f, 0.72f, 0.20f, 0.9f), true, false, Maths::Max(size * 0.004f, 0.002f));
                DebugRenderer::RequestPass();
            }
        }
    }

    // ---- fly camera --------------------------------------------------------

    void DebugMenu::SetFlyCamera(bool enabled)
    {
        if(enabled == m_FlyCamera)
            return;

        ActiveCam cam = GetActiveCam();

        if(enabled)
        {
            if(!cam.tf || !cam.cam)
            {
                LWARN("Fly camera: no camera in the scene");
                return;
            }

            m_FlyCameraEntity = (u32)entt::to_integral(cam.entity);
            m_FlySavedPos     = cam.tf->GetLocalPosition();
            m_FlySavedRot     = cam.tf->GetLocalOrientation();
            m_FlySavedScale   = cam.cam->GetScale();
            m_FlySavedNear    = cam.cam->GetNear();
            m_FlySavedFar     = cam.cam->GetFar();

            const bool camIsOrtho   = cam.cam->IsOrthographic();
            m_FlyPerspective        = (m_FlyMode == 2) || (m_FlyMode == 0 && !camIsOrtho);
            m_FlyForcedPerspective  = false;

            m_FlyPos   = m_FlySavedPos;
            m_FlyScale = m_FlySavedScale;

            const Vec3 euler = m_FlySavedRot.ToEuler();
            m_FlyPitch       = euler.x;
            m_FlyYaw         = euler.y;

            if(m_FlyPerspective && camIsOrtho)
            {
                // 3D flight through a 2D scene: swap the projection and back off
                // far enough that the sprite plane is in view.
                cam.cam->SetIsOrthographic(false);
                cam.cam->SetNear(0.1f);
                cam.cam->SetFar(2000.0f);
                m_FlyForcedPerspective = true;
                m_FlyPitch             = 0.0f;
                m_FlyYaw               = 0.0f;
                m_FlyPos.z             = m_FlySavedPos.z + Maths::Max(5.0f, m_FlySavedScale * 2.2f);
            }

            m_FlyLastMouse = Input::Get().GetMousePosition();
            m_FlyLooking   = false;
            m_FlyCamera    = true;
            // The palette eats WASD and gates the fly update, so get out of the way.
            ClosePalette();
            LINFO("Fly camera on (%s). WASD move, Q/E down/up, RMB look, scroll speed.",
                  m_FlyPerspective ? "3D" : "2D");
        }
        else
        {
            m_FlyCamera = false;
            if(m_FlyLooking)
            {
                Input::Get().SetMouseMode(MouseMode::Visible);
                m_FlyLooking = false;
            }

            // Put the game's camera back exactly as it was.
            if(cam.tf && cam.entity != entt::null && (u32)entt::to_integral(cam.entity) == m_FlyCameraEntity)
            {
                cam.tf->SetLocalPosition(m_FlySavedPos);
                cam.tf->SetLocalOrientation(m_FlySavedRot);
                if(Scene* scene = CurrentScene())
                    RefreshWorldMatrix(scene->GetRegistry(), cam.entity, cam.tf);
                if(cam.cam)
                {
                    if(m_FlyForcedPerspective)
                    {
                        cam.cam->SetIsOrthographic(true);
                        cam.cam->SetNear(m_FlySavedNear);
                        cam.cam->SetFar(m_FlySavedFar);
                    }
                    if(m_FlySavedScale > 0.0f)
                        cam.cam->SetScale(m_FlySavedScale);
                }
            }
            m_FlyForcedPerspective = false;
        }
    }

    void DebugMenu::SetFlyMode(i32 mode)
    {
        mode = Maths::Clamp(mode, 0, 2);
        if(mode == m_FlyMode)
            return;
        m_FlyMode = mode;

        // Restart so the new mode is picked up (and an ortho camera flipped back).
        if(m_FlyCamera)
        {
            SetFlyCamera(false);
            SetFlyCamera(true);
        }
    }

    void DebugMenu::UpdateFlyCamera(float dt, bool acceptInput)
    {
        if(!m_FlyCamera)
            return;

        ActiveCam cam = GetActiveCam();
        if(!cam.tf || !cam.cam)
            return;

        // With the palette up the pose is still applied every frame - only the
        // input is ignored - so the camera doesn't snap back mid-flight.
        Input& in       = Input::Get();
        const bool fast = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::LeftShift) || in.GetKeyHeldRaw(InputCode::Key::RightShift));
        const bool slow = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::LeftAlt) || in.GetKeyHeldRaw(InputCode::Key::RightAlt));
        const float scroll = acceptInput ? in.GetScrollOffset() : 0.0f;

        const bool left  = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::A) || in.GetKeyHeldRaw(InputCode::Key::Left));
        const bool right = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::D) || in.GetKeyHeldRaw(InputCode::Key::Right));
        const bool fwd   = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::W) || in.GetKeyHeldRaw(InputCode::Key::Up));
        const bool back  = acceptInput && (in.GetKeyHeldRaw(InputCode::Key::S) || in.GetKeyHeldRaw(InputCode::Key::Down));
        const bool up    = acceptInput && in.GetKeyHeldRaw(InputCode::Key::E);
        const bool down  = acceptInput && in.GetKeyHeldRaw(InputCode::Key::Q);

        if(!m_FlyPerspective)
        {
            // 2D: pan in the camera plane, scroll zooms (ortho) or dollies.
            if(cam.cam->IsOrthographic())
            {
                if(Maths::Abs(scroll) > 0.001f)
                    m_FlyScale = Maths::Clamp(m_FlyScale * (scroll > 0.0f ? 0.9f : 1.1f), 0.05f, 5000.0f);
                cam.cam->SetScale(m_FlyScale);
            }

            const float span  = cam.cam->IsOrthographic() ? m_FlyScale : 10.0f;
            const float speed = span * (fast ? 4.0f : (slow ? 0.25f : 1.5f));

            Vec3 move(0.0f);
            if(left)  move.x -= 1.0f;
            if(right) move.x += 1.0f;
            if(fwd)   move.y += 1.0f;
            if(back)  move.y -= 1.0f;
            if(up)    move.z += 1.0f;
            if(down)  move.z -= 1.0f;

            if(move.LengthSquared() > 0.0f)
                m_FlyPos += move.Normalised() * speed * dt;

            cam.tf->SetLocalPosition(m_FlyPos);
            RefreshWorldMatrix(CurrentScene()->GetRegistry(), cam.entity, cam.tf);
            return;
        }

        if(Maths::Abs(scroll) > 0.001f)
            m_FlySpeed = Maths::Clamp(m_FlySpeed * (scroll > 0.0f ? 1.15f : 0.87f), 0.1f, 500.0f);

        const Vec2 mouse = in.GetMousePosition();
        if(acceptInput && in.GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            if(!m_FlyLooking)
            {
                m_FlyLooking = true;
                in.SetMouseMode(MouseMode::Captured);
            }
            else
            {
                const Vec2 d = mouse - m_FlyLastMouse;
                // A captured cursor warps; ignore the jump that produces.
                if(Maths::Abs(d.x) < 200.0f && Maths::Abs(d.y) < 200.0f)
                {
                    m_FlyYaw -= d.x * 0.15f;
                    m_FlyPitch = Maths::Clamp(m_FlyPitch - d.y * 0.15f, -89.0f, 89.0f);
                }
            }
            m_FlyLastMouse = mouse;
        }
        else if(m_FlyLooking)
        {
            m_FlyLooking = false;
            in.SetMouseMode(MouseMode::Visible);
        }
        else
        {
            m_FlyLastMouse = mouse;
        }

        const Quat rot     = Quat(Vec3(m_FlyPitch, m_FlyYaw, 0.0f));
        const Vec3 forward = rot * Vec3(0.0f, 0.0f, -1.0f);
        const Vec3 rightV  = rot * Vec3(1.0f, 0.0f, 0.0f);

        Vec3 move(0.0f);
        if(fwd)   move += forward;
        if(back)  move -= forward;
        if(right) move += rightV;
        if(left)  move -= rightV;
        if(up)    move += Vec3(0.0f, 1.0f, 0.0f);
        if(down)  move -= Vec3(0.0f, 1.0f, 0.0f);

        if(move.LengthSquared() > 0.0f)
        {
            const float speed = m_FlySpeed * (fast ? 4.0f : (slow ? 0.2f : 1.0f));
            m_FlyPos += move.Normalised() * speed * dt;
        }

        cam.tf->SetLocalPosition(m_FlyPos);
        cam.tf->SetLocalOrientation(rot);
        RefreshWorldMatrix(CurrentScene()->GetRegistry(), cam.entity, cam.tf);
    }

    void DebugMenu::OnPostUpdate(float dt)
    {
        if(!m_Enabled || !m_Initialised)
            return;

        if(m_FlyCamera)
            UpdateFlyCamera(dt, !m_PaletteOpen);

        if(!m_PaletteOpen)
            UpdateGizmo();
    }

    void DebugMenu::DrawFlyHint()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;

        UIPushStyle(StyleVar_BackgroundColor, Vec4(0.06f, 0.07f, 0.10f, 0.80f));
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 6.0f * dpi));
        UIBeginOverlay("debugflyhint", SizeKind_ChildSum, 1.0f, SizeKind_ChildSum, 1.0f, WidgetFlags_StackHorizontally);
        UIWindowAnchor(UIAnchor_TopCenter, 0.0f, 10.0f * dpi);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIBadge("mode###flymode", m_FlyPerspective ? Str8Lit("FLY 3D") : Str8Lit("FLY 2D"),
                Vec4(0.24f, 0.44f, 0.85f, 0.9f), Vec4(1.0f));

        UIPushStyle(StyleVar_TextColor, kDimText);
        if(m_FlyPerspective)
            UILabel("flyhint", PushStr8F(fa, "  WASD move   Q/E down/up   RMB look   scroll speed (%.1f)   F9 exit", m_FlySpeed));
        else
            UILabel("flyhint", PushStr8F(fa, "  WASD pan   Q/E depth   scroll zoom   F9 exit"));
        UIPopStyle(StyleVar_TextColor);

        UIEndPanel();
    }

    void DebugMenu::DrawToast()
    {
        UI_State* ui    = GetUIState();
        const float dpi = ui->DPIScale;

        const float alpha = Maths::Clamp(m_ToastTime * 1.5f, 0.0f, 1.0f);

        UIPushStyle(StyleVar_Alpha, alpha);
        UIPushStyle(StyleVar_BackgroundColor, Vec4(0.06f, 0.07f, 0.10f, 0.92f));
        UIPushStyle(StyleVar_Padding, Vec2(14.0f * dpi, 8.0f * dpi));
        UIBeginOverlay("debugtoast", SizeKind_ChildSum, 1.0f, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_BottomCenter, 0.0f, 40.0f * dpi);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_TextColor, Vec4(0.90f, 0.93f, 0.97f, 1.0f));
        UILabelCStr("debugtoasttext", m_ToastText);
        UIPopStyle(StyleVar_TextColor);

        UIEndPanel();
        UIPopStyle(StyleVar_Alpha);
    }

    void DebugMenu::DrawPalette()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        if(strcmp(m_Query, m_LastQuery) != 0)
        {
            // \x01 is the "commands changed, rebuild" sentinel - a real edit to
            // the query restarts the list at the top, a rebuild keeps the spot.
            const bool queryEdited = m_LastQuery[0] != '\x01';
            RefreshResults();
            if(queryEdited)
            {
                m_Selected   = 0;
                m_ScrolledTo = 0;
                m_Scroll     = 0.0f;
            }
            strncpy(m_LastQuery, m_Query, sizeof(m_LastQuery) - 1);
            m_LastQuery[sizeof(m_LastQuery) - 1] = 0;
        }

        const bool clickedOutside = UIDimBackdrop("debugpalettedim", 0.45f).clicked;

        // Hover only steals the selection when the mouse actually moved -
        // otherwise a resting cursor fights the arrow keys.
        const Vec2 mouse     = Input::Get().GetMousePosition();
        const bool mouseMoved = Maths::Abs(mouse.x - m_LastMouseX) > 0.5f || Maths::Abs(mouse.y - m_LastMouseY) > 0.5f;
        m_LastMouseX          = mouse.x;
        m_LastMouseY          = mouse.y;

        const float width   = Maths::Min(kPaletteWidth * dpi, view.x * 0.92f);
        const float rowH    = kRowHeight * dpi;
        const float stride  = rowH + kRowSpacing * dpi;
        const i32 visRows   = (i32)Maths::Clamp(Maths::Floor((view.y * 0.45f) / stride), 3.0f, 12.0f);
        const float listH   = visRows * stride + 6.0f * dpi;

        // Scroll to the selection only when it actually moves - doing it every
        // frame yanks the list back the moment the wheel scrolls it away.
        if(m_Selected != m_ScrolledTo)
        {
            m_ScrolledTo       = m_Selected;
            const float top    = m_Selected * stride;
            const float bottom = top + rowH;
            if(top < m_Scroll)
                m_Scroll = top;
            else if(bottom > m_Scroll + listH - 6.0f * dpi)
                m_Scroll = bottom - (listH - 6.0f * dpi);
            const float maxScroll = Maths::Max(0.0f, (float)m_Results.Size() * stride - (listH - 6.0f * dpi));
            m_Scroll              = Maths::Clamp(m_Scroll, 0.0f, maxScroll);
        }

        UIPushStyle(StyleVar_BackgroundColor, Vec4(0.06f, 0.07f, 0.10f, 0.97f));
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 12.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 6.0f * dpi));
        UIBeginOverlay("cmdpalette", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically | WidgetFlags_AnimateAppear);
        UIWindowAnchor(UIAnchor_TopCenter, 0.0f, view.y * 0.12f);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        // Search row: magnifier + field
        UIBeginRowFullWidth();
        {
            if(m_ClaimFocus)
            {
                m_ClaimFocus = false;
                UIFocusNextTextInput();
            }
            UIPushStyle(StyleVar_TextColor, kDimText);
            UIPushStyle(StyleVar_FontSize, ui->style_variable_lists[StyleVar_FontSize].last->value.x * 1.1f);
            UILabelCStr("palettesearchicon", ICON_MDI_MAGNIFY);
            UIPopStyle(StyleVar_FontSize);
            UIPopStyle(StyleVar_TextColor);
            UISearchField("palettesearch", m_Query, sizeof(m_Query), "Search commands...", 0.0f);
        }
        UIEndRow();

        UISeparator();

        if(m_Results.Size() == 0)
        {
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelCStr("palettenoresults", "No matching commands");
            UIPopStyle(StyleVar_TextColor);
        }
        else
        {
            UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, kRowSpacing * dpi));
            UIBeginScrollArea("palettelist", listH, &m_Scroll);
            {
                UI_Widget* area = GetUIState()->parents.Back();
                area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                // Wider side padding keeps the right-hand badges clear of the scrollbar.
                area->style_vars[StyleVar_Padding]         = Vec4(12.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
            }

            const u32 shown = (u32)Maths::Min<u64>(m_Results.Size(), kMaxResults);
            for(u32 i = 0; i < shown; i++)
            {
                DebugCommand& cmd = m_Commands[m_Results[i].index];

                String8 rowId = PushStr8F(fa, "row###cmdrow%u", i);
                UI_Interaction row = UIBeginSelectableRow((const char*)rowId.str, (i32)i == m_Selected, kRowHeight);

                if(row.hovering && mouseMoved)
                    m_Selected = (i32)i;

                String8 badgeId = PushStr8F(fa, "%.*s###cmdcat%u", (int)cmd.Category.size, (const char*)cmd.Category.str, i);
                UIBadge((const char*)badgeId.str, cmd.Category, kBadgeBG, kBadgeText);

                UIHighlightMask mask;
                mask.bits[0] = m_Results[i].maskBits[0];
                mask.bits[1] = m_Results[i].maskBits[1];

                String8 nameId = PushStr8F(fa, "name###cmdname%u", i);
                UIPushStyle(StyleVar_TextColor, Vec4(0.94f, 0.95f, 0.97f, 1.0f));
                UILabelHighlighted((const char*)nameId.str, cmd.Name, mask, kAccent);
                UIPopStyle(StyleVar_TextColor);

                UIFlexSpacer();

                if(cmd.Type == DebugCommandType::Toggle && cmd.GetBool)
                {
                    const bool on   = cmd.GetBool();
                    String8 stateId = PushStr8F(fa, "%s###cmdstate%u", on ? "ON" : "OFF", i);
                    UIBadge((const char*)stateId.str, on ? Str8Lit("ON") : Str8Lit("OFF"),
                            on ? kOnBG : kOffBG, on ? Vec4(1.0f) : kDimText);
                }
                else if(cmd.Type == DebugCommandType::Float && cmd.GetFloat)
                {
                    String8 valId = PushStr8F(fa, "val###cmdval%u", i);
                    UIBadge((const char*)valId.str, PushStr8F(fa, "< %.3g >", cmd.GetFloat()), kOffBG, kDimText);
                }
                else if(cmd.Hint.size > 0)
                {
                    String8 hintId = PushStr8F(fa, "%.*s###cmdhint%u", (int)cmd.Hint.size, (const char*)cmd.Hint.str, i);
                    UIBadge((const char*)hintId.str, cmd.Hint, kOffBG, kDimText);
                }

                UIEndSelectableRow();

                if(row.clicked)
                {
                    m_Selected = (i32)i;
                    RunCommand(cmd);
                    if(cmd.Type == DebugCommandType::Action)
                    {
                        ClosePalette();
                        break;
                    }
                }
            }

            UIEndScrollArea();
            UIPopStyle(StyleVar_ItemSpacing);
        }

        UISeparator();

        UIPushStyle(StyleVar_TextColor, kDimText);
        {
            String8 footer = PushStr8F(fa, "%s/%s move    Enter run    Esc close    %llu commands",
                                       ICON_MDI_ARROW_UP, ICON_MDI_ARROW_DOWN, (unsigned long long)m_Commands.Size());
            UILabel("palettefooter", footer);
        }
        UIPopStyle(StyleVar_TextColor);

        UIEndPanel();

        if(clickedOutside)
            ClosePalette();
    }

    void DebugMenu::DrawStats()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;

        auto& stats  = Engine::Get().Statistics();
        Scene* scene = CurrentScene();

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(10.0f * dpi, 8.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 2.0f * dpi));
        UIBeginOverlay("debugstats", SizeKind_ChildSum, 1.0f, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_TopRight, 12.0f * dpi, 12.0f * dpi);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        const float fps = m_SmoothedMs > 0.0f ? 1000.0f / m_SmoothedMs : 0.0f;
        Vec4 fpsColour  = fps >= 55.0f ? Vec4(0.45f, 0.90f, 0.55f, 1.0f)
                                       : (fps >= 28.0f ? Vec4(1.0f, 0.80f, 0.35f, 1.0f) : Vec4(1.0f, 0.45f, 0.42f, 1.0f));

        UIPushStyle(StyleVar_TextColor, fpsColour);
        UILabel("statsfps", PushStr8F(fa, "%.0f FPS   %.2f ms", fps, m_SmoothedMs));
        UIPopStyle(StyleVar_TextColor);

        UIPlotHistogram("statsgraph", m_FrameTimes, kFrameHistory, 0.0f, 33.3f, 180.0f, 34.0f,
                        Vec4(0.42f, 0.72f, 1.0f, 0.85f), Vec4(1.0f, 1.0f, 1.0f, 0.05f), 16.6f);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabel("statsdraw", PushStr8F(fa, "Draws %u   Tris %u", stats.NumDrawCalls, stats.TriangleCount));
        UILabel("statsobj", PushStr8F(fa, "Objects %u   Shadow %u", stats.NumRenderedObjects, stats.NumShadowObjects));
        UILabel("statsmem", PushStr8F(fa, "GPU %.0f/%.0f MB", stats.UsedGPUMemory / 1048576.0f, stats.TotalGPUMemory / 1048576.0f));

        // Body counts make "the physics debug draw shows nothing" self-explanatory
        // when a game does its own collision instead of using an engine solver.
        {
            LumosPhysicsEngine* p3 = Application::Get().GetSystem<LumosPhysicsEngine>();
            B2PhysicsEngine* p2    = Application::Get().GetSystem<B2PhysicsEngine>();
            const int bodies3D     = p3 ? p3->GetNumberRigidBodys() : 0;
            const u32 bodies2D     = p2 ? p2->GetBodyCount() : 0;
            UILabel("statsbodies", PushStr8F(fa, "Bodies  3D %d   2D %u", bodies3D, bodies2D));
        }

        if(scene)
        {
            const u32 entities = (u32)scene->GetRegistry().storage<entt::entity>().in_use();
            UILabel("statsscene", PushStr8F(fa, "%s   %u entities", scene->GetSceneName().c_str(), entities));
        }
        UIPopStyle(StyleVar_TextColor);

        UIEndPanel();
    }

    void DebugMenu::DrawFrameGraph()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;

        float worst = 0.0f;
        float avg   = 0.0f;
        for(u32 i = 0; i < kFrameHistory; i++)
        {
            worst = Maths::Max(worst, m_FrameTimes[i]);
            avg += m_FrameTimes[i];
        }
        avg /= (float)kFrameHistory;

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(10.0f * dpi, 8.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 4.0f * dpi));
        UIBeginOverlay("debugframegraph", SizeKind_ChildSum, 1.0f, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_BottomRight, 12.0f * dpi, 12.0f * dpi);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabel("fgtitle", PushStr8F(fa, "Frame  avg %.2f ms   peak %.2f ms", avg, worst));
        UIPopStyle(StyleVar_TextColor);

        UIPlotHistogram("fggraph", m_FrameTimes, kFrameHistory, 0.0f, Maths::Max(33.3f, worst * 1.1f),
                        360.0f, 90.0f, Vec4(0.42f, 0.72f, 1.0f, 0.85f), Vec4(1.0f, 1.0f, 1.0f, 0.05f), 16.6f);

        UIEndPanel();
    }

    void DebugMenu::DrawLog()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(10.0f * dpi, 8.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 2.0f * dpi));
        UIBeginOverlay("debuglog", SizeKind_Pixels, Maths::Min(700.0f * dpi, view.x * 0.6f), SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_BottomLeft, 12.0f * dpi, 12.0f * dpi);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIBeginRowFullWidth();
        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabelCStr("logtitle", "Console");
        UIPopStyle(StyleVar_TextColor);
        UIFlexSpacer();
        if(UIButton("Clear###logclear").clicked)
            Debug::Log::ClearRecent();
        UIEndRow();

        const float logH = Maths::Min(240.0f * dpi, view.y * 0.35f);
        const u32 count  = Debug::Log::RecentCount();

        // Stick to the bottom while new lines arrive - UIEndScrollArea clamps
        // this to the real maximum during the same build.
        const u64 newest = count ? Debug::Log::GetRecent(count - 1).index : 0;
        if(newest != m_LogSeen)
        {
            m_LogSeen   = newest;
            m_LogScroll = 1.0e6f;
        }

        UIPushStyle(StyleVar_FontSize, ui->style_variable_lists[StyleVar_FontSize].last->value.x * 0.85f);
        UIBeginScrollArea("loglist", logH, &m_LogScroll);
        for(u32 i = 0; i < count; i++)
        {
            Debug::LogRecord rec = Debug::Log::GetRecent(i);
            String8 id           = PushStr8F(fa, "logline###logline%u", i);
            UIPushStyle(StyleVar_TextColor, LevelColour(rec.level));
            UILabelEllipsis((const char*)id.str, Str8C((char*)rec.message), 660.0f);
            UIPopStyle(StyleVar_TextColor);
        }
        UIEndScrollArea();
        UIPopStyle(StyleVar_FontSize);

        UIEndPanel();
    }

    void DebugMenu::DrawToggles()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginOverlay("debugtoggles", SizeKind_Pixels, Maths::Min(320.0f * dpi, view.x * 0.4f), SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_MiddleRight, 12.0f * dpi, 0.0f);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabelCStr("togglestitle", "Debug Draw");
        UIPopStyle(StyleVar_TextColor);
        UISeparator();

        static float scroll = 0.0f;
        UIBeginScrollArea("togglelist", Maths::Min(420.0f * dpi, view.y * 0.5f), &scroll);

        String8 lastCategory = { 0 };
        for(size_t i = 0; i < m_Commands.Size(); i++)
        {
            DebugCommand& cmd = m_Commands[i];
            if(cmd.Type != DebugCommandType::Toggle || !cmd.GetBool || !cmd.SetBool)
                continue;

            if(!Str8Match(cmd.Category, lastCategory, MatchFlags(0)))
            {
                lastCategory   = cmd.Category;
                String8 catId  = PushStr8F(fa, "cat###togglecat%u", (u32)i);
                UIPushStyle(StyleVar_TextColor, kAccent);
                UILabel((const char*)catId.str, cmd.Category);
                UIPopStyle(StyleVar_TextColor);
            }

            bool value    = cmd.GetBool();
            String8 rowId = PushStr8F(fa, "%.*s###toggle%u", (int)cmd.Name.size, (const char*)cmd.Name.str, (u32)i);
            if(UIToggle((const char*)rowId.str, &value).clicked)
                cmd.SetBool(!cmd.GetBool());
        }

        UIEndScrollArea();
        UIEndPanel();
    }

    void DebugMenu::DrawTweaks()
    {
        UI_State* ui    = GetUIState();
        Arena* fa       = ui->UIFrameArena;
        const float dpi = ui->DPIScale;
        const Vec2 view = ui->root_parent.size;

        UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
        UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginOverlay("debugtweaks", SizeKind_Pixels, Maths::Min(340.0f * dpi, view.x * 0.4f), SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically);
        UIWindowAnchor(UIAnchor_MiddleLeft, 12.0f * dpi, 0.0f);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabelCStr("tweakstitle", "Tweaks");
        UIPopStyle(StyleVar_TextColor);
        UISeparator();

        static float scroll = 0.0f;
        UIBeginScrollArea("tweaklist", Maths::Min(420.0f * dpi, view.y * 0.34f), &scroll);

        for(size_t i = 0; i < m_Commands.Size(); i++)
        {
            DebugCommand& cmd = m_Commands[i];
            if(cmd.Type != DebugCommandType::Float || !cmd.GetFloat || !cmd.SetFloat)
                continue;

            float value   = cmd.GetFloat();
            String8 rowId = PushStr8F(fa, "%.*s###tweak%u", (int)cmd.Name.size, (const char*)cmd.Name.str, (u32)i);
            UI_Interaction slider = UISliderRow((const char*)rowId.str, &value, cmd.Min, cmd.Max, "%.3g");
            if(slider.dragging || slider.clicked)
                cmd.SetFloat(value);
        }

        UIEndScrollArea();
        UIEndPanel();
    }

    namespace
    {
        // Render settings as they were when the explorer opened, for Reset.
        Scene::SceneRenderSettings s_ExplorerSnapshot;
        Scene* s_ExplorerSnapshotScene = nullptr;

        // Index = ToneMapIndex in ToneMapping.frag
        const char* kToneMapNames[] = { "None", "Linear", "Reinhard (Jodie)", "Reinhard (luma)", "Reinhard (white preserving)", "Uncharted 2", "ACES" };
    }

    void DebugMenu::DrawFrameExplorer()
    {
        Graphics::SceneRenderer* renderer = ExplorerRenderer();
        Scene* scene                      = CurrentScene();
        if(!renderer || !scene)
            return;
        // docked = a tool panel's body: no window of its own, the width it is given
        const bool docked = m_DockedSize.x > 0.0f;

        Scene::SceneRenderSettings& rs = scene->GetSettings().RenderSettings;
        if(!m_ExplorerWasOpen || s_ExplorerSnapshotScene != scene)
        {
            s_ExplorerSnapshot      = rs;
            s_ExplorerSnapshotScene = scene;
            m_ExplorerWasOpen       = true;
            m_ExplorerScrolledTo    = -1;
        }

        UI_State* ui       = GetUIState();
        Arena* fa          = ui->UIFrameArena;
        const float dpi    = ui->DPIScale;
        const Vec2 view    = ui->root_parent.size;
        const float panelW = docked ? m_DockedSize.x : Maths::Min(380.0f * dpi, view.x * 0.42f);
        const float wrapW  = panelW / dpi - (docked ? 22.0f : 46.0f);
        const bool paused  = Engine::GetTimeStep().GetTimeScale() == 0.0;

        if(!docked)
        {
            UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
            UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
            UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
            UIBeginOverlay("debugframeexplorer", SizeKind_Pixels, panelW, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically);
            UIWindowAnchor(UIAnchor_MiddleRight, 12.0f * dpi, 0.0f);
            UIPopStyle(StyleVar_ItemSpacing);
            UIPopStyle(StyleVar_Padding);
            UIPopStyle(StyleVar_BackgroundColor);

            UIBeginRowFullWidth();
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelCStr("fxtitle", "Frame Explorer");
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            if(UIButton("X###fxclose").clicked)
                m_Panels[DebugPanel_FrameExplorer] = false;
            UIEndRow();
            UISeparator();
        }

        UIBeginRowFullWidth();
        GetUIState()->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
        if(UIButton("< Prev###fxprev").clicked)
            StepPass(-1);
        if(UIButton("Next >###fxnext").clicked)
            StepPass(1);
        if(UIButton("Full frame###fxfull").clicked)
            SetPassCutoff(-1);
        UIFlexSpacer();
        if(UIButton(paused ? "Resume###fxfreeze" : "Freeze###fxfreeze").clicked)
            Engine::GetTimeStep().SetTimeScale(paused ? 1.0 : 0.0);
        UIEndRow();

        const i32 cutoff = renderer->GetPassCutoff();

        const float rowH   = 22.0f;
        const float stride = (rowH + 1.0f) * dpi;
        const float listH  = Maths::Min(264.0f * dpi, view.y * 0.3f);

        if(cutoff != m_ExplorerScrolledTo)
        {
            m_ExplorerScrolledTo = cutoff;
            m_ExplorerInfoScroll = 0.0f;
            if(cutoff >= 0)
            {
                const float top    = cutoff * stride;
                const float bottom = top + rowH * dpi;
                if(top < m_ExplorerScroll)
                    m_ExplorerScroll = top;
                else if(bottom > m_ExplorerScroll + listH - 6.0f * dpi)
                    m_ExplorerScroll = bottom - (listH - 6.0f * dpi);
            }
        }

        auto clearAreaSkin = [dpi]()
        {
            UI_Widget* area                            = GetUIState()->parents.Back();
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
            area->style_vars[StyleVar_Padding]         = Vec4(10.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        };

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 1.0f * dpi));
        UIBeginScrollArea("fxlist", listH, &m_ExplorerScroll);
        clearAreaSkin();

        for(u32 i = 0; i < Graphics::RenderPass_Count; i++)
        {
            const Graphics::RenderPassInfo& info  = Graphics::GetRenderPassInfo((Graphics::RenderPassID)i);
            const Graphics::RenderPassRecord& rec = renderer->GetPassRecord((Graphics::RenderPassID)i);
            const bool ran                        = rec.State == Graphics::RenderPassState::Ran;
            const bool selected                   = (i32)i == cutoff;

            String8 rowId      = PushStr8F(fa, "row###fxrow%u", i);
            UI_Interaction row = UIBeginSelectableRow((const char*)rowId.str, selected, rowH);

            if(!ran)
                UIPushStyle(StyleVar_TextColor, kDimText);
            String8 nameId = PushStr8F(fa, "n###fxname%u", i);
            UILabel((const char*)nameId.str, PushStr8F(fa, "%02u  %s", i + 1, info.Name));
            if(!ran)
                UIPopStyle(StyleVar_TextColor);

            UIFlexSpacer();

            String8 badge = { 0 };
            if(!ran)
                badge = rec.State == Graphics::RenderPassState::CutOff ? Str8Lit("cut") : Str8Lit("off");
            else if(rec.DrawCalls > 0)
                badge = PushStr8F(fa, "%u draw%s  %.2f ms", rec.DrawCalls, rec.DrawCalls == 1 ? "" : "s", rec.CpuMs);
            else
                badge = PushStr8F(fa, "%.2f ms", rec.CpuMs);
            String8 badgeId = PushStr8F(fa, "b###fxbadge%u", i);
            UIBadge((const char*)badgeId.str, badge, kOffBG, ran ? kBadgeText : kDimText);

            UIEndSelectableRow();

            // clicking the current pass again goes back to the whole frame
            if(row.clicked)
                SetPassCutoff(selected ? -1 : (i32)i);
        }

        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);
        UISeparator();

        const Graphics::RenderPassInfo& info = Graphics::GetRenderPassInfo((Graphics::RenderPassID)Maths::Max(cutoff, 0));

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginScrollArea("fxinfo", Maths::Min(400.0f * dpi, view.y * 0.44f), &m_ExplorerInfoScroll);
        clearAreaSkin();

        if(cutoff < 0)
        {
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelWrapped("fxintro", Str8Lit("Showing the whole frame. Pick a pass, or press ] and [, to stop the frame there and see what has been drawn so far. Freeze pauses the game so every step shows the same frame - the fly camera still works."), wrapW);
            UIPopStyle(StyleVar_TextColor);
        }
        else
        {
            UIBeginRowFullWidth();
            UIPushStyle(StyleVar_TextColor, kAccent);
            UILabelCStr("fxpassname", info.Name);
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            UIBadge("c###fxpasscat", Str8C((char*)info.Category), kBadgeBG, kBadgeText);
            UIEndRow();

            UILabelWrapped("fxsummary", Str8C((char*)info.Summary), wrapW);

            auto field = [&](const char* title, const char* text)
            {
                UISpacer(2.0f);
                UIPushStyle(StyleVar_TextColor, kDimText);
                String8 titleId = PushStr8F(fa, "t###fxfieldt%s", title);
                UILabelCStr((const char*)titleId.str, title);
                UIPopStyle(StyleVar_TextColor);
                String8 textId = PushStr8F(fa, "v###fxfieldv%s", title);
                UILabelWrapped((const char*)textId.str, Str8C((char*)text), wrapW);
            };
            field("Reads", info.Reads);
            field("Writes", info.Writes);
            field("What to look for", info.LookFor);

            UISpacer(4.0f);
            if(cutoff < (i32)Graphics::RenderPass_ToneMapping)
            {
                bool tonemap = renderer->GetPassPreviewTonemap();
                if(UIToggle("Tonemap preview###fxtonemap", &tonemap).clicked)
                    renderer->SetPassPreviewTonemap(tonemap);
            }
            if(info.BufferView != 0)
                UIToggle("Show buffer###fxbuffer", &m_ExplorerShowBuffer);

            DrawPassTweaks(cutoff, wrapW);
        }

        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);
        if(!docked)
            UIEndPanel();

        rs.DebugMode = (cutoff >= 0 && m_ExplorerShowBuffer) ? info.BufferView : 0;
    }

    void DebugMenu::DrawPassTweaks(i32 pass, float wrapWidth)
    {
        Scene* scene = CurrentScene();
        if(!scene)
            return;

        using namespace Graphics;
        Scene::SceneRenderSettings& rs       = scene->GetSettings().RenderSettings;
        const Scene::SceneRenderSettings* snap = s_ExplorerSnapshotScene == scene ? &s_ExplorerSnapshot : nullptr;
        const QualitySettings& quality       = Application::Get().GetQualitySettings();
        Arena* fa                            = GetUIState()->UIFrameArena;

        UISpacer(4.0f);
        UIBeginRowFullWidth();
        UIPushStyle(StyleVar_TextColor, kAccent);
        UILabelCStr("fxsettings", "Settings");
        UIPopStyle(StyleVar_TextColor);
        UIFlexSpacer();
        const bool resetting = UIButton("Reset###fxreset").clicked && snap;
        UIEndRow();

        u32 row = 0;

        // Reset copies the field back from the snapshot instead of needing a table of them.
        auto restore = [&](void* field, size_t size)
        {
            if(resetting)
                MemoryCopy(field, (const u8*)snap + ((const u8*)field - (const u8*)&rs), size);
        };

        auto note = [&](const char* text)
        {
            if(!text)
                return;
            UIPushStyle(StyleVar_TextColor, kDimText);
            String8 id = PushStr8F(fa, "n###fxnote%d_%u", pass, row++);
            UILabelWrapped((const char*)id.str, Str8C((char*)text), wrapWidth);
            UIPopStyle(StyleVar_TextColor);
        };

        auto tBool = [&](const char* label, bool& value, const char* tip = nullptr)
        {
            restore(&value, sizeof(value));
            String8 id = PushStr8F(fa, "%s###fxt%d_%u", label, pass, row++);
            UIToggle((const char*)id.str, &value);
            note(tip);
        };

        auto tFloat = [&](const char* label, float& value, float minValue, float maxValue, const char* tip = nullptr)
        {
            restore(&value, sizeof(value));
            String8 id = PushStr8F(fa, "%s###fxt%d_%u", label, pass, row++);
            UISliderRow((const char*)id.str, &value, minValue, maxValue, "%.3g");
            note(tip);
        };

        auto tInt = [&](const char* label, int& value, int minValue, int maxValue, const char* tip = nullptr)
        {
            restore(&value, sizeof(value));
            float f    = (float)value;
            String8 id = PushStr8F(fa, "%s###fxt%d_%u", label, pass, row++);
            UISliderRow((const char*)id.str, &f, (float)minValue, (float)maxValue, "%.0f");
            value = Maths::Clamp((int)(f + 0.5f), minValue, maxValue);
            note(tip);
        };

        auto qualityNote = [&](bool allowed)
        {
            if(!allowed)
                note("Switched off by the current quality preset, so this pass will not run.");
        };

        switch(pass)
        {
        case RenderPass_DepthPrePass:
            tBool("Depth pre-pass", rs.DepthPrePass, "Still runs while SSAO or reflections are on - they need its buffers.");
            break;
        case RenderPass_SSAO:
            qualityNote(quality.EnableSSAO);
            tBool("Enabled", rs.SSAOEnabled);
            tFloat("Sample radius", rs.SSAOSampleRadius, 0.1f, 8.0f, "How far around each pixel to look. Small = tight contact shadows, large = soft room-scale darkening.");
            tFloat("Strength", rs.SSAOStrength, 0.0f, 4.0f, "How dark occluded areas get.");
            break;
        case RenderPass_SSAOBlur:
            tBool("Blur", rs.SSAOBlur, "Off shows the raw sample noise in the lit image.");
            tInt("Blur radius", rs.SSAOBlurRadius, 1, 8);
            break;
        case RenderPass_ContactShadows:
            tBool("Enabled", rs.ContactShadowsEnabled, "Needs a directional light. Turns the depth pre-pass on.");
            tFloat("Ray length", rs.ContactShadowLength, 0.02f, 2.0f, "How far towards the sun each pixel looks, in metres. Longer catches more but starts to show its screen-space limits.");
            tFloat("Thickness", rs.ContactShadowThickness, 0.01f, 1.0f, "How thick things are assumed to be behind what the depth buffer shows. Too thin and rays slip behind objects, too thick and things cast shadows that are too long.");
            tInt("Steps", rs.ContactShadowSteps, 4, 64);
            break;
        case RenderPass_GI:
            tBool("Enabled", rs.GIEnabled, "Static models only: anything with a moving rigid body or an animated mesh is lit by the probes but not seen by them.");
            tFloat("Probe spacing", rs.GIProbeSpacing, 0.25f, 16.0f, "Metres between probes. Closer = more detail, more probes to trace. The grid is capped and widens itself to fit.");
            tFloat("Intensity", rs.GIIntensity, 0.0f, 4.0f);
            tFloat("Bounce", rs.GIBounce, 0.0f, 1.0f, "How much of the probes' own light the rays carry on. 0 = one bounce only.");
            tFloat("Hysteresis", rs.GIHysteresis, 0.5f, 0.995f, "Share of the old value each update keeps. Higher is smoother and slower to catch a moving sun.");
            tInt("Update stride", rs.GIUpdateStride, 1, 16, "One probe in this many is traced each frame.");
            break;
        case RenderPass_VolumetricFog:
            tBool("Enabled", rs.VolumetricFogEnabled, "Replaces the Environment's flat fog while on.");
            tFloat("Density", rs.VolumetricFogDensity, 0.0f, 0.2f, "How much light the air stops per metre at the base height.");
            tFloat("Height falloff", rs.VolumetricFogFalloff, 0.0f, 1.0f, "How fast it thins going up. 0 = the same everywhere.");
            tFloat("Base height", rs.VolumetricFogBaseHeight, -100.0f, 100.0f);
            tFloat("Anisotropy", rs.VolumetricFogAnisotropy, -0.9f, 0.9f, "Positive scatters light forwards: the fog glows looking towards the sun and shafts stand out.");
            tFloat("Sun", rs.VolumetricFogSun, 0.0f, 4.0f);
            tFloat("Ambient", rs.VolumetricFogAmbient, 0.0f, 4.0f, "Sky light scattered in. Fog in full shadow is lit by this alone.");
            tFloat("Distance", rs.VolumetricFogDistance, 10.0f, 1000.0f);
            tInt("Steps", rs.VolumetricFogSteps, 4, 96);
            break;
        case RenderPass_TAA:
            tBool("Enabled", rs.TAAEnabled, "Works with FXAA / SMAA off. Turn Sharpen on if the picture looks soft.");
            tFloat("History (still)", rs.TAAHistoryWeight, 0.5f, 0.98f, "Share of the accumulated picture kept each frame. Higher = smoother edges, slower to settle.");
            tFloat("History (moving)", rs.TAAMotionWeight, 0.0f, 0.98f, "The same for pixels that moved a lot, where old frames smear.");
            tFloat("Clip", rs.TAAClipGamma, 0.5f, 3.0f, "How far history may stray from this frame's colours. Lower stops ghosting, higher stops flicker.");
            break;
        case RenderPass_Shadow:
            qualityNote(quality.EnableShadows);
            tBool("Enabled", rs.ShadowsEnabled);
            {
                // shadow fitting is renderer state, shared by every scene
                auto& shadows = ExplorerRenderer()->GetShadowData();
                auto sFloat   = [&](const char* label, float& value, float minValue, float maxValue, const char* tip)
                {
                    String8 id = PushStr8F(fa, "%s###fxt%d_%u", label, pass, row++);
                    UISliderRow((const char*)id.str, &value, minValue, maxValue, "%.3g");
                    note(tip);
                };
                sFloat("Max distance", shadows.m_MaxShadowDistance, 20.0f, 1000.0f, "The shadow maps are stretched over this range. Shorter = sharper, since the same texels cover less ground.");
                sFloat("Cascade split", shadows.m_CascadeSplitLambda, 0.0f, 1.0f, "0 spaces cascades evenly, 1 logarithmically - more detail close to the camera.");
                sFloat("Softness", shadows.m_LightSize, 0.0f, 4.0f, "Blur radius in shadow texels. Bigger light = softer edges.");
                sFloat("Bias", shadows.m_InitialBias, 0.0f, 4.0f, "Offset in shadow texels that stops surfaces shadowing themselves. Too low = stripes (acne), too high = shadows detach from objects.");
            }
            tInt("Max spot shadows", rs.MaxSpotShadows, 0, 16, "Spot lights with Cast Shadows each get a tile of a shadow atlas, nearest the camera first. Each one draws the scene again from the light.");
            tFloat("Spot shadow distance", rs.SpotShadowDistance, 5.0f, 500.0f, "Spot lights further than this from the camera cast no shadow; they fade out over the last fifth.");
            break;
        case RenderPass_Forward:
            tBool("3D renderer", rs.Renderer3DEnabled);
            tBool("Wireframe", rs.Wireframe, "Shows the triangles actually being drawn.");
            break;
        case RenderPass_Skybox:
            tBool("Enabled", rs.SkyboxRenderEnabled);
            tFloat("Mip level", rs.SkyboxMipLevel, 0.0f, 8.0f, "Higher mips are the blurrier copies of the cubemap that rough materials reflect.");
            break;
        case RenderPass_2D:
        case RenderPass_2DLit:
            tBool("2D renderer", rs.Renderer2DEnabled);
            break;
        case RenderPass_SSR:
            qualityNote(quality.EnableSSR);
            tBool("Enabled", rs.SSREnabled);
            tFloat("Max distance", rs.SSRMaxDistance, 1.0f, 100.0f, "How far a reflection ray travels before giving up.");
            tInt("Steps", rs.SSRMaxSteps, 4, 128, "March steps per ray. Fewer = faster, but rays skip over thin objects.");
            tInt("Refine steps", rs.SSRBinarySteps, 0, 16, "Binary search after a hit to find the exact surface.");
            tFloat("Thickness", rs.SSRThickness, 0.01f, 2.0f, "How thick surfaces are assumed to be. The depth buffer only knows the front of things.");
            tFloat("Strength", rs.SSRStrength, 0.0f, 1.0f);
            tFloat("Max roughness", rs.SSRMaxRoughness, 0.0f, 1.0f, "Rougher surfaces than this skip reflections.");
            tFloat("Environment match", rs.SSREnvIntensity, 0.0f, 2.0f, "How much of the forward pass's environment reflection a screen hit replaces. 1 = matches it.");
            break;
        case RenderPass_DepthOfField:
            tBool("Enabled", rs.DepthOfFieldEnabled);
            tFloat("Focus distance", rs.DepthOfFieldDistance, 0.0f, 500.0f);
            tFloat("Strength", rs.DepthOfFieldStrength, 0.0f, 5.0f);
            break;
        case RenderPass_Bloom:
            qualityNote(quality.EnableBloom);
            tBool("Enabled", rs.BloomEnabled);
            tFloat("Threshold", rs.BloomThreshold, 0.0f, 5.0f, "Only pixels brighter than this glow. Above 1 means only true HDR highlights.");
            tFloat("Knee", rs.BloomKnee, 0.0f, 1.0f, "Softens the threshold so glow fades in instead of popping.");
            tFloat("Intensity", rs.m_BloomIntensity, 0.0f, 5.0f);
            tFloat("Upsample scale", rs.BloomUpsampleScale, 0.0f, 2.0f, "Blur radius on the way back up - how wide the glow spreads.");
            break;
        case RenderPass_Debanding:
            tBool("Enabled", rs.DebandingEnabled);
            break;
        case RenderPass_EyeAdaptation:
            tBool("Adaptive exposure", rs.AdaptiveExposure);
            tFloat("Target luminance", rs.AdaptiveTargetLuminance, 0.01f, 1.0f, "The average brightness exposure aims for. 0.18 is photographic middle grey.");
            tFloat("Speed up", rs.AdaptiveSpeedUp, 0.1f, 10.0f, "How fast it adapts when the scene gets brighter.");
            tFloat("Speed down", rs.AdaptiveSpeedDown, 0.1f, 10.0f, "How fast it adapts when the scene gets darker.");
            break;
        case RenderPass_ToneMapping:
        {
            restore(&rs.m_ToneMapIndex, sizeof(rs.m_ToneMapIndex));
            int index = Maths::Clamp((int)rs.m_ToneMapIndex, 0, 6);
            UIDropdown("Operator###fxtonemapop", &index, kToneMapNames, 7);
            rs.m_ToneMapIndex = (uint32_t)Maths::Clamp(index, 0, 6);
            note("None clips anything over 1 to white. The others compress highlights in different ways.");
            tFloat("Brightness", rs.Brightness, -1.0f, 1.0f);
            tFloat("Contrast", rs.Contrast, 0.0f, 2.0f);
            tFloat("Saturation", rs.Saturation, 0.0f, 2.0f);
            break;
        }
        case RenderPass_MotionBlur:
            tBool("Enabled", rs.MotionBlurEnabled);
            tFloat("Strength", rs.MotionBlurStrength, 0.0f, 2.0f);
            tInt("Samples", rs.MotionBlurSamples, 2, 32, "Taps along the motion direction. Too few shows as separate ghost copies.");
            break;
        case RenderPass_Vignette:
            tBool("Enabled", rs.VignetteEnabled);
            tFloat("Intensity", rs.VignetteIntensity, 0.0f, 2.0f);
            tFloat("Smoothness", rs.VignetteSmoothness, 0.0f, 1.0f);
            tFloat("Roundness", rs.VignetteRoundness, 0.0f, 2.0f);
            break;
        case RenderPass_Sharpen:
            tBool("Enabled", rs.SharpenEnabled);
            break;
        case RenderPass_SMAA:
            qualityNote(quality.EnableFXAA);
            tBool("Enabled", rs.SMAAEnabled, "Takes priority over FXAA when both are on.");
            break;
        case RenderPass_FXAA:
            qualityNote(quality.EnableFXAA);
            tBool("Enabled", rs.FXAAEnabled);
            break;
        case RenderPass_ChromaticAberation:
            tBool("Enabled", rs.ChromaticAberationEnabled);
            break;
        case RenderPass_FilmicGrain:
            tBool("Enabled", rs.FilmicGrainEnabled);
            break;
        case RenderPass_Debug:
            tBool("Enabled", rs.DebugRenderEnabled);
            break;
        default:
            note("Nothing to tweak on this pass.");
            break;
        }
    }

    namespace
    {
        struct PhysicsStageInfo
        {
            const char* Name;
            const char* Summary;
            const char* DebugView;
            const char* LookFor;
            u32 DebugFlags;
        };

        // Same order as PhysicsStage.
        const PhysicsStageInfo kPhysicsStages[PhysicsStage_Count] = {
            { "Broadphase",
              "Finds pairs of bodies that might be touching, using only their bounding boxes. Testing every body against every other is n squared, so bodies are sorted into a spatial structure and only neighbours are paired up.",
              "Boxes are each body's AABB, the grid is the spatial partition, lines join candidate pairs.",
              "Pairs here are only maybes. Two boxes can overlap while the shapes inside are nowhere near each other - the next stage sorts that out. Compare the pair count with the octree on and off.",
              PhysicsDebugFlags::BROADPHASE | PhysicsDebugFlags::BROADPHASE_PAIRS | PhysicsDebugFlags::AABB },
            { "Narrowphase",
              "Runs the exact shape-against-shape test on every candidate pair. For each real collision it builds a manifold: the contact points, the collision normal and how deep the shapes overlap.",
              "Collision shapes, plus contact points and normals for every manifold found.",
              "Pair count drops to manifold count - most candidates were false alarms. A box resting flat on the floor needs several contact points; one point would let it rock.",
              PhysicsDebugFlags::COLLISIONVOLUMES | PhysicsDebugFlags::MANIFOLD | PhysicsDebugFlags::COLLISIONNORMALS },
            { "Forces",
              "Applies gravity and other forces to velocities. Nothing has moved yet - a force changes velocity, and velocity changes position later.",
              "Velocity and force arrows on each body.",
              "A resting box gains downward velocity here every single step. It only stays put because the solver removes that velocity again in the next stage.",
              PhysicsDebugFlags::LINEARVELOCITY | PhysicsDebugFlags::LINEARFORCE },
            { "Solve",
              "Fixes velocities so contacts and joints are respected. Each contact pushes its two bodies apart with an impulse; because fixing one contact disturbs its neighbours, the whole set is repeated several times until it settles.",
              "Manifolds, constraints and the corrected velocities.",
              "Drop the iteration count to 1 on a stack of boxes and watch it sag and jitter. Warm starting reuses last step's impulses as the first guess, so fewer iterations are needed.",
              PhysicsDebugFlags::MANIFOLD | PhysicsDebugFlags::CONSTRAINT | PhysicsDebugFlags::LINEARVELOCITY },
            { "Integrate",
              "Moves every body by its velocity over the timestep. This is the only stage where positions and rotations change.",
              "Collision shapes at their new positions, with velocities.",
              "The timestep is fixed, not the frame time - that keeps the simulation stable and repeatable. Different integrators trade accuracy for cost; explicit Euler gains energy and is the one that explodes.",
              PhysicsDebugFlags::COLLISIONVOLUMES | PhysicsDebugFlags::LINEARVELOCITY },
            { "Rest Test",
              "Bodies that have barely moved for a while are put to sleep and skipped by every stage until something hits them.",
              "Bounding boxes - sleeping bodies are drawn in a different colour.",
              "Sleeping is why a level full of settled crates costs almost nothing. It uses a moving average of speed, so one still frame at the top of a bounce does not send a body to sleep.",
              PhysicsDebugFlags::AABB | PhysicsDebugFlags::COLLISIONVOLUMES },
        };

        bool SliderChanged(const UI_Interaction& slider)
        {
            return slider.dragging || slider.clicked;
        }

        const char* kIntegratorNames[] = { "Explicit Euler", "Semi-implicit Euler", "Runge-Kutta 2", "Runge-Kutta 4" };
    }

    void DebugMenu::DrawPhysicsExplorer()
    {
        LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>();
        Scene* scene                = CurrentScene();
        if(!physics || !scene)
            return;

        if(!m_PhysicsWasOpen)
        {
            m_PhysicsWasOpen    = true;
            m_PhysicsSavedFlags = physics->GetDebugDrawFlags();
        }

        UI_State* ui       = GetUIState();
        Arena* fa          = ui->UIFrameArena;
        const float dpi    = ui->DPIScale;
        const Vec2 view    = ui->root_parent.size;
        const bool docked  = m_DockedSize.x > 0.0f;
        const float panelW = docked ? m_DockedSize.x : Maths::Min(380.0f * dpi, view.x * 0.42f);
        const float wrapW  = panelW / dpi - (docked ? 22.0f : 46.0f);
        const bool manual  = physics->GetManualStep();

        if(!docked)
        {
            UIPushStyle(StyleVar_BackgroundColor, kPanelBG);
            UIPushStyle(StyleVar_Padding, Vec2(12.0f * dpi, 10.0f * dpi));
            UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
            UIBeginOverlay("debugphysicsexplorer", SizeKind_Pixels, panelW, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically);
            UIWindowAnchor(UIAnchor_MiddleLeft, 12.0f * dpi, 0.0f);
            UIPopStyle(StyleVar_ItemSpacing);
            UIPopStyle(StyleVar_Padding);
            UIPopStyle(StyleVar_BackgroundColor);

            UIBeginRowFullWidth();
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelCStr("pxtitle", "Physics Explorer");
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            if(UIButton("X###pxclose").clicked)
                m_Panels[DebugPanel_PhysicsExplorer] = false;
            UIEndRow();
            UISeparator();
        }

        UIBeginRowFullWidth();
        GetUIState()->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
        if(UIButton("Step stage###pxstage").clicked)
        {
            physics->SetManualStep(true);
            physics->StepStages(1);
        }
        if(UIButton("Step substep###pxsubstep").clicked)
        {
            physics->SetManualStep(true);
            physics->StepSubstep();
        }
        UIFlexSpacer();
        if(UIButton(manual ? "Run###pxrun" : "Pause###pxrun").clicked)
            physics->SetManualStep(!manual);
        UIEndRow();

        // while stepping, the card follows the stage that just ran
        const PhysicsStage last = physics->GetLastStage();
        if(manual && last != PhysicsStage_Count)
            m_PhysicsSelected = (i32)last;

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 1.0f * dpi));
        for(u32 i = 0; i < PhysicsStage_Count; i++)
        {
            const bool selected = (i32)i == m_PhysicsSelected;
            String8 rowId       = PushStr8F(fa, "row###pxrow%u", i);
            UI_Interaction row  = UIBeginSelectableRow((const char*)rowId.str, selected, 22.0f);

            String8 nameId = PushStr8F(fa, "n###pxname%u", i);
            UILabel((const char*)nameId.str, PushStr8F(fa, "%u  %s", i + 1, kPhysicsStages[i].Name));
            UIFlexSpacer();
            if(manual && i == (u32)physics->GetNextStage())
            {
                String8 nextId = PushStr8F(fa, "x###pxnext%u", i);
                UIBadge((const char*)nextId.str, Str8Lit("next"), kOnBG, Vec4(1.0f));
            }
            String8 msId = PushStr8F(fa, "m###pxms%u", i);
            UIBadge((const char*)msId.str, PushStr8F(fa, "%.2f ms", physics->GetStageMs((PhysicsStage)i)), kOffBG, kBadgeText);
            UIEndSelectableRow();

            if(row.clicked)
            {
                // stepping: run up to and including the clicked stage
                if(manual)
                    physics->StepStages(((i + PhysicsStage_Count - (u32)physics->GetNextStage()) % PhysicsStage_Count) + 1);
                m_PhysicsSelected = (i32)i;
            }
        }
        UIPopStyle(StyleVar_ItemSpacing);

        const PhysicsStats3D& stats = physics->GetStats();
        const u32 awake             = stats.RigidBodyCount - Maths::Min(stats.RigidBodyCount, stats.RestCount);
        UIPushStyle(StyleVar_TextColor, kDimText);
        UILabel("pxstats", PushStr8F(fa, "%u bodies  %u awake  %d pairs  %u manifolds  %u constraints",
                                      stats.RigidBodyCount, awake, physics->GetNumberCollisionPairs(),
                                      physics->GetManifoldCount(), stats.ConstraintCount));
        UIPopStyle(StyleVar_TextColor);
        UISeparator();

        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 3.0f * dpi));
        UIBeginScrollArea("pxinfo", Maths::Min(420.0f * dpi, view.y * 0.46f), &m_PhysicsInfoScroll);
        {
            UI_Widget* area                            = GetUIState()->parents.Back();
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
            area->style_vars[StyleVar_Padding]         = Vec4(10.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        }

        auto note = [&](const char* id, const char* text)
        {
            UIPushStyle(StyleVar_TextColor, kDimText);
            UILabelWrapped(id, Str8C((char*)text), wrapW);
            UIPopStyle(StyleVar_TextColor);
        };

        if(physics->IsPaused())
            note("pxpaused", "The physics engine is paused, so stepping does nothing.");

        if(m_PhysicsSelected < 0)
        {
            note("pxintro", "The simulation runs in fixed substeps, and every substep is these six stages in order. Step stage runs one and stops, with the debug view switched to what that stage just produced. Click a stage to read about it.");
        }
        else
        {
            const PhysicsStageInfo& info = kPhysicsStages[m_PhysicsSelected];

            UIPushStyle(StyleVar_TextColor, kAccent);
            UILabelCStr("pxstagename", info.Name);
            UIPopStyle(StyleVar_TextColor);
            UILabelWrapped("pxsummary", Str8C((char*)info.Summary), wrapW);

            auto field = [&](const char* title, const char* text)
            {
                UISpacer(2.0f);
                UIPushStyle(StyleVar_TextColor, kDimText);
                String8 titleId = PushStr8F(fa, "t###pxfieldt%s", title);
                UILabelCStr((const char*)titleId.str, title);
                UIPopStyle(StyleVar_TextColor);
                String8 textId = PushStr8F(fa, "v###pxfieldv%s", title);
                UILabelWrapped((const char*)textId.str, Str8C((char*)text), wrapW);
            };
            field("In the debug view", info.DebugView);
            field("What to look for", info.LookFor);

            UISpacer(4.0f);
            UIToggle("Debug view follows stage###pxautodraw", &m_PhysicsAutoDraw);
            if(m_PhysicsAutoDraw)
            {
                physics->SetDebugDrawFlags(info.DebugFlags);
                scene->GetSettings().RenderSettings.DebugRenderEnabled = true;
            }

            UISpacer(4.0f);
            UIPushStyle(StyleVar_TextColor, kAccent);
            UILabelCStr("pxsettings", "Settings");
            UIPopStyle(StyleVar_TextColor);

            switch(m_PhysicsSelected)
            {
            case PhysicsStage_Broadphase:
            {
                bool octree = physics->GetBroadphaseType() == BroadphaseType::OCTREE;
                if(UIToggle("Octree###pxoctree", &octree).clicked)
                    physics->SetBroadphaseType(octree ? BroadphaseType::OCTREE : BroadphaseType::BRUTE_FORCE);
                note("pxoctreetip", "Off = brute force: every body tested against every other. Same result, far more box tests.");
                break;
            }
            case PhysicsStage_Narrowphase:
            {
                bool parallel = physics->GetParallelNarrowphase();
                if(UIToggle("Parallel###pxparallel", &parallel).clicked)
                    physics->SetParallelNarrowphase(parallel);
                note("pxparalleltip", "Pairs do not depend on each other, so they can be tested on several threads at once.");
                break;
            }
            case PhysicsStage_Forces:
            {
                Vec3 gravity = physics->GetGravity();
                if(SliderChanged(UISliderRow("Gravity Y###pxgravity", &gravity.y, -30.0f, 10.0f, "%.2f")))
                    physics->SetGravity(gravity);
                float damping = physics->GetDampingFactor();
                if(SliderChanged(UISliderRow("Damping###pxdamping", &damping, 0.9f, 1.0f, "%.4f")))
                    physics->SetDampingFactor(damping);
                note("pxdampingtip", "Velocity is multiplied by this every substep. 1 = nothing ever slows down on its own.");
                break;
            }
            case PhysicsStage_Solve:
            {
                float iterations = (float)physics->GetVelocityIterations();
                if(SliderChanged(UISliderRow("Iterations###pxveliters", &iterations, 1.0f, 30.0f, "%.0f")))
                    physics->SetVelocityIterations((u32)(iterations + 0.5f));
                note("pxveliterstip", "Passes over all contacts per substep. More = stiffer stacks, more CPU.");
                bool warm = physics->GetWarmStartingEnabled();
                if(UIToggle("Warm starting###pxwarm", &warm).clicked)
                    physics->SetWarmStartingEnabled(warm);
                break;
            }
            case PhysicsStage_Integrate:
            {
                int integrator = Maths::Clamp((int)physics->GetIntegrationType(), 0, 3);
                UIDropdown("Integrator###pxintegrator", &integrator, kIntegratorNames, 4);
                physics->SetIntegrationType((IntegrationType)Maths::Clamp(integrator, 0, 3));
                float iterations = (float)physics->GetPositionIterations();
                if(SliderChanged(UISliderRow("Sub-steps###pxpositers", &iterations, 1.0f, 8.0f, "%.0f")))
                    physics->SetPositionIterations((u32)(iterations + 0.5f));
                note("pxpositerstip", "The move is split into this many smaller moves.");
                break;
            }
            default:
                note("pxnosettings", "Nothing to tweak on this stage.");
                break;
            }

            if(!manual)
            {
                float timeScale = physics->GetTimeScale();
                if(SliderChanged(UISliderRow("Time scale###pxtimescale", &timeScale, 0.0f, 2.0f, "%.2f")))
                    physics->SetTimeScale(timeScale);
            }
        }

        // body under the inspector selection
        UISpacer(4.0f);
        UIPushStyle(StyleVar_TextColor, kAccent);
        UILabelCStr("pxbody", "Selected body");
        UIPopStyle(StyleVar_TextColor);

        auto& reg              = scene->GetRegistry();
        const entt::entity sel = (entt::entity)m_SelectedEntity;
        RigidBody3D* body      = nullptr;
        if(m_SelectedEntity != 0xFFFFFFFFu && reg.valid(sel))
        {
            if(RigidBody3DComponent* comp = reg.try_get<RigidBody3DComponent>(sel))
                body = comp->GetRigidBody();
        }

        if(!body)
        {
            note("pxnobody", "Select an entity with a 3D rigid body (Entity List, F7) to watch its state change stage by stage.");
        }
        else
        {
            const Vec3& v      = body->GetLinearVelocity();
            const Vec3& w      = body->GetAngularVelocity();
            const float invM   = body->GetInverseMass();
            const char* state  = body->GetIsStatic() ? "static" : (body->IsAwake() ? "awake" : "asleep");
            String8 label      = EntityLabel(fa, reg, sel);
            UILabel("pxbodyname", PushStr8F(fa, "%.*s  (%s)", (int)label.size, (const char*)label.str, state));
            if(invM > 0.0f)
                UILabel("pxbodymass", PushStr8F(fa, "Mass  %.2f", 1.0f / invM));
            else
                UILabelCStr("pxbodymass", "Mass  infinite (never moved by collisions)");
            UILabel("pxbodyvel", PushStr8F(fa, "Velocity  %.2f  %.2f  %.2f   (%.2f m/s)", v.x, v.y, v.z, Maths::Length(v)));
            UILabel("pxbodyang", PushStr8F(fa, "Spin  %.2f  %.2f  %.2f", w.x, w.y, w.z));
            const Vec3& pos = body->GetPosition();
            UILabel("pxbodypos", PushStr8F(fa, "Position  %.2f  %.2f  %.2f", pos.x, pos.y, pos.z));
        }

        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);
        if(!docked)
            UIEndPanel();
    }
}
