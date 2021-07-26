#include "SceneViewPanel.h"
#include "Editor.h"
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/Renderers/RenderGraph.h>
#include <Lumos/Graphics/GBuffer.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Graphics/Renderers/GridRenderer.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/ImGui/ImGuiHelpers.h>
#include <Lumos/Events/ApplicationEvent.h>

#include <box2d/box2d.h>
#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
namespace Lumos
{
    SceneViewPanel::SceneViewPanel()
    {
        m_Name = ICON_MDI_GAMEPAD_VARIANT " Scene###scene";
        m_SimpleName = "Scene";
        m_CurrentScene = nullptr;

        m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
        m_ShowComponentGizmoMap[typeid(Camera).hash_code()] = true;
        m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()] = true;

        m_AspectRatio = 1.0;
        m_Width = 1280;
        m_Height = 800;
    }

    static std::string AspectToString(float aspect)
    {
        if(Maths::Equals(aspect, 16.0f / 10.0f))
        {
            return "16:10";
        }
        else if(Maths::Equals(aspect, 16.0f / 9.0f))
        {
            return "16:9";
        }
        else if(Maths::Equals(aspect, 4.0f / 3.0f))
        {
            return "4:3";
        }
        else if(Maths::Equals(aspect, 3.0f / 2.0f))
        {
            return "3:2";
        }
        else if(Maths::Equals(aspect, 9.0f / 16.0f))
        {
            return "9:16";
        }
        else
        {
            return "Unsupported";
        }
    }

    static float StringToAspect(const std::string& aspect)
    {
        if(aspect == "16:10")
        {
            return 16.0f / 10.0f;
        }
        else if(aspect == "16:9")
        {
            return 16.0f / 9.0f;
        }
        else if(aspect == "4:3")
        {
            return 4.0f / 3.0f;
        }
        else if(aspect == "3:2")
        {
            return 3.0f / 2.0f;
        }
        else if(aspect == "9:16")
        {
            return 9.0f / 16.0f;
        }
        else
        {
            return 1.0f;
        }
    }

    void SceneViewPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        Application& app = Application::Get();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin(m_Name.c_str(), &m_Active, flags);

        Camera* camera = nullptr;
        Maths::Transform* transform = nullptr;

        bool gameView = false;

        if(app.GetEditorState() == EditorState::Preview)
        {
            camera = m_Editor->GetCamera();
            transform = &m_Editor->GetEditorCameraTransform();

            app.GetRenderGraph()->SetOverrideCamera(camera, transform);

            DebugRenderer::SetOverrideCamera(camera, transform);
        }
        else
        {
            gameView = true;

            app.GetRenderGraph()->SetOverrideCamera(nullptr, nullptr);
            DebugRenderer::SetOverrideCamera(nullptr, nullptr);

            auto& registry = m_CurrentScene->GetRegistry();
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                camera = &registry.get<Camera>(cameraView.front());
            }
        }

        ImVec2 offset = { 0.0f, 0.0f };

        if(!gameView)
        {
            ToolBar();
            offset = ImGui::GetCursorPos(); //Usually ImVec2(0.0f, 50.0f);
        }

        if(!camera)
        {
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        ImGuizmo::SetDrawlist();
        auto sceneViewSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() - offset / 2.0f; // - offset * 0.5f;
        auto sceneViewPosition = ImGui::GetWindowPos() + offset;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        if(!m_FreeAspect)
        {
            float heightNeededForAspect = sceneViewSize.x / m_FixedAspect;

            if(heightNeededForAspect > sceneViewSize.y)
            {
                sceneViewSize.x = sceneViewSize.y * m_FixedAspect;
                float xOffset = ((ImGui::GetContentRegionAvail() - sceneViewSize) * 0.5f).x;
                sceneViewPosition.x += xOffset;
                ImGui::SetCursorPos({ xOffset, ImGui::GetCursorPosY() + offset.x });
                offset.x += xOffset;
            }
            else
            {
                sceneViewSize.y = sceneViewSize.x / m_FixedAspect;
                float yOffset = ((ImGui::GetContentRegionAvail() - sceneViewSize) * 0.5f).y;
                sceneViewPosition.y += yOffset;

                ImGui::SetCursorPos({ ImGui::GetCursorPosX(), yOffset + offset.y });
                offset.y += yOffset;
            }
        }

        float aspect = static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y);

        if(!Maths::Equals(aspect, camera->GetAspectRatio()))
        {
            camera->SetAspectRatio(aspect);
        }
        m_Editor->m_SceneWindowPos = Maths::Vector2(sceneViewPosition.x, sceneViewPosition.y);

        if(m_HalfRes)
            sceneViewSize /= 2.0f;

        Resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

        if(m_HalfRes)
            sceneViewSize *= 2.0f;

        ImGuiHelpers::Image(m_GameViewTexture.get(), Maths::Vector2(sceneViewSize.x, sceneViewSize.y));

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = sceneViewPosition;

        ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        bool updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound) || Input::Get().GetMouseMode() == MouseMode::Captured;

        app.SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

        if(gameView)
        {
            ImGui::End();
            ImGui::PopStyleVar();

            return;
        }

        ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);

        if(m_Editor->ShowGrid())
        {
            if(camera->IsOrthographic())
            {
                m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { transform->GetWorldPosition().x, transform->GetWorldPosition().y }, sceneViewPosition, { sceneViewSize.x, sceneViewSize.y }, 1.0f, 1.5f);
            }
        }

        ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, { sceneViewSize.x + sceneViewPosition.x, sceneViewSize.y + sceneViewPosition.y - 2.0f });

        m_Editor->OnImGuizmo();

        if(!gameView && app.GetSceneActive() && !ImGuizmo::IsUsing() && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
        {
            float dpi = Application::Get().GetWindowDPI();
            auto clickPos = Input::Get().GetMousePosition() - Maths::Vector2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);
            m_Editor->SelectObject(m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x) / dpi, int(sceneViewSize.y) / dpi));
        }

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

        DrawGizmos(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y, app.GetSceneManager()->GetCurrentScene());

        if(m_ShowStats && ImGui::IsWindowFocused())
        {
            static bool p_open = true;
            const float DISTANCE = 5.0f;
            static int corner = 0;

            if(corner != -1)
            {
                ImVec2 window_pos = ImVec2((corner & 1) ? (sceneViewPosition.x + sceneViewSize.x - DISTANCE) : (sceneViewPosition.x + DISTANCE), (corner & 2) ? (sceneViewPosition.y + sceneViewSize.y - DISTANCE) : (sceneViewPosition.y + DISTANCE));
                ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            }

            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            if(ImGui::Begin("Example: Simple overlay", &p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
            {
                ImGuiIO& io = ImGui::GetIO();

                static Engine::Stats stats = Engine::Get().Statistics();
                static float timer = 1.0f;
                timer += io.DeltaTime;

                if(timer > 1.0f)
                {
                    timer = 0.0f;
                    stats = Engine::Get().Statistics();
                }
                Engine::Get().ResetStats();

                ImGui::Text("%.2f ms (%i FPS)", stats.FrameTime * 1000.0f, stats.FramesPerSecond);
                ImGui::Separator();

                if(ImGui::IsMousePosValid())
                    ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
                else
                    ImGui::TextUnformatted("Mouse Position: <invalid>");

                ImGui::Text("Num Rendered Objects %u", stats.NumRenderedObjects);
                ImGui::Text("Num Shadow Objects %u", stats.NumShadowObjects);
                ImGui::Text("Num Draw Calls  %u", stats.NumDrawCalls);
                ImGui::Text("Used GPU Memory : %.1f mb | Total : %.1f mb", stats.UsedGPUMemory * 0.000001f, stats.TotalGPUMemory * 0.000001f);

                if(ImGui::BeginPopupContextWindow())
                {
                    if(ImGui::MenuItem("Custom", NULL, corner == -1))
                        corner = -1;
                    if(ImGui::MenuItem("Top-left", NULL, corner == 0))
                        corner = 0;
                    if(ImGui::MenuItem("Top-right", NULL, corner == 1))
                        corner = 1;
                    if(ImGui::MenuItem("Bottom-left", NULL, corner == 2))
                        corner = 2;
                    if(ImGui::MenuItem("Bottom-right", NULL, corner == 3))
                        corner = 3;
                    if(p_open && ImGui::MenuItem("Close"))
                        p_open = false;
                    ImGui::EndPopup();
                }
            }
            ImGui::End();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SceneViewPanel::DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        Camera* camera = m_Editor->GetCamera();
        Maths::Transform& cameraTransform = m_Editor->GetEditorCameraTransform();
        auto& registry = scene->GetRegistry();
        Maths::Matrix4 view = cameraTransform.GetWorldMatrix().Inverse();
        Maths::Matrix4 proj = camera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
        if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
            proj.m11_ *= -1.0f;
#endif

        Maths::Matrix4 viewProj = proj * view;
        const Maths::Frustum& f = camera->GetFrustum(view);

        ShowComponentGizmo<Graphics::Light>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<Camera>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<SoundComponent>(width, height, xpos, ypos, viewProj, f, registry);
    }

    void SceneViewPanel::ToolBar()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        bool selected = false;

        {
            selected = m_Editor->GetImGuizmoOperation() == 4;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_CURSOR_DEFAULT))
                m_Editor->SetImGuizmoOperation(4);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Select");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Translate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ROTATE_ORBIT))
                m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Rotate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Scale");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_BORDER_NONE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Bounds");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::SameLine();
        {
            selected = (m_Editor->SnapGuizmo() == true);

            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            if(ImGui::Button(ICON_MDI_MAGNET))
                m_Editor->SnapGuizmo() = !selected;

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Snap");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        if(ImGui::Button("Gizmos " ICON_MDI_CHEVRON_DOWN))
            ImGui::OpenPopup("GizmosPopup");
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

                auto physics2D = Application::Get().GetSystem<B2PhysicsEngine>();

                if(physics2D)
                {
                    uint32_t flags = physics2D->GetDebugDrawFlags();

                    bool show2DShapes = flags & b2Draw::e_shapeBit;
                    if(ImGui::Checkbox("Shapes (2D)", &show2DShapes))
                    {
                        if(show2DShapes)
                            flags += b2Draw::e_shapeBit;
                        else
                            flags -= b2Draw::e_shapeBit;
                    }

                    bool showCOG = flags & b2Draw::e_centerOfMassBit;
                    if(ImGui::Checkbox("Centre of Mass (2D)", &showCOG))
                    {
                        if(showCOG)
                            flags += b2Draw::e_centerOfMassBit;
                        else
                            flags -= b2Draw::e_centerOfMassBit;
                    }

                    bool showJoint = flags & b2Draw::e_jointBit;
                    if(ImGui::Checkbox("Joint Connection (2D)", &showJoint))
                    {
                        if(showJoint)
                            flags += b2Draw::e_jointBit;
                        else
                            flags -= b2Draw::e_jointBit;
                    }

                    bool showAABB = flags & b2Draw::e_aabbBit;
                    if(ImGui::Checkbox("AABB (2D)", &showAABB))
                    {
                        if(showAABB)
                            flags += b2Draw::e_aabbBit;
                        else
                            flags -= b2Draw::e_aabbBit;
                    }

                    bool showPairs = static_cast<bool>(flags & b2Draw::e_pairBit);
                    if(ImGui::Checkbox("Broadphase Pairs  (2D)", &showPairs))
                    {
                        if(showPairs)
                            flags += b2Draw::e_pairBit;
                        else
                            flags -= b2Draw::e_pairBit;
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

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        {
            selected = m_ShowStats;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button("Stats"))
            {
                m_ShowStats = !m_ShowStats;
            }

            if(selected)
                ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Show Statistics");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        //Editor Camera Settings

        auto& camera = *m_Editor->GetCamera();
        bool ortho = camera.IsOrthographic();

        selected = !ortho;
        if(selected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());
        if(ImGui::Button(ICON_MDI_AXIS_ARROW " 3D"))
        {
            if(ortho)
            {
                camera.SetIsOrthographic(false);
                m_Editor->GetEditorCameraController().SetMode(false);
            }
        }
        if(selected)
            ImGui::PopStyleColor();
        ImGui::SameLine();

        selected = ortho;
        if(selected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());
        if(ImGui::Button(ICON_MDI_ANGLE_RIGHT "2D"))
        {
            if(!ortho)
            {
                camera.SetIsOrthographic(true);
                m_Editor->GetEditorCameraTransform().SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f));
                m_Editor->GetEditorCameraController().SetMode(true);
            }
        }
        if(selected)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        static std::string supportedAspects[] = { "Free Aspect", "16:10", "16:9", "4:3", "3:2", "9:16" };

        if(ImGui::Button("Aspect " ICON_MDI_CHEVRON_DOWN))
            ImGui::OpenPopup("AspectPopup");
        if(ImGui::BeginPopup("AspectPopup"))
        {
            std::string currentAspect = m_FreeAspect ? "Free Aspect" : AspectToString(m_FixedAspect);

            for(int n = 0; n < 6; n++)
            {
                bool is_selected = (currentAspect == supportedAspects[n]);
                if(ImGui::Checkbox(supportedAspects[n].c_str(), &is_selected))
                {
                    if(supportedAspects[n] == "Free Aspect")
                    {
                        m_FreeAspect = is_selected;
                    }
                    else
                    {
                        m_FreeAspect = false;
                        m_FixedAspect = StringToAspect(supportedAspects[n]);
                    }
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        {
            selected = m_HalfRes;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            if(ImGui::Button("Half Res"))
                m_HalfRes = !m_HalfRes;

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Scene view with halved resolution");

            if(selected)
                ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        {
            selected = m_Editor->FullScreenOnPlay();
            if(m_Editor->FullScreenOnPlay())
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::GetSelectedColour());

            if(ImGui::Button("Maximise"))
                m_Editor->FullScreenOnPlay() = !m_Editor->FullScreenOnPlay();

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Maximise on play");

            if(selected)
                ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    void SceneViewPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_AspectRatio = 1.0f;
        m_CurrentScene = scene;

        auto renderGraph = Application::Get().GetRenderGraph();
        renderGraph->SetRenderTarget(m_GameViewTexture.get(), true);
        renderGraph->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
        m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), true);

        DebugRenderer::SetRenderTarget(m_GameViewTexture.get(), true);
        DebugRenderer::SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
    }

    void SceneViewPanel::Resize(uint32_t width, uint32_t height)
    {
        bool resize = false;

        LUMOS_ASSERT(width > 0 && height > 0, "Scene View Dimensions 0");

        Application::Get().SetSceneViewDimensions(width, height);

        if(m_Width != width || m_Height != height)
        {
            resize = true;
            m_Width = width;
            m_Height = height;
        }

        if(!m_GameViewTexture)
            m_GameViewTexture = SharedRef<Graphics::Texture2D>(Graphics::Texture2D::Create());

        if(resize)
        {
            Graphics::GraphicsContext::GetContext()->WaitIdle();

            m_GameViewTexture->BuildTexture(Graphics::TextureFormat::RGBA8, m_Width, m_Height, false, false, false);

            auto renderGraph = Application::Get().GetRenderGraph();
            renderGraph->SetRenderTarget(m_GameViewTexture.get(), true, false);

            DebugRenderer::SetRenderTarget(m_GameViewTexture.get(), false);

            if(!m_Editor->GetGridRenderer())
                m_Editor->CreateGridRenderer();
            m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);

            WindowResizeEvent e(width, height);
            auto& app = Application::Get();
            app.GetRenderGraph()->OnResize(width, height);

            renderGraph->OnEvent(e);

            DebugRenderer::OnResize(width, height);
            m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

            Graphics::GraphicsContext::GetContext()->WaitIdle();
        }
    }
}
