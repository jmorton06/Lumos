#include "SceneViewPanel.h"
#include "Editor.h"
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/EntityFactory.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/SwapChain.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/VoxelWorldComponent.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <Lumos/Graphics/Renderers/GridRenderer.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Events/ApplicationEvent.h>
#include <Lumos/Graphics/Sprite.h>
#include <Lumos/Maths/Ray.h>

#include "TerrainEditorPanel.h"

#include <box2d/box2d.h>
#include <imgui/imgui_internal.h>
#include <imgui/Plugins/ImGuizmo.h>
#include <sol/sol.hpp>

namespace Lumos
{
    SceneViewPanel::SceneViewPanel()
    {
        m_Name         = "Scene###scene";
        m_SimpleName   = "Scene";
        m_CurrentScene = nullptr;

        m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
        m_ShowComponentGizmoMap[typeid(Camera).hash_code()]          = true;
        m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]  = true;

        m_Width  = 800;
        m_Height = 600;

        m_bRequestCameraUpdate = true;

        Application::Get().GetSceneRenderer()->SetDisablePostProcess(false);
    }

    void SceneViewPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        Application& app = Application::Get();

        ImGuiUtilities::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if(!ImGuiUtilities::BeginPanel(m_Name.c_str(), nullptr, flags) || !m_CurrentScene)
        {
            app.SetDisableMainSceneRenderer(true);
            ImGui::End();
            return;
        }

        Camera* camera              = nullptr;
        Maths::Transform* transform = nullptr;

        app.SetDisableMainSceneRenderer(false);

        // if(app.GetEditorState() == EditorState::Preview)
        {
            LUMOS_PROFILE_SCOPE("Set Override Camera");
            camera    = m_Editor->GetCamera();
            transform = &m_Editor->GetEditorCameraTransform();

            app.GetSceneRenderer()->SetOverrideCamera(camera, transform);
            app.GetSceneRenderer()->m_EnableUIPass = false;
        }

        ImVec2 offset = { 0.0f, 0.0f };

        if(!camera || !transform)
        {
            ImGui::End();
            return;
        }

        ImGuizmo::SetDrawlist();
        auto sceneViewSize     = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() - offset * 0.5f; // - offset * 0.5f;
        auto sceneViewPosition = ImGui::GetWindowPos() + offset;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        float aspect = static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y);

        if(!Maths::Equals(aspect, camera->GetAspectRatio()))
        {
            camera->SetAspectRatio(aspect);
        }
        // m_Editor->m_SceneViewPosition = Vec2(sceneViewPosition.x, sceneViewPosition.y);

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        Resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

        const ImVec2 imageOrigin = ImGui::GetCursorScreenPos();
        ImGuiUtilities::Image(m_GameViewTexture.get(), Vec2(sceneViewSize.x, sceneViewSize.y), Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture());

        m_SceneViewPos = imageOrigin;
        ToolBar();

        // ---- Camera HUD overlay (top-right of viewport) ----
        if(m_ShowCameraHUD)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImFont* smallF = (ImGui::GetIO().Fonts->Fonts.Size > 2) ? ImGui::GetIO().Fonts->Fonts[2] : ImGui::GetFont();
            const float fs = smallF->FontSize;
            const Vec3 pos    = transform->GetWorldPosition();
            const Vec3 euler  = transform->GetLocalOrientation().ToEuler();
            const bool ortho  = camera->IsOrthographic();
            const float speed = m_Editor->GetEditorCameraController().GetSpeed();
            char line0[64], line1[80], line2[80], line3[80];
            snprintf(line0, sizeof(line0), "cam %s  %.0f°", ortho ? "orthographic" : "perspective", camera->GetFOV());
            snprintf(line1, sizeof(line1), "pos x %.2f  y %.2f  z %.2f", pos.x, pos.y, pos.z);
            snprintf(line2, sizeof(line2), "rot %.1f°  %.1f°  %.1f°", Maths::ToDegrees(euler.x), Maths::ToDegrees(euler.y), Maths::ToDegrees(euler.z));
            snprintf(line3, sizeof(line3), "speed %.0f", speed);

            const float padX = 10.0f, padY = 8.0f, lineGap = 2.0f;
            const ImU32 dim  = IM_COL32(170, 165, 155, 255);
            const ImU32 fg   = IM_COL32(232, 226, 216, 255);
            const ImU32 bg   = IM_COL32(15, 14, 12, 200);

            float wMax = 0;
            for(const char* s : { line0, line1, line2, line3 })
            {
                ImVec2 sz = smallF->CalcTextSizeA(fs, FLT_MAX, 0, s);
                if(sz.x > wMax) wMax = sz.x;
            }
            const float boxW = wMax + padX * 2;
            const float boxH = (fs * 4) + (lineGap * 3) + padY * 2;
            ImVec2 boxMin = { imageOrigin.x + sceneViewSize.x - boxW - 16.0f, imageOrigin.y + 16.0f };
            ImVec2 boxMax = { boxMin.x + boxW, boxMin.y + boxH };
            dl->AddRectFilled(boxMin, boxMax, bg, 4.0f);

            ImVec2 tp = { boxMin.x + padX, boxMin.y + padY };
            dl->AddText(smallF, fs, tp, dim, line0);
            tp.y += fs + lineGap;
            dl->AddText(smallF, fs, tp, fg, line1);
            tp.y += fs + lineGap;
            dl->AddText(smallF, fs, tp, fg, line2);
            tp.y += fs + lineGap;
            dl->AddText(smallF, fs, tp, fg, line3);
        }

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = sceneViewPosition;

        ImVec2 maxBound   = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        bool updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound) || m_bRequestCameraUpdate; // || Input::Get().GetMouseMode() == MouseMode::Captured;

        // app.SetSceneActive(true);// ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

        ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);

        m_Editor->SetSceneViewActive(updateCamera && !Application::Get().GetSceneActive());
        {
            LUMOS_PROFILE_SCOPE("Push Clip Rect");
            ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, { sceneViewSize.x + sceneViewPosition.x, sceneViewSize.y + sceneViewPosition.y - 2.0f });
        }

        if(m_Editor->ShowGrid())
        {
            if(camera->IsOrthographic())
            {
                LUMOS_PROFILE_SCOPE("2D Grid");
                m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { transform->GetWorldPosition().x, transform->GetWorldPosition().y }, sceneViewPosition, { sceneViewSize.x, sceneViewSize.y }, camera->GetScale(), 1.5f);
            }
        }

        m_Editor->OnImGuizmo();

        // Terrain sculpt tool — intercepts LMB drag inside the viewport, runs the
        // brush each frame against the picked terrain. Bypasses normal selection
        // while active.
        TerrainEditorPanel* terrainPanel = m_Editor ? m_Editor->GetTerrainEditorPanel() : nullptr;
        const bool terrainToolActive = terrainPanel && terrainPanel->IsActive();

        // Voxel edit mode: 'B' toggles. Left-click removes a block, Shift+Left places
        // one against the hit face. Active only when the scene has a voxel world.
        static bool s_VoxelEditMode = false;
        if(ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_B, false) && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
            s_VoxelEditMode = !s_VoxelEditMode;

        Graphics::VoxelWorld* voxelWorld = nullptr;
        if(m_CurrentScene)
        {
            auto& reg = m_CurrentScene->GetRegistry();
            auto vv   = reg.view<VoxelWorldComponent>();
            if(vv.begin() != vv.end())
                voxelWorld = reg.get<VoxelWorldComponent>(vv.front()).World.get();
        }
        const bool voxelEditActive = s_VoxelEditMode && voxelWorld;

        if(voxelEditActive && ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing()
           && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
        {
            float dpi      = Application::Get().GetWindowDPI();
            auto clickPos  = Input::Get().GetMousePosition() - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);
            Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));

            if(ImGui::GetIO().KeyShift)
                voxelWorld->PlaceBlock(ray.Origin, ray.Direction, 256.0f, Graphics::Block_Solid);
            else
                voxelWorld->RemoveBlock(ray.Origin, ray.Direction, 256.0f);
        }

        if(terrainToolActive && ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing())
        {
            LUMOS_PROFILE_SCOPE("Terrain Sculpt");
            float dpi     = Application::Get().GetWindowDPI();
            auto cursor   = Input::Get().GetMousePosition() - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);
            Maths::Ray ray = m_Editor->GetScreenRay(int(cursor.x), int(cursor.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));

            entt::entity hitEntity = entt::null;
            Vec3 hitWorld;
            bool gotHit = terrainPanel->RaycastTerrain(ray, hitEntity, hitWorld);

            // Brush preview overlay (contour-following ring).
            if(gotHit)
            {
                constexpr int kRing = 24;
                Vec3 ring[kRing];
                terrainPanel->SampleBrushRing(hitWorld, kRing, ring);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 first(0, 0), prev(0, 0);
                Mat4 viewProj = camera->GetProjectionMatrix() * transform->GetWorldMatrix().Inverse();
                auto Project = [&](const Vec3& p) -> ImVec2
                {
                    Vec2 sp = Maths::WorldToScreen(p, viewProj, sceneViewSize.x, sceneViewSize.y, sceneViewPosition.x, sceneViewPosition.y);
                    return ImVec2(sp.x, sp.y);
                };
                ImU32 colour = IM_COL32(60, 220, 110, 220);
                switch(terrainPanel->CurrentTool())
                {
                    case TerrainTool::Lower:   colour = IM_COL32(230,  90,  60, 220); break;
                    case TerrainTool::Smooth:  colour = IM_COL32( 90, 160, 230, 220); break;
                    case TerrainTool::Flatten: colour = IM_COL32(230, 200,  60, 220); break;
                    case TerrainTool::Noise:   colour = IM_COL32(200,  90, 230, 220); break;
                    default: break;
                }
                for(int i = 0; i < kRing; ++i)
                {
                    ImVec2 sp = Project(ring[i]);
                    if(i == 0) first = sp;
                    else       dl->AddLine(prev, sp, colour, 1.5f);
                    prev = sp;
                }
                dl->AddLine(prev, first, colour, 1.5f);
                ImVec2 centreSp = Project(hitWorld);
                dl->AddCircleFilled(centreSp, 3.0f, colour);
            }

            // Drag-to-paint. ButtonLeft press starts a stroke, hold continues, release ends.
            const bool lmbPressed = Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft);
            const bool lmbHeld    = Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft);

            if(lmbPressed && gotHit)
                terrainPanel->BeginStroke();
            if(lmbHeld && gotHit)
                terrainPanel->ApplyBrushAt(hitWorld, hitEntity, (float)Engine::GetTimeStep().GetSeconds());
            if(!lmbHeld)
                terrainPanel->EndStroke();

            // Keyboard hotkeys (only while viewport focused).
            if(ImGui::IsKeyPressed(ImGuiKey_G, false))
                terrainPanel->SetCurrentTool(ImGui::GetIO().KeyShift ? TerrainTool::Lower : TerrainTool::Raise);
            else if(ImGui::IsKeyPressed(ImGuiKey_S, false) && !ImGui::GetIO().KeyCtrl)
                terrainPanel->SetCurrentTool(TerrainTool::Smooth);
            else if(ImGui::IsKeyPressed(ImGuiKey_F, false))
                terrainPanel->SetCurrentTool(TerrainTool::Flatten);
            else if(ImGui::IsKeyPressed(ImGuiKey_N, false) && !ImGui::GetIO().KeyCtrl)
                terrainPanel->SetCurrentTool(TerrainTool::Noise);
            if(ImGui::IsKeyPressed(ImGuiKey_LeftBracket, true))
            {
                float& v = ImGui::GetIO().KeyShift ? terrainPanel->BrushStrength() : terrainPanel->BrushRadius();
                v = std::max(0.05f, v * 0.85f);
            }
            if(ImGui::IsKeyPressed(ImGuiKey_RightBracket, true))
            {
                float& v = ImGui::GetIO().KeyShift ? terrainPanel->BrushStrength() : terrainPanel->BrushRadius();
                v *= 1.15f;
            }
            // ESC / RMB exits terrain mode.
            if(ImGui::IsKeyPressed(ImGuiKey_Escape, false))
                terrainPanel->SetEnabled(false);
        }

        // T toggles terrain tool mode regardless of current state — opens the panel
        // and switches input intercept on.
        if(ImGui::IsWindowFocused() && terrainPanel && ImGui::IsKeyPressed(ImGuiKey_T, false) && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
        {
            terrainPanel->SetEnabled(!terrainPanel->IsActive());
        }

        if(!terrainToolActive && !voxelEditActive && ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing() && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
        {
            float dpi     = Application::Get().GetWindowDPI();
            auto clickPos = Input::Get().GetMousePosition() - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

            Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));

            // Handle measurement mode click
            if(m_MeasurementMode)
            {
                LUMOS_PROFILE_SCOPE("Measurement Click");
                Vec3 clickWorldPos;

                if(m_MeasurementPointIndex >= 2)
                {
                    m_MeasurementPointIndex = 0;
                }

                if(camera->IsOrthographic())
                {
                    clickWorldPos   = ray.Origin;
                    clickWorldPos.z = 0.0f;
                }
                else
                {
                    // For 3D mode, place at a fixed distance or use entity under cursor
                    float distance  = 10.0f;
                    clickWorldPos   = ray.Origin + ray.Direction * distance;
                }

                if(m_MeasurementPointIndex == 0)
                {
                    m_MeasurementPoint1 = clickWorldPos;
                    m_MeasurementPointIndex = 1;
                }
                else if(m_MeasurementPointIndex == 1)
                {
                    m_MeasurementPoint2 = clickWorldPos;
                    m_MeasurementPointIndex = 2;
                }
            }
            else
            {
                LUMOS_PROFILE_SCOPE("Select Object");
                m_Editor->SelectObject(ray);
            }
        }

        if(!terrainToolActive && !voxelEditActive && ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing() && ImGui::IsItemHovered() && Input::Get().GetMouseMode() == MouseMode::Visible)
        {
            LUMOS_PROFILE_SCOPE("Hover Object");

            float dpi     = Application::Get().GetWindowDPI();
            auto clickPos = Input::Get().GetMousePosition() - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

            Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));
            m_Editor->SelectObject(ray, true);
        }
        else
            m_Editor->SetHoveredEntity({});

        if(m_Editor->GetHoveredEntity())
        {
            ImGuiUtilities::ScopedStyle scopedStyle(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(m_Editor->GetHoveredEntity().GetName().c_str());
            ImGui::EndTooltip();
        }

        // Right-click context menu (only on release without drag)
        if(ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing())
        {
            if(Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonRight))
            {
                m_RightMousePressed  = true;
                m_RightMouseDragged  = false;
                m_RightMousePressPos = Input::Get().GetMousePosition();
            }

            if(m_RightMousePressed && Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
            {
                Vec2 delta = Input::Get().GetMousePosition() - m_RightMousePressPos;
                if(delta.Length() > 3.0f)
                    m_RightMouseDragged = true;
            }

            if(m_RightMousePressed && !Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
            {
                if(!m_RightMouseDragged)
                {
                    float dpi     = Application::Get().GetWindowDPI();
                    auto clickPos = m_RightMousePressPos - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

                    Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));

                    if(camera->IsOrthographic())
                    {
                        m_ContextMenuWorldPos = ray.Origin;
                        m_ContextMenuWorldPos.z = 0.0f;
                    }
                    else
                    {
                        float distance = 10.0f;
                        m_ContextMenuWorldPos = ray.Origin + ray.Direction * distance;
                    }

                    m_ContextMenuPending = true;
                    ImGui::OpenPopup("SceneViewContextMenu");
                }
                m_RightMousePressed = false;
            }
        }

        // Handle pinch gesture for zoom (less restrictive for touch - gestures are 2+ fingers so won't conflict with UI)
        if(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && Input::Get().GetGesturePinchActive())
        {
            float scale = Input::Get().GetGesturePinchScale();
            float velocity = Input::Get().GetGesturePinchVelocity();
            m_Editor->GetEditorCameraController().HandleGesturePinch(
                m_Editor->GetEditorCameraTransform(), scale, velocity, (float)Engine::Get().GetTimeStep().GetSeconds());
        }

        // Touch pan: 2-finger = rotate, 3-finger = pan focal point
        if(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && Input::Get().GetGesturePanActive())
        {
            uint32_t touchCount = Input::Get().GetGesturePanTouchCount();
            if(touchCount >= 2)
            {
                Vec2 translation = Input::Get().GetGesturePanTranslation();
                Vec2 velocity = Input::Get().GetGesturePanVelocity();
                m_Editor->GetEditorCameraController().HandleGesturePan(
                    m_Editor->GetEditorCameraTransform(), translation, velocity, touchCount);
            }
        }

        // Handle long-press gesture for context menu
        if(ImGui::IsWindowFocused() && updateCamera && !ImGuizmo::IsUsing() && !ImGui::GetIO().WantCaptureMouse && Input::Get().GetGestureLongPressActive())
        {
            Vec2 pressLocation = Input::Get().GetGestureLongPressLocation();
            float dpi = Application::Get().GetWindowDPI();
            auto clickPos = pressLocation - Vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

            if(clickPos.x >= 0 && clickPos.y >= 0 && clickPos.x < sceneViewSize.x && clickPos.y < sceneViewSize.y)
            {
                Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));

                // Calculate world position based on camera mode
                if(camera->IsOrthographic())
                {
                    m_ContextMenuWorldPos = ray.Origin;
                    m_ContextMenuWorldPos.z = 0.0f;
                }
                else
                {
                    float distance = 10.0f;
                    m_ContextMenuWorldPos = ray.Origin + ray.Direction * distance;
                }

                m_ContextMenuPending = true;
                ImGui::OpenPopup("SceneViewContextMenu");
            }
        }

        DrawContextMenu(m_CurrentScene);

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if(ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("AssetFile", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
            if(data)
            {
                std::string file = (char*)data->Data;
                m_Editor->FileOpenCallback(file);
            }
            ImGui::EndDragDropTarget();
        }

        if(app.GetSceneManager()->GetCurrentScene())
        {
            DrawGizmos(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y, app.GetSceneManager()->GetCurrentScene());
            DrawMeasurementTool(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y, app.GetSceneManager()->GetCurrentScene());
        }

        ImGui::GetWindowDrawList()->PopClipRect();
        ImGui::End();

        // One mesh + collision rebuild per affected terrain per frame.
        if(terrainPanel)
            terrainPanel->FlushDirty();

        m_bRequestCameraUpdate = false;
    }

    void SceneViewPanel::DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        Camera* camera                    = m_Editor->GetCamera();
        Maths::Transform& cameraTransform = m_Editor->GetEditorCameraTransform();
        auto& registry                    = scene->GetRegistry();
        Mat4 view                         = cameraTransform.GetWorldMatrix().Inverse();
        Mat4 proj                         = camera->GetProjectionMatrix();
        Mat4 viewProj                     = proj * view;
        const Maths::Frustum& f           = camera->GetFrustum(view);

        ShowComponentGizmo<Graphics::Light>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<Camera>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<SoundComponent>(width, height, xpos, ypos, viewProj, f, registry);
    }

    void SceneViewPanel::DrawMeasurementTool(float width, float height, float xpos, float ypos, Scene* scene)
    {
        if(!m_MeasurementMode && m_MeasurementPointIndex == 0)
            return;

        Camera* camera                    = m_Editor->GetCamera();
        Maths::Transform& cameraTransform = m_Editor->GetEditorCameraTransform();
        Mat4 view                         = cameraTransform.GetWorldMatrix().Inverse();
        Mat4 proj                         = camera->GetProjectionMatrix();
        Mat4 viewProj                     = proj * view;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 lineColor = IM_COL32(255, 200, 50, 255);
        const ImU32 pointColor = IM_COL32(50, 255, 100, 255);
        const ImU32 textBgColor = IM_COL32(0, 0, 0, 180);
        const float lineThickness = 2.0f;
        const float pointRadius = 6.0f;

        // Draw first point if selected
        if(m_MeasurementPointIndex >= 1)
        {
            Vec2 screenPos1 = Maths::WorldToScreen(m_MeasurementPoint1, viewProj, width, height, xpos, ypos);
            drawList->AddCircleFilled(ImVec2(screenPos1.x, screenPos1.y), pointRadius, pointColor);
            drawList->AddCircle(ImVec2(screenPos1.x, screenPos1.y), pointRadius + 1, IM_COL32(255, 255, 255, 200), 0, 1.5f);

            // Draw label "P1"
            drawList->AddText(ImVec2(screenPos1.x + 10, screenPos1.y - 8), IM_COL32(255, 255, 255, 255), "P1");
        }

        // Draw second point and line if both selected
        if(m_MeasurementPointIndex >= 2)
        {
            Vec2 screenPos1 = Maths::WorldToScreen(m_MeasurementPoint1, viewProj, width, height, xpos, ypos);
            Vec2 screenPos2 = Maths::WorldToScreen(m_MeasurementPoint2, viewProj, width, height, xpos, ypos);

            // Draw line
            drawList->AddLine(ImVec2(screenPos1.x, screenPos1.y), ImVec2(screenPos2.x, screenPos2.y), lineColor, lineThickness);

            // Draw second point
            drawList->AddCircleFilled(ImVec2(screenPos2.x, screenPos2.y), pointRadius, pointColor);
            drawList->AddCircle(ImVec2(screenPos2.x, screenPos2.y), pointRadius + 1, IM_COL32(255, 255, 255, 200), 0, 1.5f);

            // Draw label "P2"
            drawList->AddText(ImVec2(screenPos2.x + 10, screenPos2.y - 8), IM_COL32(255, 255, 255, 255), "P2");

            // Calculate and display distance at midpoint
            float distance = Maths::Distance(m_MeasurementPoint1, m_MeasurementPoint2);
            Vec2 midScreen = (screenPos1 + screenPos2) * 0.5f;

            char distText[64];
            snprintf(distText, sizeof(distText), "%.3f", distance);

            ImVec2 textSize = ImGui::CalcTextSize(distText);
            ImVec2 padding = ImVec2(4, 2);

            // Draw background box for text
            drawList->AddRectFilled(
                ImVec2(midScreen.x - textSize.x / 2 - padding.x, midScreen.y - 12 - textSize.y - padding.y),
                ImVec2(midScreen.x + textSize.x / 2 + padding.x, midScreen.y - 12 + padding.y),
                textBgColor, 3.0f);

            // Draw distance text
            drawList->AddText(
                ImVec2(midScreen.x - textSize.x / 2, midScreen.y - 12 - textSize.y),
                IM_COL32(255, 200, 50, 255), distText);
        }
    }

    void SceneViewPanel::ToolBar()
    {
        LUMOS_PROFILE_FUNCTION();

        // Anchored, vertical, collapsable tool column at top-left of the scene viewport.
        ImGuiWindowFlags wflags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking
                                | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoTitleBar
                                | ImGuiWindowFlags_NoNavFocus;

        // Target a fixed visual button size so 10+ icons fit in the viewport regardless of
        // how large the editor font is on iOS / high-DPI desktops. The icon glyph (rendered
        // from the text font) is shrunk to fit via SetWindowFontScale below.
#ifdef LUMOS_PLATFORM_IOS
        const float btnW      = 52.0f;   // touch-friendly
        const float padX      = 10.0f;
        const float padY      = 6.0f;
        const float anchorOff = 10.0f;
        const float winRound  = 10.0f;
#else
        const float btnW      = 36.0f;
        const float padX      = 10.0f;
        const float padY      = 6.0f;
        const float anchorOff = 10.0f;
        const float winRound  = 8.0f;
#endif
        const float winW = btnW + padX * 2.0f;

        ImGui::SetNextWindowPos(ImVec2(m_SceneViewPos.x + anchorOff, m_SceneViewPos.y + anchorOff), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(winW, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(winW, 0.0f), ImVec2(winW, FLT_MAX));
        ImGui::SetNextWindowBgAlpha(0.45f);
        ImGuiUtilities::ScopedStyle minSize(ImGuiStyleVar_WindowMinSize, ImVec2(8, 8));
        ImGuiUtilities::ScopedStyle round(ImGuiStyleVar_WindowRounding, winRound);
        ImGuiUtilities::ScopedStyle padScope(ImGuiStyleVar_WindowPadding, ImVec2(padX, padY));
        ImGuiUtilities::ScopedStyle itemScope(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        ImGuiUtilities::ScopedStyle framePad(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        if(!ImGui::Begin("Tools##scene_tools", nullptr, wflags))
        {
            ImGui::End();
            return;
        }

        // Scale the icon font so the glyph fits inside the fixed-size button regardless
        // of how large the editor font is (iOS fonts can be 80–140 px otherwise).
        const float kIconCellFrac = 0.62f;
        const float curFont       = ImGui::GetFontSize();
        if(curFont > btnW * kIconCellFrac)
            ImGui::SetWindowFontScale(btnW * kIconCellFrac / curFont);

        // Collapse toggle (full-width chevron header).
        static bool collapsed = false;
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if(ImGui::Button(collapsed ? ICON_MDI_CHEVRON_DOWN : ICON_MDI_CHEVRON_UP, ImVec2(btnW, btnW)))
                collapsed = !collapsed;
            ImGui::PopStyleColor();
        }
        if(collapsed)
        {
            ImGui::End();
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        const ImVec2 btnSize(btnW, btnW);
        const auto IconBtn = [&](const char* icon, const char* tip, bool active, auto&& action)
        {
            if(active) ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());
            if(ImGui::Button(icon, btnSize)) action();
            if(active) ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip(tip);
        };

        IconBtn(ICON_MDI_CURSOR_DEFAULT, "Select",   m_Editor->GetImGuizmoOperation() == 4,
                [&]{ m_Editor->SetImGuizmoOperation(4); });
        ImGui::Separator();
        IconBtn(ICON_MDI_ARROW_ALL,         "Translate", m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE,
                [&]{ m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE); });
        IconBtn(ICON_MDI_ROTATE_ORBIT,      "Rotate",    m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE,
                [&]{ m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE); });
        IconBtn(ICON_MDI_ARROW_EXPAND_ALL,  "Scale",     m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE,
                [&]{ m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE); });
        ImGui::Separator();
        IconBtn(ICON_MDI_CROP_ROTATE, "Universal", m_Editor->GetImGuizmoOperation() == ImGuizmo::UNIVERSAL,
                [&]{ m_Editor->SetImGuizmoOperation(ImGuizmo::UNIVERSAL); });
        IconBtn(ICON_MDI_BORDER_NONE, "Bounds",    m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS,
                [&]{ m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS); });
        ImGui::Separator();
        IconBtn(ICON_MDI_MAGNET, "Snap", m_Editor->SnapGuizmo(),
                [&]{ m_Editor->SnapGuizmo() = !m_Editor->SnapGuizmo(); });
        ImGui::Separator();
        {
            TerrainEditorPanel* tep = m_Editor->GetTerrainEditorPanel();
            const bool active = tep && tep->IsActive();
            IconBtn(ICON_MDI_TERRAIN, "Terrain Sculpt (T)", active, [&]{
                if(tep) tep->SetEnabled(!tep->IsActive());
            });
        }
        ImGui::Separator();

        if(ImGui::Button(ICON_MDI_DOTS_HORIZONTAL, btnSize))
            ImGui::OpenPopup("GizmosPopup");
        ImGuiUtilities::Tooltip("Gizmos & Debug");
        if(ImGui::BeginPopup("GizmosPopup"))
        {
            {
                ImGui::Checkbox("Grid", &m_Editor->ShowGrid());
                ImGui::Checkbox("Selected Gizmos", &m_Editor->ShowGizmos());
                ImGui::Checkbox("View Selected", &m_Editor->ShowViewSelected());

                ImGui::Separator();
                ImGui::Checkbox("Camera", &m_ShowComponentGizmoMap[typeid(Camera).hash_code()]);
                ImGui::Checkbox("Light", &m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()]);
                ImGui::Checkbox("Audio", &m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]);

                ImGui::Separator();

                uint32_t flags       = m_Editor->GetSettings().m_DebugDrawFlags;
                bool showEntityNames = flags & EditorDebugFlags::EntityNames;
                if(ImGui::Checkbox("Entity Names", &showEntityNames))
                {
                    if(showEntityNames)
                        flags += EditorDebugFlags::EntityNames;
                    else
                        flags -= EditorDebugFlags::EntityNames;
                }

                bool showAABB = flags & EditorDebugFlags::MeshBoundingBoxes;
                if(ImGui::Checkbox("Mesh AABB", &showAABB))
                {
                    if(showAABB)
                        flags += EditorDebugFlags::MeshBoundingBoxes;
                    else
                        flags -= EditorDebugFlags::MeshBoundingBoxes;
                }

                bool showSpriteBox = flags & EditorDebugFlags::SpriteBoxes;
                if(ImGui::Checkbox("Sprite Box", &showSpriteBox))
                {
                    if(showSpriteBox)
                        flags += EditorDebugFlags::SpriteBoxes;
                    else
                        flags -= EditorDebugFlags::SpriteBoxes;
                }

                bool showCameraFrustums = flags & EditorDebugFlags::CameraFrustum;
                if(ImGui::Checkbox("Camera Frustums", &showCameraFrustums))
                {
                    if(showCameraFrustums)
                        flags += EditorDebugFlags::CameraFrustum;
                    else
                        flags -= EditorDebugFlags::CameraFrustum;
                }

                m_Editor->GetSettings().m_DebugDrawFlags = flags;
                ImGui::Separator();

                auto physics2D = Application::Get().GetSystem<B2PhysicsEngine>();

                if(physics2D)
                {
                    uint32_t flags = physics2D->GetDebugDrawFlags();

                    bool show2DShapes = flags & PhysicsDebugFlags2D::LINEARFORCE2D;
                    if(ImGui::Checkbox("Shapes (2D)", &show2DShapes))
                    {
                        if(show2DShapes)
                            flags += PhysicsDebugFlags2D::LINEARFORCE2D;
                        else
                            flags -= PhysicsDebugFlags2D::LINEARFORCE2D;
                    }

                    bool showCOG = flags & PhysicsDebugFlags2D::COLLISIONVOLUMES2D;
                    if(ImGui::Checkbox("Centre of Mass (2D)", &showCOG))
                    {
                        if(showCOG)
                            flags += PhysicsDebugFlags2D::COLLISIONVOLUMES2D;
                        else
                            flags -= PhysicsDebugFlags2D::COLLISIONVOLUMES2D;
                    }

                    bool showJoint = flags & PhysicsDebugFlags2D::CONSTRAINT2D;
                    if(ImGui::Checkbox("Joint Connection (2D)", &showJoint))
                    {
                        if(showJoint)
                            flags += PhysicsDebugFlags2D::CONSTRAINT2D;
                        else
                            flags -= PhysicsDebugFlags2D::CONSTRAINT2D;
                    }

                    bool showAABB = flags & PhysicsDebugFlags2D::AABB2D;
                    if(ImGui::Checkbox("AABB (2D)", &showAABB))
                    {
                        if(showAABB)
                            flags += PhysicsDebugFlags2D::AABB2D;
                        else
                            flags -= PhysicsDebugFlags2D::AABB2D;
                    }

                    physics2D->SetDebugDrawFlags(flags);
                }

                auto physics3D = Application::Get().GetSystem<LumosPhysicsEngine>();

                if(physics3D)
                {
                    uint32_t flags = physics3D->GetDebugDrawFlags();

                    bool showCollisionShapes = flags & PhysicsDebugFlags::COLLISIONVOLUMES;
                    if(ImGui::Checkbox("Collision Volumes", &showCollisionShapes))
                    {
                        if(showCollisionShapes)
                            flags += PhysicsDebugFlags::COLLISIONVOLUMES;
                        else
                            flags -= PhysicsDebugFlags::COLLISIONVOLUMES;
                    }

                    bool showConstraints = static_cast<bool>(flags & PhysicsDebugFlags::CONSTRAINT);
                    if(ImGui::Checkbox("Constraints", &showConstraints))
                    {
                        if(showConstraints)
                            flags += PhysicsDebugFlags::CONSTRAINT;
                        else
                            flags -= PhysicsDebugFlags::CONSTRAINT;
                    }

                    bool showManifolds = static_cast<bool>(flags & PhysicsDebugFlags::MANIFOLD);
                    if(ImGui::Checkbox("Manifolds", &showManifolds))
                    {
                        if(showManifolds)
                            flags += PhysicsDebugFlags::MANIFOLD;
                        else
                            flags -= PhysicsDebugFlags::MANIFOLD;
                    }

                    bool showCollisionNormals = flags & PhysicsDebugFlags::COLLISIONNORMALS;
                    if(ImGui::Checkbox("Collision Normals", &showCollisionNormals))
                    {
                        if(showCollisionNormals)
                            flags += PhysicsDebugFlags::COLLISIONNORMALS;
                        else
                            flags -= PhysicsDebugFlags::COLLISIONNORMALS;
                    }

                    bool showAABB = flags & PhysicsDebugFlags::AABB;
                    if(ImGui::Checkbox("AABB", &showAABB))
                    {
                        if(showAABB)
                            flags += PhysicsDebugFlags::AABB;
                        else
                            flags -= PhysicsDebugFlags::AABB;
                    }

                    bool showLinearVelocity = flags & PhysicsDebugFlags::LINEARVELOCITY;
                    if(ImGui::Checkbox("Linear Velocity", &showLinearVelocity))
                    {
                        if(showLinearVelocity)
                            flags += PhysicsDebugFlags::LINEARVELOCITY;
                        else
                            flags -= PhysicsDebugFlags::LINEARVELOCITY;
                    }

                    bool LINEARFORCE = flags & PhysicsDebugFlags::LINEARFORCE;
                    if(ImGui::Checkbox("Linear Force", &LINEARFORCE))
                    {
                        if(LINEARFORCE)
                            flags += PhysicsDebugFlags::LINEARFORCE;
                        else
                            flags -= PhysicsDebugFlags::LINEARFORCE;
                    }

                    bool BROADPHASE = flags & PhysicsDebugFlags::BROADPHASE;
                    if(ImGui::Checkbox("Broadphase", &BROADPHASE))
                    {
                        if(BROADPHASE)
                            flags += PhysicsDebugFlags::BROADPHASE;
                        else
                            flags -= PhysicsDebugFlags::BROADPHASE;
                    }

                    bool showPairs = flags & PhysicsDebugFlags::BROADPHASE_PAIRS;
                    if(ImGui::Checkbox("Broadphase Pairs", &showPairs))
                    {
                        if(showPairs)
                            flags += PhysicsDebugFlags::BROADPHASE_PAIRS;
                        else
                            flags -= PhysicsDebugFlags::BROADPHASE_PAIRS;
                    }

                    physics3D->SetDebugDrawFlags(flags);
                }

                ImGui::EndPopup();
            }
        }

        ImGui::Separator();

        // Camera ortho/perspective toggle.
        auto& camera = *m_Editor->GetCamera();
        bool ortho   = camera.IsOrthographic();
        IconBtn(ICON_MDI_AXIS_ARROW, "3D camera (perspective)", !ortho, [&]
        {
            if(ortho)
            {
                camera.SetIsOrthographic(false);
                camera.SetNear(0.01f);
                m_Editor->GetEditorCameraController().SetCurrentMode(EditorCameraMode::FLYCAM);
            }
        });
        IconBtn(ICON_MDI_ANGLE_RIGHT, "2D camera (orthographic)", ortho, [&]
        {
            if(!ortho)
            {
                camera.SetIsOrthographic(true);
                auto camPos = m_Editor->GetEditorCameraTransform().GetLocalPosition();
                camPos.z    = 0.0f;
                camera.SetNear(-10.0f);
                m_Editor->GetEditorCameraTransform().SetLocalPosition(camPos);
                m_Editor->GetEditorCameraTransform().SetLocalOrientation(Quat(Vec3(0.0f, 0.0f, 0.0f)));
                m_Editor->GetEditorCameraTransform().SetLocalScale(Vec3(1.0f, 1.0f, 1.0f));
                m_Editor->GetEditorCameraController().SetCurrentMode(EditorCameraMode::TWODIM);
            }
        });
        ImGui::Separator();

        IconBtn(ICON_MDI_RULER, "Measure distance (click two points)", m_MeasurementMode, [&]
        {
            m_MeasurementMode = !m_MeasurementMode;
            if(m_MeasurementMode) m_MeasurementPointIndex = 0;
        });
        ImGui::Separator();
        IconBtn(ICON_MDI_CAMERA_OUTLINE, "Toggle camera HUD", m_ShowCameraHUD, [&]
        {
            m_ShowCameraHUD = !m_ShowCameraHUD;
        });

        ImGui::PopStyleColor();
        ImGui::End();

        // Measurement status overlay — separate floating element below the tools column.
        if(m_MeasurementMode)
        {
            // Sit just to the right of the tools column, using the same font-scaled metrics.
            const float measureX = m_SceneViewPos.x + anchorOff + winW + anchorOff;
            const float measureY = m_SceneViewPos.y + anchorOff;
            ImGui::SetNextWindowPos(ImVec2(measureX, measureY), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.80f);
            if(ImGui::Begin("Measure##scene_tools_measure", nullptr, wflags | ImGuiWindowFlags_NoTitleBar))
            {
                if(m_MeasurementPointIndex == 0)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Click first point");
                else if(m_MeasurementPointIndex == 1)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Click second point");
                else
                {
                    float distance = Maths::Distance(m_MeasurementPoint1, m_MeasurementPoint2);
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Distance: %.3f units", distance);
                    ImGui::SameLine();
                    if(ImGui::SmallButton("Clear"))
                        m_MeasurementPointIndex = 0;
                }
            }
            ImGui::End();
        }
    }

    void SceneViewPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_Editor->GetSettings().m_AspectRatio = 1.0f;
        m_CurrentScene                        = scene;

        auto SceneRenderer = Application::Get().GetSceneRenderer();
        SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true);
        SceneRenderer->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
        m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), true);
        m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);
    }

    void SceneViewPanel::Resize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        bool resize = false;

        ASSERT(width > 0 && height > 0, "Scene View Dimensions 0");

        if(m_Width != width || m_Height != height)
        {
            resize   = true;
            m_Width  = width;
            m_Height = height;
        }

        if(!m_GameViewTexture)
        {
            Graphics::TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format = Graphics::RHIFormat::R8G8B8A8_Unorm;
            mainRenderTargetDesc.flags  = Graphics::TextureFlags::Texture_RenderTarget;

            m_GameViewTexture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create(mainRenderTargetDesc, m_Width, m_Height));
        }

        if(resize)
        {
            m_GameViewTexture->Resize(m_Width, m_Height);

            auto SceneRenderer = Application::Get().GetSceneRenderer();
            SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true, false);

            if(!m_Editor->GetGridRenderer())
                m_Editor->CreateGridRenderer();
            m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);
            m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);

            WindowResizeEvent e(width, height);
            auto& app = Application::Get();
            app.GetSceneRenderer()->OnResize(width, height);

            SceneRenderer->OnEvent(e);

            m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

            // Should be just build texture and scene renderer set render target
            // Renderer::GetGraphicsContext()->WaitIdle();
        }
    }

    void SceneViewPanel::DrawContextMenu(Scene* scene)
    {
        if(!scene)
            return;

        auto editor = m_Editor;
        Vec3 pos    = m_ContextMenuWorldPos;

        auto CreateEntityAtPosition = [scene, editor, pos](const std::string& name) -> Entity {
            auto entity = scene->CreateEntity(name);
            entity.GetOrAddComponent<Maths::Transform>().SetLocalPosition(pos);
            editor->ClearSelected();
            editor->SetSelected(entity);
            return entity;
        };

        if(ImGui::BeginPopup("SceneViewContextMenu"))
        {
            if(ImGui::MenuItem(ICON_MDI_CUBE_OUTLINE " Empty Entity"))
            {
                CreateEntityAtPosition("Entity");
            }

            ImGui::Separator();

            if(ImGui::BeginMenu(ICON_MDI_SHAPE " 3D Object"))
            {
                if(ImGui::MenuItem("Cube"))
                {
                    auto entity = CreateEntityAtPosition("Cube");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);
                }
                if(ImGui::MenuItem("Sphere"))
                {
                    auto entity = CreateEntityAtPosition("Sphere");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
                }
                if(ImGui::MenuItem("Plane"))
                {
                    auto entity = CreateEntityAtPosition("Plane");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Plane);
                }
                if(ImGui::MenuItem("Cylinder"))
                {
                    auto entity = CreateEntityAtPosition("Cylinder");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cylinder);
                }
                if(ImGui::MenuItem("Capsule"))
                {
                    auto entity = CreateEntityAtPosition("Capsule");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Capsule);
                }
                if(ImGui::MenuItem("Pyramid"))
                {
                    auto entity = CreateEntityAtPosition("Pyramid");
                    entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Pyramid);
                }
                if(ImGui::MenuItem("Voxel World"))
                {
                    auto entity = CreateEntityAtPosition("Voxel World");
                    entity.AddComponent<VoxelWorldComponent>();
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu(ICON_MDI_IMAGE " 2D Object"))
            {
                if(ImGui::MenuItem("Sprite"))
                {
                    auto entity = CreateEntityAtPosition("Sprite");
                    entity.AddComponent<Graphics::Sprite>();
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu(ICON_MDI_LIGHTBULB " Light"))
            {
                if(ImGui::MenuItem("Point Light"))
                {
                    auto entity = CreateEntityAtPosition("Point Light");
                    auto& light = entity.AddComponent<Graphics::Light>();
                    light.Type  = (float)Graphics::LightType::PointLight;
                }
                if(ImGui::MenuItem("Directional Light"))
                {
                    auto entity = CreateEntityAtPosition("Directional Light");
                    auto& light = entity.AddComponent<Graphics::Light>();
                    light.Type  = (float)Graphics::LightType::DirectionalLight;
                }
                if(ImGui::MenuItem("Spot Light"))
                {
                    auto entity = CreateEntityAtPosition("Spot Light");
                    auto& light = entity.AddComponent<Graphics::Light>();
                    light.Type  = (float)Graphics::LightType::SpotLight;
                }
                ImGui::EndMenu();
            }

            if(ImGui::MenuItem(ICON_MDI_CAMERA " Camera"))
            {
                auto entity = CreateEntityAtPosition("Camera");
                entity.AddComponent<Camera>();
            }

            if(ImGui::MenuItem(ICON_MDI_VOLUME_HIGH " Audio Source"))
            {
                auto entity = CreateEntityAtPosition("Audio Source");
                entity.AddComponent<SoundComponent>();
            }

            if(ImGui::MenuItem(ICON_MDI_SCRIPT " Lua Script"))
            {
                auto entity = CreateEntityAtPosition("Lua Script");
                entity.AddComponent<LuaScriptComponent>();
            }

            ImGui::Separator();

            const auto& copiedEntities = m_Editor->GetCopiedEntity();
            if(!copiedEntities.empty())
            {
                if(ImGui::MenuItem(ICON_MDI_CONTENT_PASTE " Paste"))
                {
                    bool wasCutOperation = m_Editor->GetCutCopyEntity();
                    editor->ClearSelected();

                    for(auto copiedEntity : copiedEntities)
                    {
                        if(copiedEntity.Valid())
                        {
                            Entity newEntity = scene->CreateEntity();
                            scene->DuplicateEntity(copiedEntity, newEntity);
                            newEntity.GetOrAddComponent<Maths::Transform>().SetLocalPosition(pos);
                            editor->SetSelected(newEntity);

                            if(wasCutOperation)
                            {
                                scene->DestroyEntity(copiedEntity);
                            }
                        }
                    }

                    if(wasCutOperation)
                    {
                        m_Editor->SetCopiedEntity({});
                    }
                }
            }
            else
            {
                ImGui::TextDisabled(ICON_MDI_CONTENT_PASTE " Paste");
            }

            ImGui::EndPopup();
            m_ContextMenuPending = false;
        }
    }
}
