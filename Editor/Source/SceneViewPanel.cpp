#include "SceneViewPanel.h"
#include "Editor.h"
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/SwapChain.h>
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
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Events/ApplicationEvent.h>

#include <box2d/box2d.h>
#include <imgui/imgui_internal.h>
#include <imgui/Plugins/ImGuizmo.h>
namespace Lumos
{
    SceneViewPanel::SceneViewPanel()
    {
        m_Name         = ICON_MDI_GAMEPAD_VARIANT " Scene###scene";
        m_SimpleName   = "Scene";
        m_CurrentScene = nullptr;

        m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
        m_ShowComponentGizmoMap[typeid(Camera).hash_code()]          = true;
        m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]  = true;

        m_Width  = 1280;
        m_Height = 800;
    }

    void SceneViewPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        Application& app = Application::Get();

        ImGuiUtilities::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if(!ImGui::Begin(m_Name.c_str(), &m_Active, flags) || !m_CurrentScene)
        {
            app.SetDisableMainRenderGraph(true);
            ImGui::End();
            return;
        }

        Camera* camera              = nullptr;
        Maths::Transform* transform = nullptr;

        app.SetDisableMainRenderGraph(false);

        // if(app.GetEditorState() == EditorState::Preview)
        {
            LUMOS_PROFILE_SCOPE("Set Override Camera");
            camera    = m_Editor->GetCamera();
            transform = &m_Editor->GetEditorCameraTransform();

            app.GetRenderGraph()->SetOverrideCamera(camera, transform);
        }

        ImVec2 offset = { 0.0f, 0.0f };

        {
            ToolBar();
            offset = ImGui::GetCursorPos(); // Usually ImVec2(0.0f, 50.0f);
        }

        if(!camera)
        {
            ImGui::End();
            ImGui::PopStyleVar();
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
        m_Editor->m_SceneViewPanelPosition = glm::vec2(sceneViewPosition.x, sceneViewPosition.y);

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 0.5f;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        Resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 2.0f;

        ImGuiUtilities::Image(m_GameViewTexture.get(), glm::vec2(sceneViewSize.x, sceneViewSize.y));

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = sceneViewPosition;

        ImVec2 maxBound   = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        bool updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound); // || Input::Get().GetMouseMode() == MouseMode::Captured;

        app.SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

        ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);

        m_Editor->SetSceneViewActive(updateCamera);

        if(m_Editor->ShowGrid())
        {
            if(camera->IsOrthographic())
            {
                LUMOS_PROFILE_SCOPE("2D Grid");

                m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { transform->GetWorldPosition().x, transform->GetWorldPosition().y }, sceneViewPosition, { sceneViewSize.x, sceneViewSize.y }, 1.0f, 1.5f);
            }
        }

        {
            LUMOS_PROFILE_SCOPE("Push Clip Rect");
            ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, { sceneViewSize.x + sceneViewPosition.x, sceneViewSize.y + sceneViewPosition.y - 2.0f });
        }

        m_Editor->OnImGuizmo();

        if(updateCamera && app.GetSceneActive() && !ImGuizmo::IsUsing() && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
        {
            LUMOS_PROFILE_SCOPE("Select Object");

            float dpi     = Application::Get().GetWindowDPI();
            auto clickPos = Input::Get().GetMousePosition() - glm::vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

            Maths::Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi), int(sceneViewSize.y / dpi));
            m_Editor->SelectObject(ray);
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

        if(app.GetSceneManager()->GetCurrentScene())
            DrawGizmos(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y, app.GetSceneManager()->GetCurrentScene());

        ImGui::End();
    }

    void SceneViewPanel::DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        Camera* camera                    = m_Editor->GetCamera();
        Maths::Transform& cameraTransform = m_Editor->GetEditorCameraTransform();
        auto& registry                    = scene->GetRegistry();
        glm::mat4 view                    = glm::inverse(cameraTransform.GetWorldMatrix());
        glm::mat4 proj                    = camera->GetProjectionMatrix();
        glm::mat4 viewProj                = proj * view;
        const Maths::Frustum& f           = camera->GetFrustum(view);

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
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_CURSOR_DEFAULT))
                m_Editor->SetImGuizmoOperation(4);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Select");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Translate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ROTATE_ORBIT))
                m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Rotate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Scale");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::UNIVERSAL;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_CROP_ROTATE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::UNIVERSAL);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Universal");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_BORDER_NONE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Bounds");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::SameLine();
        {
            selected = (m_Editor->SnapGuizmo() == true);

            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());

            if(ImGui::Button(ICON_MDI_MAGNET))
                m_Editor->SnapGuizmo() = !selected;

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Snap");
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

                uint32_t flags = m_Editor->GetSettings().m_DebugDrawFlags;

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
        // Editor Camera Settings

        auto& camera = *m_Editor->GetCamera();
        bool ortho   = camera.IsOrthographic();

        selected = !ortho;
        if(selected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());
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
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetSelectedColour());
        if(ImGui::Button(ICON_MDI_ANGLE_RIGHT "2D"))
        {
            if(!ortho)
            {
                camera.SetIsOrthographic(true);
                m_Editor->GetEditorCameraTransform().SetLocalOrientation(glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)));
                m_Editor->GetEditorCameraController().SetMode(true);
            }
        }
        if(selected)
            ImGui::PopStyleColor();

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    void SceneViewPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_Editor->GetSettings().m_AspectRatio = 1.0f;
        m_CurrentScene                        = scene;

        auto renderGraph = Application::Get().GetRenderGraph();
        renderGraph->SetRenderTarget(m_GameViewTexture.get(), true);
        renderGraph->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
        m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), true);
        m_Editor->GetGridRenderer()->SetDepthTarget(renderGraph->GetForwardData().m_DepthTexture);
    }

    void SceneViewPanel::Resize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        bool resize = false;

        LUMOS_ASSERT(width > 0 && height > 0, "Scene View Dimensions 0");

        Application::Get().SetSceneViewDimensions(width, height);

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

            auto renderGraph = Application::Get().GetRenderGraph();
            renderGraph->SetRenderTarget(m_GameViewTexture.get(), true, false);

            if(!m_Editor->GetGridRenderer())
                m_Editor->CreateGridRenderer();
            m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);
            m_Editor->GetGridRenderer()->SetDepthTarget(renderGraph->GetForwardData().m_DepthTexture);

            WindowResizeEvent e(width, height);
            auto& app = Application::Get();
            app.GetRenderGraph()->OnResize(width, height);

            renderGraph->OnEvent(e);

            m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

            // Should be just build texture and scene renderer set render target
            // Renderer::GetGraphicsContext()->WaitIdle();
        }
    }
}
