#include "InspectorPanel.h"
#include "Editor.h"
#include "FileBrowserPanel.h"
#include "TextEditPanel.h"
#include <Lumos/Audio/AudioManager.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/String.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Component/Components.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Scene/Component/RigidBody3DComponent.h>
#include <Lumos/Scene/Component/RigidBody2DComponent.h>
#include <Lumos/Scene/Component/TextureMatrixComponent.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Graphics/Sprite.h>
#include <Lumos/Graphics/AnimatedSprite.h>
#include <Lumos/Graphics/Animation/Skeleton.h>
#include <Lumos/Graphics/Animation/Animation.h>
#include <Lumos/Graphics/Animation/AnimationController.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Graphics/MeshFactory.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/ParticleManager.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Renderer.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Scene/SceneGraph.h>

#include <Lumos/Graphics/Material.h>
#include <Lumos/Graphics/Environment.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/HullCollisionShape.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Maths/Quaternion.h>
#include <Lumos/Maths/Matrix4.h>

#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/ImGui/ImGuiManager.h>
#include <Lumos/Graphics/RHI/IMGUIRenderer.h>

#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#include <Lumos/Scene/Serialisation/SerialisationImplementation.h>

#include <sol/sol.hpp>
#include <cstdint>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <inttypes.h>

static bool DebugView = false;

namespace MM
{
#define PropertySet(name, getter, setter)                \
    {                                                    \
        auto value = getter();                           \
        if(Lumos::ImGuiUtilities::Property(name, value)) \
            setter(value);                               \
    }

    template <>
    void ComponentEditorWidget<Lumos::LuaScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& script     = reg.get<Lumos::LuaScriptComponent>(e);
        bool hasReloaded = false;
        bool loaded      = false;
        ImGui::Columns(2);
        if(!script.Loaded())
        {
            ImGui::Text("Script Failed to Load : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else if(!script.Loaded() && script.GetFilePath().empty())
        {
            ImGui::Text("FilePath empty : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else
            loaded = true;

        // Get all .lua files from Assets/Scripts directory
        Lumos::ArenaTemp scratchTemp = Lumos::ScratchBegin(0, 0);
        Lumos::String8 scriptsPath = Lumos::Str8Lit("//Assets/Scripts");
        Lumos::String8 physicalScriptsPath;

        std::vector<std::string> luaFiles;
        std::vector<std::string> luaFilesVirtual;

        if(Lumos::FileSystem::Get().ResolvePhysicalPath(scratchTemp.arena, scriptsPath, &physicalScriptsPath, true))
        {
            try
            {
                if(fs::exists((const char*)physicalScriptsPath.str) && fs::is_directory((const char*)physicalScriptsPath.str))
                {
                    for(const auto& entry : fs::recursive_directory_iterator((const char*)physicalScriptsPath.str))
                    {
                        if(entry.is_regular_file() && entry.path().extension() == ".lua")
                        {
                            std::string absolutePath = entry.path().string();
                            luaFiles.push_back(absolutePath);

                            // Convert to //Assets virtual path
                            Lumos::String8 absPathStr = Lumos::PushStr8Copy(scratchTemp.arena, absolutePath.c_str());
                            Lumos::String8 virtualPath = Lumos::FileSystem::Get().AbsolutePathToFileSystem(scratchTemp.arena, absPathStr, false);
                            luaFilesVirtual.push_back(std::string((const char*)virtualPath.str, virtualPath.size));
                        }
                    }
                }
            }
            catch(...) {}
        }

        std::string currentFilePath = script.GetFilePath();
        std::string currentFileName = "None";

        if(!currentFilePath.empty())
        {
            size_t lastSlash = currentFilePath.find_last_of("/\\");
            currentFileName = (lastSlash != std::string::npos) ? currentFilePath.substr(lastSlash + 1) : currentFilePath;
        }

        ImGui::TextUnformatted("Script");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if(ImGui::BeginCombo("##ScriptFile", currentFileName.c_str()))
        {
            static char searchBuffer[256] = "";
            ImGui::SetNextItemWidth(-1);
            if(ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                searchBuffer[0] = '\0';
            }
            ImGui::InputTextWithHint("##ScriptSearch", ICON_MDI_MAGNIFY " Search scripts...", searchBuffer, sizeof(searchBuffer));

            std::string searchStr = searchBuffer;
            std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

            ImGui::Separator();

            // "None" option to clear the script
            if(searchStr.empty() || std::string("none").find(searchStr) != std::string::npos)
            {
                if(ImGui::Selectable("None", currentFilePath.empty()))
                {
                    script.SetFilePath("");
                    searchBuffer[0] = '\0';
                }
            }

            for(size_t i = 0; i < luaFilesVirtual.size(); i++)
            {
                const auto& virtualPath = luaFilesVirtual[i];

                size_t lastSlash = virtualPath.find_last_of("/");
                std::string displayName = (lastSlash != std::string::npos) ? virtualPath.substr(lastSlash + 1) : virtualPath;

                if(!searchStr.empty())
                {
                    std::string lowerDisplayName = displayName;
                    std::string lowerVirtualPath = virtualPath;
                    std::transform(lowerDisplayName.begin(), lowerDisplayName.end(), lowerDisplayName.begin(), ::tolower);
                    std::transform(lowerVirtualPath.begin(), lowerVirtualPath.end(), lowerVirtualPath.begin(), ::tolower);

                    if(lowerDisplayName.find(searchStr) == std::string::npos &&
                       lowerVirtualPath.find(searchStr) == std::string::npos)
                    {
                        continue;
                    }
                }

                // Show relative path in tooltip
                bool isSelected = (currentFilePath == virtualPath || currentFilePath == luaFiles[i]);
                if(ImGui::Selectable(displayName.c_str(), isSelected))
                {
                    script.SetFilePath(virtualPath);
                    script.Reload();
                    hasReloaded = true;
                    searchBuffer[0] = '\0';
                }

                if(ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", virtualPath.c_str());
                }
            }

            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        ImGui::Columns(1);

        Lumos::ScratchEnd(scratchTemp);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        {
            const char* statusIcon = ICON_MDI_ALERT_CIRCLE;
            ImVec4 statusColor = ImVec4(0.8f, 0.8f, 0.1f, 1.0f);
            const char* statusText = "No script";

            if(!currentFilePath.empty())
            {
                if(script.Loaded())
                {
                    statusIcon = ICON_MDI_CHECK_CIRCLE;
                    statusColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
                    statusText = "Loaded";
                }
                else
                {
                    statusIcon = ICON_MDI_ALERT_CIRCLE;
                    statusColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    statusText = "Failed to load";
                }
            }

            ImGui::TextColored(statusColor, "%s", statusIcon);
            ImGui::SameLine();
            ImGui::TextColored(statusColor, "%s", statusText);

            if(script.Loaded() && !currentFilePath.empty())
            {
                Lumos::ArenaTemp infoScratch = Lumos::ScratchBegin(0, 0);
                Lumos::String8 virtualPath = Lumos::PushStr8Copy(infoScratch.arena, currentFilePath.c_str());
                Lumos::String8 physicalPath;
                if(Lumos::FileSystem::Get().ResolvePhysicalPath(infoScratch.arena, virtualPath, &physicalPath, false))
                {
                    int64_t fileSize = Lumos::FileSystem::GetFileSize(physicalPath);
                    if(fileSize >= 0)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("|");
                        ImGui::SameLine();

                        if(fileSize < 1024)
                            ImGui::TextDisabled("%lld bytes", fileSize);
                        else if(fileSize < 1024 * 1024)
                            ImGui::TextDisabled("%.1f KB", fileSize / 1024.0f);
                        else
                            ImGui::TextDisabled("%.1f MB", fileSize / (1024.0f * 1024.0f));
                    }
                }
                Lumos::ScratchEnd(infoScratch);
            }
        }

        ImGui::Spacing();

        Lumos::ArenaTemp Scratch = Lumos::ScratchBegin(0, 0);

        float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;

        {
            if(ImGui::Button(ICON_MDI_FILE_PLUS " New", ImVec2(buttonWidth, 0.0f)))
            {
                Lumos::String8 newFilePath = Lumos::Str8Lit("//Assets/Scripts");
                Lumos::String8 physicalPath;
                if(!Lumos::FileSystem::Get().ResolvePhysicalPath(Scratch.arena, newFilePath, &physicalPath, true))
                {
                    LERROR("Failed to Create Lua script %s", (const char*)physicalPath.str);
                }
                else
                {
                    Lumos::String8 defaultScript = Lumos::Str8Lit(
                        R"(-- Lua Script

function OnInit()
    -- Called when script is initialized
    Log.Info("Script initialized")
end

function OnUpdate(dt)
    -- Called every frame
    -- dt = delta time in seconds
end

function OnCleanUp()
    -- Called when script is destroyed
end
)");
                    Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
                    Lumos::String8 newScriptFileName = Lumos::Str8Lit("NewScript");
                    int fileIndex = 0;

                    Lumos::String8 Path = Lumos::PushStr8FillByte(scratch.arena, 260, 0);
                    Path = Lumos::Str8F(Path, "%s/%s.lua", (const char*)physicalPath.str, (const char*)newScriptFileName.str);

                    while(Lumos::FileSystem::FileExists(Path))
                    {
                        fileIndex++;
                        Path = Lumos::Str8F(Path, "%s/%s%i.lua", (const char*)physicalPath.str, (const char*)newScriptFileName.str, fileIndex);
                    }

                    Lumos::FileSystem::WriteTextFile(Path, defaultScript);

                    Lumos::String8 pathStr = Lumos::PushStr8Copy(scratch.arena, ToStdString(Path).c_str());
                    Lumos::String8 virtualScriptPath = Lumos::FileSystem::Get().AbsolutePathToFileSystem(scratch.arena, pathStr, false);

                    script.SetFilePath(std::string((const char*)virtualScriptPath.str, virtualScriptPath.size));
                    script.Reload();
                    hasReloaded = true;

                    Lumos::ScratchEnd(scratch);
                }
            }
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new Lua script file");

            ImGui::SameLine();

            bool hasFile = !currentFilePath.empty();
            if(!hasFile) ImGui::BeginDisabled();

            if(ImGui::Button(ICON_MDI_FILE_DOCUMENT_EDIT " Edit", ImVec2(buttonWidth, 0.0f)))
            {
                if(!script.GetFilePath().empty())
                {
                    Lumos::Editor::GetEditor()->OpenTextFile(script.GetFilePath(), [&]
                    {
                        script.Reload();
                        hasReloaded = true;
                        auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                        if(textEditPanel)
                            ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
                    });

                    auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                    if(textEditPanel)
                        ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
                }
            }
            if(!hasFile) ImGui::EndDisabled();
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Edit script in text editor");

            ImGui::SameLine();

            if(!hasFile) ImGui::BeginDisabled();

            if(ImGui::Button(ICON_MDI_REFRESH " Reload", ImVec2(buttonWidth, 0.0f)))
            {
                script.Reload();
                hasReloaded = true;
            }
            if(!hasFile) ImGui::EndDisabled();
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Reload script from file (Ctrl+R)");
        }

        {
            float secondaryButtonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

            if(ImGui::Button(ICON_MDI_FOLDER_OPEN " Browse...", ImVec2(secondaryButtonWidth, 0.0f)))
            {
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(std::bind(&Lumos::LuaScriptComponent::LoadScript, &script, std::placeholders::_1));
            }
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Browse for a Lua script file");

            ImGui::SameLine();

            bool hasFileToCopy = !currentFilePath.empty();
            if(!hasFileToCopy) ImGui::BeginDisabled();

            if(ImGui::Button(ICON_MDI_CONTENT_COPY " Copy Path", ImVec2(secondaryButtonWidth, 0.0f)))
            {
                ImGui::SetClipboardText(currentFilePath.c_str());
            }
            if(!hasFileToCopy) ImGui::EndDisabled();
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Copy virtual path to clipboard");
        }

        ScratchEnd(Scratch);

        if(const ImGuiPayload* payload = ImGui::GetDragDropPayload())
        {
            if(payload->IsDataType("AssetFile"))
            {
                if(ImGui::BeginDragDropTarget())
                {
                    if(ImGui::AcceptDragDropPayload("AssetFile"))
                    {
                        auto filePathDropped = std::string(reinterpret_cast<const char*>(payload->Data));
                        script.SetFilePath(filePathDropped);
                        script.Reload();
                        hasReloaded = true;
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }

        if(!script.Loaded() && !script.GetFilePath().empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.1f, 0.1f, 0.3f));
            ImGui::BeginChild("ErrorBanner", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ICON_MDI_ALERT_OCTAGON " Script Load Failed");

            const auto& errors = script.GetErrors();
            if(!errors.empty())
            {
                ImGui::Spacing();
                ImGui::TextWrapped("The script contains errors and could not be loaded:");
                ImGui::Spacing();

                for(const auto& err : errors)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
                    ImGui::BulletText("Line %d: %s", err.first, err.second.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::Spacing();

                if(ImGui::Button(ICON_MDI_FILE_DOCUMENT_EDIT " Open and Fix", ImVec2(-1, 0)))
                {
                    if(!script.GetFilePath().empty())
                    {
                        Lumos::Editor::GetEditor()->OpenTextFile(script.GetFilePath(), [&]
                        {
                            script.Reload();
                            auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                            if(textEditPanel)
                                ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
                        });

                        auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                        if(textEditPanel)
                            ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
                    }
                }
            }
            else
            {
                ImGui::Spacing();
                ImGui::TextWrapped("The script file could not be loaded. The file may not exist or may have incorrect syntax.");
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();

            return;
        }

        if(script.Loaded())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if(ImGui::TreeNodeEx(ICON_MDI_INFORMATION " Script Info"))
            {
                ImGui::Indent();

                ImGui::TextDisabled("Path:");
                ImGui::SameLine();
                ImGui::TextWrapped("%s", currentFilePath.c_str());

                Lumos::ArenaTemp funcScratch = Lumos::ScratchBegin(0, 0);
                Lumos::String8 virtualPath = Lumos::PushStr8Copy(funcScratch.arena, currentFilePath.c_str());
                Lumos::String8 physicalPath;

                if(Lumos::FileSystem::Get().ResolvePhysicalPath(funcScratch.arena, virtualPath, &physicalPath, false))
                {
                    Lumos::String8 scriptContent = Lumos::FileSystem::ReadTextFile(funcScratch.arena, physicalPath);
                    if(scriptContent.size > 0)
                    {
                        std::vector<std::string> functions;
                        const char* content = (const char*)scriptContent.str;
                        const char* pos = content;

                        while(pos && *pos)
                        {
                            const char* funcPos = strstr(pos, "function ");
                            if(!funcPos) break;

                            funcPos += 9;

                            while(*funcPos == ' ' || *funcPos == '\t') funcPos++;

                            const char* nameStart = funcPos;
                            while(*funcPos && (*funcPos == '_' || isalnum(*funcPos))) funcPos++;

                            if(funcPos > nameStart)
                            {
                                std::string funcName(nameStart, funcPos - nameStart);
                                if(!funcName.empty())
                                    functions.push_back(funcName);
                            }

                            pos = funcPos;
                        }

                        if(!functions.empty())
                        {
                            ImGui::TextDisabled("Functions:");
                            ImGui::SameLine();
                            ImGui::Text("%d detected", functions.size());

                            if(ImGui::TreeNode("Function List"))
                            {
                                for(const auto& func : functions)
                                {
                                    ImGui::BulletText("%s()", func.c_str());
                                }
                                ImGui::TreePop();
                            }
                        }
                    }
                }

                Lumos::ScratchEnd(funcScratch);

                ImGui::Unindent();
                ImGui::TreePop();
            }
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::Maths::Transform>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        using namespace Lumos;
        auto& transform = reg.get<Lumos::Maths::Transform>(e);

        auto rotation   = transform.GetLocalOrientation().ToEuler();
        auto position   = transform.GetLocalPosition();
        auto scale      = transform.GetLocalScale();
        float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

        // Call this to fix alignment with columns
        ImGui::AlignTextToFramePadding();

        ImGui::PushID((void*)&position);
        if(Lumos::ImGuiUtilities::PropertyTransform("Position", position, itemWidth, 0.0f))
            transform.SetLocalPosition(position);
        ImGui::PopID();

        ImGui::SameLine();

        ImGui::PushID((void*)&rotation);
        if(Lumos::ImGuiUtilities::PropertyTransform("Rotation", rotation, itemWidth, 0.0f))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch       = Lumos::Maths::Max(pitch, -89.9f);
            transform.SetLocalOrientation(Quat(Vec3(pitch, rotation.y, rotation.z)));
        }
        ImGui::PopID();

        ImGui::SameLine();

        ImGui::PushID((void*)&scale);
        if(Lumos::ImGuiUtilities::PropertyTransform("Scale", scale, itemWidth, 1.0f))
        {
            transform.SetLocalScale(scale);
        }
        ImGui::PopID();

        ImGui::Columns(1);
        ImGui::Separator();

        if(ImGui::Button("Copy Editor Camera Transforn", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
        {
            Lumos::Application* app     = &Lumos::Editor::Get();
            Lumos::Mat4 cameraTransform = ((Lumos::Editor*)app)->GetEditorCameraTransform().GetWorldMatrix();
            transform.SetLocalTransform(cameraTransform);
        }
    }

    static void CuboidCollisionShapeInspector(Lumos::CuboidCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        Lumos::Vec3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePtr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void SphereCollisionShapeInspector(Lumos::SphereCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        float radius = shape->GetRadius();
        if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f))
        {
            shape->SetRadius(radius);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void PyramidCollisionShapeInspector(Lumos::PyramidCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        Lumos::Vec3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePtr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void CapsuleCollisionShapeInspector(Lumos::CapsuleCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        float radius = shape->GetRadius();
        if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetRadius(radius);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }

        float height = shape->GetHeight();
        if(ImGui::DragFloat("##CollisionShapeHeight", &height, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHeight(height);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void HullCollisionShapeInspector(Lumos::HullCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::TextUnformatted("Hull Collision Shape");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if(ImGui::Button("Generate Collider"))
        {
            auto test = Lumos::SharedPtr<Lumos::Graphics::Mesh>(Lumos::Graphics::CreatePrimative(Lumos::Graphics::PrimitiveType::Cube));
            shape->BuildFromMesh(test.get());
        }
    }

    std::string CollisionShape2DTypeToString(Lumos::Shape shape)
    {
        LUMOS_PROFILE_FUNCTION();
        switch(shape)
        {
        case Lumos::Shape::Circle:
            return "Circle";
        case Lumos::Shape::Square:
            return "Square";
        case Lumos::Shape::Custom:
            return "Custom";
        }

        return "Unknown Shape";
    }

    Lumos::Shape StringToCollisionShape2DType(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Circle")
            return Lumos::Shape::Circle;
        if(type == "Square")
            return Lumos::Shape::Square;
        if(type == "Custom")
            return Lumos::Shape::Custom;

        LERROR("Unsupported Collision shape %s", type.c_str());
        return Lumos::Shape::Circle;
    }

    const char* CollisionShapeTypeToString(Lumos::CollisionShapeType type)
    {
        LUMOS_PROFILE_FUNCTION();
        switch(type)
        {
        case Lumos::CollisionShapeType::CollisionCuboid:
            return "Cuboid";
        case Lumos::CollisionShapeType::CollisionSphere:
            return "Sphere";
        case Lumos::CollisionShapeType::CollisionPyramid:
            return "Pyramid";
        case Lumos::CollisionShapeType::CollisionCapsule:
            return "Capsule";
        case Lumos::CollisionShapeType::CollisionHull:
            return "Hull";
        default:
            LERROR("Unsupported Collision shape");
            break;
        }

        return "Error";
    }

    Lumos::CollisionShapeType StringToCollisionShapeType(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Sphere")
            return Lumos::CollisionShapeType::CollisionSphere;
        if(type == "Cuboid")
            return Lumos::CollisionShapeType::CollisionCuboid;
        if(type == "Pyramid")
            return Lumos::CollisionShapeType::CollisionPyramid;
        if(type == "Capsule")
            return Lumos::CollisionShapeType::CollisionCapsule;
        if(type == "Hull")
            return Lumos::CollisionShapeType::CollisionHull;
        LERROR("Unsupported Collision shape %s", type.c_str());
        return Lumos::CollisionShapeType::CollisionSphere;
    }

    template <>
    void ComponentEditorWidget<Lumos::AxisConstraintComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        ImGuiUtilities::ScopedStyle frameStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        ImGui::Columns(2);
        ImGui::Separator();
        AxisConstraintComponent& axisConstraintComponent = reg.get<Lumos::AxisConstraintComponent>(e);

        uint64_t entityID = axisConstraintComponent.GetEntityID();
        Axes axes         = axisConstraintComponent.GetAxes();
        Entity entity     = Application::Get().GetCurrentScene()->GetEntityManager()->GetEntityByUUID(entityID);

        bool hasName = entity ? entity.HasComponent<NameComponent>() : false;
        std::string name;
        if(hasName)
            name = entity.GetComponent<NameComponent>().name;
        else
            name = "Empty";
        ImGui::TextUnformatted("Entity");
        ImGui::NextColumn();
        ImGui::Text("%s", name.c_str());

        if(ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity");
            if(payload)
            {
                size_t count                 = payload->DataSize / sizeof(entt::entity);
                entt::entity droppedEntityID = *(((entt::entity*)payload->Data));
                axisConstraintComponent.SetEntity(Entity(droppedEntityID, Application::Get().GetCurrentScene()).GetID());
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::NextColumn();

        std::vector<std::string> entities;
        uint64_t currentEntityID = axisConstraintComponent.GetEntityID();
        int index                = 0;
        int selectedIndex        = 0;

        auto physics3dEntities = Application::Get().GetCurrentScene()->GetEntityManager()->GetRegistry().view<Lumos::RigidBody3DComponent>();

        for(auto entity : physics3dEntities)
        {
            if(Entity(entity, Application::Get().GetCurrentScene()).GetID() == currentEntityID)
                selectedIndex = index;

            entities.push_back(Entity(entity, Application::Get().GetCurrentScene()).GetName());

            index++;
        }

        static const char* possibleAxes[7] = { "X", "Y", "Z", "XY", "XZ", "YZ", "XYZ" };

        selectedIndex = (int)axes;

        bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Axes", possibleAxes, 7, &selectedIndex);
        if(updated)
            axisConstraintComponent.SetAxes((Axes)selectedIndex);

        // bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Entity", entities.data(), (int)entities.size(), &selectedIndex);

        // if(updated)
        // axisConstraintComponent.SetEntity(Entity(physics3dEntities[selectedIndex], Application::Get().GetCurrentScene()).GetID());

        ImGui::Columns(1);
    }

    template <>
    void ComponentEditorWidget<Lumos::SpringConstraintComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        ImGuiUtilities::ScopedStyle frameStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        ImGui::Columns(2);
        ImGui::Separator();
        SpringConstraintComponent& springConstraintComponent = reg.get<Lumos::SpringConstraintComponent>(e);
        {
            uint64_t entityID = springConstraintComponent.GetEntityID();

            Entity entity = Application::Get().GetCurrentScene()->GetEntityManager()->GetEntityByUUID(entityID);

            bool hasName = entity ? entity.HasComponent<NameComponent>() : false;
            std::string name;
            if(hasName)
                name = entity.GetComponent<NameComponent>().name;
            else
                name = "Empty";
            ImGui::TextUnformatted("Entity");
            ImGui::NextColumn();
            ImGui::Text("%s", name.c_str());

            if(ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity");
                if(payload)
                {
                    size_t count                 = payload->DataSize / sizeof(entt::entity);
                    entt::entity droppedEntityID = *(((entt::entity*)payload->Data));

                    Entity entity = Entity(droppedEntityID, Application::Get().GetCurrentScene());
                    springConstraintComponent.SetEntityID((uint64_t)entity.GetID());
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::NextColumn();
        {
            uint64_t entityID = springConstraintComponent.GetOtherEntityID();

            Entity entity = Application::Get().GetCurrentScene()->GetEntityManager()->GetEntityByUUID(entityID);

            bool hasName = entity ? entity.HasComponent<NameComponent>() : false;
            std::string name;
            if(hasName)
                name = entity.GetComponent<NameComponent>().name;
            else
                name = "Empty";
            ImGui::TextUnformatted("Other Entity");
            ImGui::NextColumn();
            ImGui::Text("%s", name.c_str());

            if(ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity");
                if(payload)
                {
                    size_t count                 = payload->DataSize / sizeof(entt::entity);
                    entt::entity droppedEntityID = *(((entt::entity*)payload->Data));
                    Entity entity                = Entity(droppedEntityID, Application::Get().GetCurrentScene());
                    springConstraintComponent.SetOtherEntityID((uint64_t)entity.GetID());
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::NextColumn();

        float constant = springConstraintComponent.GetConstant();
        if(ImGuiUtilities::Property("Constant", constant))
            springConstraintComponent.SetConstant(constant);

        // ImGui::NextColumn();

        // std::vector<std::string> entities;
        // uint64_t currentEntityID = springConstraintComponent.GetEntityID();
        // int index = 0;
        // int selectedIndex = 0;

        // auto physics3dEntities = Application::Get().GetCurrentScene()->GetEntityManager()->GetRegistry().view<Lumos::RigidBody3DComponent>();

        // for (auto entity : physics3dEntities)
        //{
        //     if (Entity(entity, Application::Get().GetCurrentScene()).GetID() == currentEntityID)
        //         selectedIndex = index;

        //    entities.push_back(Entity(entity, Application::Get().GetCurrentScene()).GetName());

        //    index++;
        //}

        ImGui::Columns(1);
    }

    template <>
    void ComponentEditorWidget<Lumos::RigidBody3DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        auto& phys = reg.get<Lumos::RigidBody3DComponent>(e);

        auto pos             = phys.GetRigidBody()->GetPosition();
        auto force           = phys.GetRigidBody()->GetForce();
        auto torque          = phys.GetRigidBody()->GetTorque();
        auto orientation     = phys.GetRigidBody()->GetOrientation().ToEuler();
        auto angularVelocity = phys.GetRigidBody()->GetAngularVelocity();
        auto friction        = phys.GetRigidBody()->GetFriction();
        auto isStatic        = phys.GetRigidBody()->GetIsStatic();
        auto isRest          = phys.GetRigidBody()->GetIsAtRest();
        auto mass            = 1.0f / phys.GetRigidBody()->GetInverseMass();
        auto velocity        = phys.GetRigidBody()->GetLinearVelocity();
        auto elasticity      = phys.GetRigidBody()->GetElasticity();
        auto angularFactor   = phys.GetRigidBody()->GetAngularFactor();
        auto collisionShape  = phys.GetRigidBody()->GetCollisionShape();
        auto uuid            = phys.GetRigidBody()->GetUUID();

        if(DebugView)
            Lumos::ImGuiUtilities::Property("UUID", (uint64_t&)uuid, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

        if(Lumos::ImGuiUtilities::Property("Position", pos))
            phys.GetRigidBody()->SetPosition(pos);

        if(Lumos::ImGuiUtilities::Property("Velocity", velocity))
            phys.GetRigidBody()->SetLinearVelocity(velocity);

        if(Lumos::ImGuiUtilities::Property("Torque", torque))
            phys.GetRigidBody()->SetTorque(torque);

        if(Lumos::ImGuiUtilities::Property("Orientation", orientation))
            phys.GetRigidBody()->SetOrientation(Lumos::Quat(orientation));

        if(Lumos::ImGuiUtilities::Property("Force", force))
            phys.GetRigidBody()->SetForce(force);

        if(Lumos::ImGuiUtilities::Property("Angular Velocity", angularVelocity))
            phys.GetRigidBody()->SetAngularVelocity(angularVelocity);

        if(Lumos::ImGuiUtilities::Property("Friction", friction, 0.0f, 1.0f))
            phys.GetRigidBody()->SetFriction(friction);

        if(Lumos::ImGuiUtilities::Property("Mass", mass))
        {
            mass = Lumos::Maths::Max(mass, 0.0001f);
            phys.GetRigidBody()->SetInverseMass(1.0f / mass);
        }

        if(Lumos::ImGuiUtilities::Property("Elasticity", elasticity))
            phys.GetRigidBody()->SetElasticity(elasticity);

        if(Lumos::ImGuiUtilities::Property("Static", isStatic))
            phys.GetRigidBody()->SetIsStatic(isStatic);

        if(Lumos::ImGuiUtilities::Property("At Rest", isRest))
            phys.GetRigidBody()->SetIsAtRest(isRest);

        if(Lumos::ImGuiUtilities::Property("Angular Factor", angularFactor))
            phys.GetRigidBody()->SetAngularFactor(angularFactor);

        // Collision Layer UI
        auto collisionLayer = phys.GetRigidBody()->GetCollisionLayer();
        auto collisionMask  = phys.GetRigidBody()->GetCollisionMask();

        const char* layerNames[16] = {
            "Layer 0 (Default)", "Layer 1 (Static)", "Layer 2 (Dynamic)", "Layer 3 (Player)",
            "Layer 4 (Enemy)", "Layer 5 (Trigger)", "Layer 6 (Projectile)", "Layer 7",
            "Layer 8", "Layer 9", "Layer 10", "Layer 11",
            "Layer 12", "Layer 13", "Layer 14", "Layer 15"
        };

        int layerIndex = (int)collisionLayer;
        if(Lumos::ImGuiUtilities::PropertyDropdown("Collision Layer", layerNames, 16, &layerIndex))
            phys.GetRigidBody()->SetCollisionLayer((u16)layerIndex);

        // Collision Mask as checkboxes for each layer
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Collision Mask");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        bool maskChanged = false;
        u16 newMask      = collisionMask;

        // Show checkboxes in a compact grid layout
        for(int i = 0; i < 16; i++)
        {
            bool layerEnabled = (collisionMask & (1 << i)) != 0;
            
            char label[32];
            snprintf(label, sizeof(label), "L%d##mask%d", i, i);
            
            if(ImGui::Checkbox(label, &layerEnabled))
            {
                if(layerEnabled)
                    newMask |= (1 << i);
                else
                    newMask &= ~(1 << i);
                maskChanged = true;
            }

            // 4 checkboxes per row
            if((i + 1) % 4 != 0 && i < 15)
                ImGui::SameLine();
        }

        if(maskChanged)
            phys.GetRigidBody()->SetCollisionMask(newMask);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        const char* shapes[5]     = { "Sphere", "Cuboid", "Pyramid", "Capsule", "Hull" };
        int selectedIndex         = 0;
        const char* shape_current = collisionShape ? CollisionShapeTypeToString(collisionShape->GetType()) : "";
        int index                 = 0;
        for(auto& shape : shapes)
        {
            if(strcmp(shape, shape_current) == 0)
            {
                selectedIndex = index;
                break;
            }
            index++;
        }

        bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Collision Shape", shapes, 5, &selectedIndex);

        if(updated)
            phys.GetRigidBody()->SetCollisionShape(StringToCollisionShapeType(shapes[selectedIndex]));

        if(collisionShape)
        {
            switch(collisionShape->GetType())
            {
            case Lumos::CollisionShapeType::CollisionCuboid:
                CuboidCollisionShapeInspector(reinterpret_cast<Lumos::CuboidCollisionShape*>(collisionShape.get()), phys);
                break;
            case Lumos::CollisionShapeType::CollisionSphere:
                SphereCollisionShapeInspector(reinterpret_cast<Lumos::SphereCollisionShape*>(collisionShape.get()), phys);
                break;
            case Lumos::CollisionShapeType::CollisionPyramid:
                PyramidCollisionShapeInspector(reinterpret_cast<Lumos::PyramidCollisionShape*>(collisionShape.get()), phys);
                break;
            case Lumos::CollisionShapeType::CollisionCapsule:
                CapsuleCollisionShapeInspector(reinterpret_cast<Lumos::CapsuleCollisionShape*>(collisionShape.get()), phys);
                break;
            case Lumos::CollisionShapeType::CollisionHull:
                HullCollisionShapeInspector(reinterpret_cast<Lumos::HullCollisionShape*>(collisionShape.get()), phys);
                break;
            default:
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                LERROR("Unsupported Collision shape");
                break;
            }
        }
        else
        {
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
        }

        ImGui::PopItemWidth();
        ImGui::Columns(1);

        ImGui::Separator();
    }

    template <>
    void ComponentEditorWidget<Lumos::RigidBody2DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& phys = reg.get<Lumos::RigidBody2DComponent>(e);

        auto pos        = phys.GetRigidBody()->GetPosition();
        auto scale      = phys.GetRigidBody()->GetScale();
        auto angle      = phys.GetRigidBody()->GetAngle();
        auto friction   = phys.GetRigidBody()->GetFriction();
        auto isStatic   = phys.GetRigidBody()->GetIsStatic();
        auto isRest     = phys.GetRigidBody()->GetIsAtRest();
        auto damping    = phys.GetRigidBody()->GetDamping();
        auto elasticity = phys.GetRigidBody()->GetElasticity();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat2("##Position", &pos.x))
            phys.GetRigidBody()->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat2("##Scale", &scale.x))
            phys.GetRigidBody()->SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Orientation", &angle))
            phys.GetRigidBody()->SetOrientation(angle);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction))
            phys.GetRigidBody()->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Damping");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Damping", &damping))
            phys.GetRigidBody()->SetDamping(damping);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity))
            phys.GetRigidBody()->SetElasticity(elasticity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Static", &isStatic))
            phys.GetRigidBody()->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##At Rest", &isRest))
            phys.GetRigidBody()->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Shape Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* shapes[]      = { "Circle", "Square", "Custom" };
        std::string shape_current = CollisionShape2DTypeToString(phys.GetRigidBody()->GetShapeType());
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 3; n++)
            {
                bool is_selected = (shape_current.c_str() == shapes[n]);
                if(ImGui::Selectable(shapes[n], shape_current.c_str()))
                {
                    phys.GetRigidBody()->SetShape(StringToCollisionShape2DType(shapes[n]));
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        if(ImGui::Button("Rebuild"))
        {
            phys.GetRigidBody()->RebuildShape();
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::SoundComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& sound    = reg.get<Lumos::SoundComponent>(e);
        auto soundNode = sound.GetSoundNode();

        bool updated = false;

        auto pos               = soundNode->GetPosition();
        auto radius            = soundNode->GetRadius();
        auto paused            = soundNode->GetPaused();
        auto pitch             = soundNode->GetPitch();
        auto volume            = soundNode->GetVolume();
        auto referenceDistance = soundNode->GetReferenceDistance();
        auto rollOffFactor     = soundNode->GetRollOffFactor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(Lumos::ImGuiUtilities::Property("Position", pos))
        {
            soundNode->SetPosition(pos);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Radius", radius))
        {
            soundNode->SetRadius(radius);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Pitch", pitch))
        {
            soundNode->SetPitch(pitch);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Volume", volume))
        {
            soundNode->SetVolume(volume);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Reference Distance", referenceDistance))
        {
            soundNode->SetReferenceDistance(referenceDistance);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("RollOffFactor", rollOffFactor))
        {
            soundNode->SetRollOffFactor(paused);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Paused", paused))
        {
            soundNode->SetPaused(paused);
            updated = true;
        }

        ImGui::Separator();
        auto soundPointer = soundNode->GetSound();

        std::string path = "Empty Path";
        if(soundPointer)
            path = soundPointer->GetFilePath();

        Lumos::ImGuiUtilities::Property("File Path", path);

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if(payload != NULL && payload->IsDataType("AssetFile"))
        {
            auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
            if(Lumos::Editor::GetEditor()->IsAudioFile(filePath))
            {
                if(ImGui::BeginDragDropTarget())
                {
                    // Drop directly on to node and append to the end of it's children list.
                    if(ImGui::AcceptDragDropPayload("AssetFile"))
                    {
                        Lumos::String8 physicalPath;
                        Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
                        Lumos::FileSystem::Get().ResolvePhysicalPath(scratch.arena, Lumos::Str8StdS(filePath), &physicalPath);
                        auto newSound = Lumos::Sound::Create(std::string((const char*)physicalPath.str, physicalPath.size), Lumos::StringUtilities::GetFilePathExtension(std::string((const char*)physicalPath.str, physicalPath.size)));
                        Lumos::ArenaTempEnd(scratch);
                        soundNode->SetSound(newSound);
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }

        if(soundPointer)
        {
            int bitrate     = soundPointer->GetBitRate();
            float frequency = soundPointer->GetFrequency();
            int size        = soundPointer->GetSize();
            double length   = soundPointer->GetLength();
            int channels    = soundPointer->GetChannels();

            Lumos::ImGuiUtilities::Property("Bit Rate", bitrate, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Frequency", frequency, 1.0f, 0.0f, 0.0f, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Size", size, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Length", length, 0.0, 0.0, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Channels", channels, 0, 0, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

            if(updated)
                soundNode->SetSound(soundPointer);
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Camera>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& camera = reg.get<Lumos::Camera>(e);

        Lumos::ImGuiUtilities::ScopedStyle frameStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        using namespace Lumos;

        float aspect = camera.GetAspectRatio();
        if(ImGuiUtilities::Property("Aspect", aspect, 0.0f, 10.0f))
            camera.SetAspectRatio(aspect);

        float fov = camera.GetFOV();
        if(ImGuiUtilities::Property("Fov", fov, 1.0f, 120.0f))
            camera.SetFOV(fov);

        float n = camera.GetNear();
        if(ImGuiUtilities::Property("Near", n, 0.0f, 10.0f))
            camera.SetNear(n);

        float f = camera.GetFar();
        if(ImGuiUtilities::Property("Far", f, 10.0f, 10000.0f))
            camera.SetFar(f);

        float scale = camera.GetScale();
        if(ImGuiUtilities::Property("Scale", scale, 0.0f, 1000.0f))
            camera.SetScale(scale);

        bool ortho = camera.IsOrthographic();
        if(ImGuiUtilities::Property("Orthograhic", ortho))
            camera.SetIsOrthographic(ortho);

        float aperture = camera.GetAperture();
        if(ImGuiUtilities::Property("Aperture", aperture, 0.0f, 200.0f))
            camera.SetAperture(aperture);

        float shutterSpeed = camera.GetShutterSpeed();
        if(ImGuiUtilities::Property("Shutter Speed", shutterSpeed, 0.0f, 1.0f))
            camera.SetShutterSpeed(shutterSpeed);

        float sensitivity = camera.GetSensitivity();
        if(ImGuiUtilities::Property("ISO", sensitivity, 0.0f, 5000.0f))
            camera.SetSensitivity(sensitivity);

        float exposure = camera.GetExposure();
        ImGuiUtilities::Property("Exposure", exposure, 0.0f, 0.0f, 0.0f, ImGuiUtilities::PropertyFlag::ReadOnly);

        ImGui::Columns(1);
        ImGui::Separator();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Sprite>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();

        auto& sprite = reg.get<Lumos::Graphics::Sprite>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto pos = sprite.GetPosition();
        if(ImGui::InputFloat2("##Position", Maths::ValuePtr(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", Maths::ValuePtr(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", Maths::ValuePtr(colour)))
            sprite.SetColour(colour);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGuiUtilities::Property("Using Sprite Sheet", sprite.UsingSpriteSheet);
        ImGuiUtilities::Property("Tile Size X", sprite.SpriteSheetTileSizeX);
        ImGuiUtilities::Property("Tile Size Y", sprite.SpriteSheetTileSizeY);

        if(sprite.UsingSpriteSheet)
        {
            static Vec2 tileIndex;

            ImGuiUtilities::Property("Tile Index", tileIndex);
            ImGui::Columns(1);

            if(ImGui::Button("Set Sprite Sheet UV"))
                sprite.SetSpriteSheetIndex((int)tileIndex.x, (int)tileIndex.y);
            ImGui::Columns(2);
        }

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGui::AlignTextToFramePadding();
            auto tex = sprite.GetTexture();

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback               = std::bind(&Lumos::Graphics::Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex.get()), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
                    ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar(2);

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            if(tex)
            {
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        if(ImGui::Button("Convert to Animated Sprite"))
        {
            Lumos::Graphics::AnimatedSprite& animatedSprite = reg.get_or_emplace<Lumos::Graphics::AnimatedSprite>(e);
            animatedSprite.SetTexture(sprite.GetTexture());
            animatedSprite.SetPosition(sprite.GetPosition());

            reg.remove<Lumos::Graphics::Sprite>(e);
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::TextComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        auto& text = reg.get<Lumos::TextComponent>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGuiUtilities::PropertyMultiline("Text String", text.TextString);
        if(text.FontHandle)
        {
            Lumos::ImGuiUtilities::PropertyConst("FilePath", text.FontHandle->GetFilePath().c_str());
        }

        Lumos::ImGuiUtilities::Property("Colour", text.Colour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Outline Colour", text.OutlineColour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Outline Width", text.OutlineWidth);

        Lumos::ImGuiUtilities::Property("Line Spacing", text.LineSpacing);
        Lumos::ImGuiUtilities::Property("Max Width", text.MaxWidth);

        ImGui::Columns(1);

        auto callback = std::bind(&TextComponent::LoadFont, &text, std::placeholders::_1);

        ImGui::Separator();

        if(ImGui::Button("Load Font File"))
        {
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
        }

        ImGui::Separator();

        ImGui::Columns(2);

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGui::AlignTextToFramePadding();
            auto tex = text.FontHandle ? text.FontHandle->GetFontAtlas() : Lumos::Graphics::Font::GetDefaultFont()->GetFontAtlas();

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("Font")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex.get()), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), ImVec2(512, 512), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("Font"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsFontFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("Font"))
                        {
                            Application::Get().GetAssetManager()->AddAsset(Str8StdS(filePath), text.FontHandle);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar(2);

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
            if(tex)
            {
                ImGuiUtilities::Tooltip(tex->GetFilepath().c_str());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::AnimatedSprite>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        using namespace Lumos;
        using namespace Graphics;
        auto& sprite = reg.get<Lumos::Graphics::AnimatedSprite>(e);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto pos = sprite.GetPosition();
        if(ImGui::InputFloat2("##Position", Maths::ValuePtr(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", Maths::ValuePtr(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", Maths::ValuePtr(colour)))
            sprite.SetColour(colour);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        static bool byTile = true;
        uint32_t tileSizeX = sprite.SpriteSheetTileSizeX;
        uint32_t tileSizeY = sprite.SpriteSheetTileSizeY;

        Lumos::ImGuiUtilities::Property("By Tile", byTile);
        if(Lumos::ImGuiUtilities::Property("Tile Size X", tileSizeX))
            sprite.SpriteSheetTileSizeX = tileSizeX;
        if(Lumos::ImGuiUtilities::Property("Tile Size Y", tileSizeY))
            sprite.SpriteSheetTileSizeY = tileSizeY;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Current State");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        {
            std::vector<std::string> states;
            auto& animStates = sprite.GetAnimationStates();

            if(animStates.empty())
            {
                ImGui::TextUnformatted("No States Available");
            }
            else
            {
                for(auto& [name, frame] : animStates)
                {
                    states.push_back(name);
                }

                std::string currentStateName = sprite.GetState();
                if(ImGui::BeginCombo("##FrameSelect", currentStateName.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
                {
                    for(int n = 0; n < animStates.size(); n++)
                    {
                        bool is_selected = (currentStateName.c_str() == states[n].c_str());
                        if(ImGui::Selectable(states[n].c_str(), currentStateName.c_str()))
                        {
                            sprite.SetState(states[n]);
                        }
                        if(is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        auto& animStates = sprite.GetAnimationStates();

        if(ImGui::TreeNode("Helper"))
        {
            static std::string newStateName = "NewState";

            ImGuiUtilities::InputText(newStateName, "##NewStateName");

            static Vec2 StartFrame;
            static Vec2 EndFrame;
            static int Count = 0;

            ImGui::DragFloat2("##Position", Maths::ValuePtr(StartFrame));
            ImGui::DragFloat2("##EndFrame", Maths::ValuePtr(EndFrame));

            if(ImGui::Button("Create State"))
            {
                Graphics::AnimatedSprite::AnimationState state;
                state.Frames        = {};
                state.FrameDuration = 1.0f;
                state.Mode          = Graphics::AnimatedSprite::PlayMode::Loop;

                Vec2 Current = StartFrame;
                Current.x *= (float)tileSizeX;
                Current.y *= (float)tileSizeY;
                for(int i = 0; i < EndFrame.x; i++)
                {
                    Current.x = StartFrame.x + i * tileSizeX;
                    state.Frames.push_back(Current);
                }

                if(EndFrame.y > StartFrame.y)
                {
                    for(int i = 0; i < EndFrame.y; i++)
                    {
                        Current.y = StartFrame.y + i * tileSizeY;
                        state.Frames.push_back(Current);
                    }
                }
                animStates[newStateName] = state;
            }

            ImGui::TreePop();
        }

        if(ImGui::TreeNode("States"))
        {
            // ImGui::Indent(20.0f);
            ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_PLUS))
            {
                Graphics::AnimatedSprite::AnimationState state;
                state.Frames          = {};
                state.FrameDuration   = 1.0f;
                state.Mode            = Graphics::AnimatedSprite::PlayMode::Loop;
                animStates["--New--"] = state;
            }

            ImGuiUtilities::Tooltip("Add New State");

            ImGui::PopStyleColor();

            ImGui::Separator();

            int frameID = 0;

            std::vector<std::string> statesToDelete;
            std::vector<std::pair<std::string, std::string>> statesToRename;

            for(auto& [name, state] : animStates)
            {
                ImGui::PushID(frameID);
                bool open = ImGui::TreeNode(&state, "%s", name.c_str());

                ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                if(ImGui::Button((ICON_MDI_MINUS "##" + name).c_str()))
                    ImGui::OpenPopup(("##" + name).c_str());

                ImGuiUtilities::Tooltip("Remove State");

                ImGui::PopStyleColor();

                if(ImGui::BeginPopup(("##" + name).c_str(), 3))
                {
                    if(ImGui::Button(("Remove##" + name).c_str()))
                    {
                        statesToDelete.push_back(name);
                    }
                    ImGui::EndPopup();
                }

                if(open)
                {
                    ImGui::Columns(2);

                    ImGui::AlignTextToFramePadding();

                    ImGui::TextUnformatted("Name");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static char objName[INPUT_BUF_SIZE];
                    strcpy(objName, name.c_str());
                    ImGui::PushItemWidth(-1);

                    bool renameState = false;
                    std::string newName;

                    if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
                    {
                        renameState = true;
                        newName     = objName;
                    }

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Duration");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    ImGui::DragFloat("##Duration", &state.FrameDuration);

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("PlayMode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    const char* modeTypes[]  = { "Loop", "Ping Pong" };
                    std::string mode_current = state.Mode == Graphics::AnimatedSprite::PlayMode::Loop ? "Loop" : "PingPong";
                    if(ImGui::BeginCombo("##ModeSelect", mode_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
                    {
                        for(int n = 0; n < 2; n++)
                        {
                            bool is_selected = (mode_current.c_str() == modeTypes[n]);
                            if(ImGui::Selectable(modeTypes[n], mode_current.c_str()))
                            {
                                state.Mode = n == 0 ? Graphics::AnimatedSprite::PlayMode::Loop : Graphics::AnimatedSprite::PlayMode::PingPong;
                            }
                            if(is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Columns(1);
                    if(ImGui::TreeNode("Frames"))
                    {
                        ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                        std::vector<Vec2>& frames = state.Frames;

                        if(ImGui::Button(ICON_MDI_PLUS))
                        {
                            frames.emplace_back(0.0f, 0.0f);
                        }

                        ImGui::PopStyleColor();

                        auto begin = frames.begin();
                        auto end   = frames.end();

                        static int numRemoved = 0;
                        for(auto it = begin; it != end; ++it)
                        {
                            auto& pos = (*it);
                            ImGui::PushID(&pos + numRemoved * 100);
                            ImGui::PushItemWidth((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize() * 3.0f);

                            if(byTile)
                            {
                                pos.x /= (float)tileSizeX;
                                pos.y /= (float)tileSizeY;
                            }

                            ImGui::DragFloat2("##Position", Maths::ValuePtr(pos));

                            if(byTile)
                            {
                                pos.x *= (float)tileSizeX;
                                pos.y *= (float)tileSizeY;
                            }

                            ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                            if(ImGui::Button(ICON_MDI_MINUS))
                                ImGui::OpenPopup("Remove");

                            ImGuiUtilities::Tooltip("Remove");

                            ImGui::PopStyleColor();

                            if(ImGui::BeginPopup("Remove", 3))
                            {
                                if(ImGui::Button("Remove"))
                                {
                                    frames.erase(it);
                                    numRemoved++;
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    if(renameState)
                    {
                        statesToRename.emplace_back(name, newName);
                    }

                    frameID++;

                    ImGui::Separator();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            for(auto& stateName : statesToDelete)
            {
                animStates.erase(stateName);
            }

            for(auto& statePair : statesToRename)
            {
                auto nodeHandler  = animStates.extract(statePair.first);
                nodeHandler.key() = statePair.second;
                animStates.insert(std::move(nodeHandler));
            }

            ImGui::Unindent(20.0f);
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            // ImGui::AlignTextToFramePadding();
            auto tex = sprite.GetTexture();

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback               = std::bind(&Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex.get()), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar(2);

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
            if(tex)
            {
                ImGuiUtilities::Tooltip(tex->GetFilepath().c_str());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Light>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& light = reg.get<Lumos::Graphics::Light>(e);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(light.Type != 0)
            Lumos::ImGuiUtilities::Property("Position", light.Position);

        if(light.Type != 2)
            Lumos::ImGuiUtilities::Property("Direction", light.Direction);

        if(light.Type != 0)
            Lumos::ImGuiUtilities::Property("Radius", light.Radius, 0.0f, 100.0f);
        Lumos::ImGuiUtilities::Property("Colour", light.Colour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Intensity", light.Intensity, 0.0f, 4.0f);

        if(light.Type == 1)
            Lumos::ImGuiUtilities::Property("Angle", light.Angle, -1.0f, 1.0f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Light Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* types[]       = { "Directional", "Spot", "Point" };
        std::string light_current = Lumos::Graphics::Light::LightTypeToString(Lumos::Graphics::LightType(int(light.Type)));
        if(ImGui::BeginCombo("", light_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 3; n++)
            {
                bool is_selected = (light_current.c_str() == types[n]);
                if(ImGui::Selectable(types[n], light_current.c_str()))
                {
                    light.Type = Lumos::Graphics::Light::StringToLightType(types[n]);
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    Lumos::Graphics::PrimitiveType GetPrimativeName(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Cube")
        {
            return Lumos::Graphics::PrimitiveType::Cube;
        }
        else if(type == "Quad")
        {
            return Lumos::Graphics::PrimitiveType::Quad;
        }
        else if(type == "Sphere")
        {
            return Lumos::Graphics::PrimitiveType::Sphere;
        }
        else if(type == "Pyramid")
        {
            return Lumos::Graphics::PrimitiveType::Pyramid;
        }
        else if(type == "Capsule")
        {
            return Lumos::Graphics::PrimitiveType::Capsule;
        }
        else if(type == "Cylinder")
        {
            return Lumos::Graphics::PrimitiveType::Cylinder;
        }
        else if(type == "Terrain")
        {
            return Lumos::Graphics::PrimitiveType::Terrain;
        }

        LERROR("Primitive not supported");
        return Lumos::Graphics::PrimitiveType::Cube;
    };

    std::string GetPrimativeName(Lumos::Graphics::PrimitiveType type)
    {
        LUMOS_PROFILE_FUNCTION();
        switch(type)
        {
        case Lumos::Graphics::PrimitiveType::Cube:
            return "Cube";
        case Lumos::Graphics::PrimitiveType::Plane:
            return "Plane";
        case Lumos::Graphics::PrimitiveType::Quad:
            return "Quad";
        case Lumos::Graphics::PrimitiveType::Sphere:
            return "Sphere";
        case Lumos::Graphics::PrimitiveType::Pyramid:
            return "Pyramid";
        case Lumos::Graphics::PrimitiveType::Capsule:
            return "Capsule";
        case Lumos::Graphics::PrimitiveType::Cylinder:
            return "Cylinder";
        case Lumos::Graphics::PrimitiveType::Terrain:
            return "Terrain";
        case Lumos::Graphics::PrimitiveType::File:
            return "File";
        case Lumos::Graphics::PrimitiveType::None:
            return "None";
        }

        LERROR("Primitive not supported");
        return "";
    };

    void TextureWidget(const char* label, Lumos::Graphics::Material* material, Lumos::Graphics::Texture2D* tex, bool flipImage, float& usingMapProperty, Lumos::Vec4& colourProperty, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64), bool defaultOpen = false)
    {
        using namespace Lumos;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
        if(defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        if(ImGui::TreeNodeEx(label, flags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            // ImGui::Columns(2);
            ImGui::BeginColumns("TextureWidget", 2, ImGuiOldColumnFlags_NoResize);
            ImGui::SetColumnWidth(0, imageButtonSize.x + 10.0f);

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(tex->GetFilepath().c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize * 3.0f, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar();

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            // ImGui::PushItemWidth(-1);

            if(tex)
            {
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            // ImGui::TextUnformatted("Use Map");
            // ImGui::SameLine();
            // ImGui::PushItemWidth(-1);

            ImGui::SliderFloat(Lumos::ImGuiUtilities::GenerateLabelID("Use Map"), &usingMapProperty, 0.0f, 1.0f);

            ImGui::ColorEdit4(Lumos::ImGuiUtilities::GenerateLabelID("Colour"), Maths::ValuePtr(colourProperty));
            /*       ImGui::TextUnformatted("Value");
                   ImGui::SameLine();
                   ImGui::PushItemWidth(-1);*/

            // ImGui::DragFloat(Lumos::ImGuiUtilities::GenerateID(), &amount, 0.0f, 20.0f);

            // ImGui::PopItemWidth();
            // ImGui::NextColumn();

            // ImGuiUtilities::Property("Use Map", usingMapProperty, 0.0f, 1.0f);
            // ImGuiUtilities::Property("Colour", colourProperty, 0.0f, 1.0f, false, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);

            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    void TextureWidget(const char* label, Lumos::Graphics::Material* material, Lumos::Graphics::Texture2D* tex, bool flipImage, float& usingMapProperty, float& amount, bool hasAmountValue, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64), bool defaultOpen = false)
    {
        using namespace Lumos;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
        if(defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        if(ImGui::TreeNodeEx(label, flags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::BeginColumns("TextureWidget", 2, ImGuiOldColumnFlags_NoResize);
            ImGui::SetColumnWidth(0, imageButtonSize.x + 10.0f);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(tex->GetFilepath().c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize * 3.0f, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar();

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if(tex)
            {
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }
            ImGui::PopItemWidth();
            /*      ImGui::TextUnformatted("Use Map");
                  ImGui::SameLine();
                  ImGui::PushItemWidth(-1);*/

            // ImGui::DragFloat(Lumos::ImGuiUtilities::GenerateID(), &usingMapProperty, 0.0f, 1.0f);
            ImGui::SliderFloat(Lumos::ImGuiUtilities::GenerateLabelID("Use Map"), &usingMapProperty, 0.0f, 1.0f);

            if(hasAmountValue)
            {
                float maxValue = 20.0f;
                if(std::strcmp(label, "Metallic") == 0 || std::strcmp(label, "Roughness") == 0)
                    maxValue = 1.0f;
                ImGui::SliderFloat(Lumos::ImGuiUtilities::GenerateLabelID("Value"), &amount, 0.0f, maxValue);
            }
            // ImGui::TextUnformatted("Value");
            // ImGui::SameLine();
            // ImGui::PushItemWidth(-1);

            // ImGui::DragFloat(Lumos::ImGuiUtilities::GenerateID(), &amount, 0.0f, 20.0f);

            // ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::ModelComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& model = *reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef.get();

        auto primitiveType = reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef ? model.GetPrimitiveType() : Lumos::Graphics::PrimitiveType::None;

        Lumos::ImGuiUtilities::PushID();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("Primitive Type");

        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* shapes[]      = { "Sphere", "Cube", "Pyramid", "Capsule", "Cylinder", "Terrain", "File", "Quad", "None" };
        std::string shape_current = GetPrimativeName(primitiveType).c_str();
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 8; n++)
            {
                bool is_selected = (shape_current.c_str() == shapes[n]);
                if(ImGui::Selectable(shapes[n], shape_current.c_str()))
                {
                    if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                        model.GetMeshesRef().Clear();

                    if(strcmp(shapes[n], "File") != 0)
                    {
                        if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                        {
                            model.GetMeshesRef().PushBack(Lumos::SharedPtr<Lumos::Graphics::Mesh>(Lumos::Graphics::CreatePrimative(GetPrimativeName(shapes[n]))));
                            model.SetPrimitiveType(GetPrimativeName(shapes[n]));
                        }
                        else
                        {
                            reg.get<Lumos::Graphics::ModelComponent>(e).LoadPrimitive(GetPrimativeName(shapes[n]));
                        }
                    }
                    else
                    {
                        if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                            model.SetPrimitiveType(Lumos::Graphics::PrimitiveType::File);
                    }
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        if(primitiveType == Lumos::Graphics::PrimitiveType::File)
        {
            ImGui::TextUnformatted("FilePath");

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(model.GetFilePath().c_str());
            Lumos::ImGuiUtilities::Tooltip(model.GetFilePath().c_str());

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        int matIndex = 0;

        auto modelRef = reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef;
        if(!modelRef)
        {
            Lumos::ImGuiUtilities::PopID();
            return;
        }
        using namespace Lumos;
        ImGui::Separator();
        const auto& meshes = modelRef->GetMeshes();
        ImGui::Indent();
        if(ImGui::TreeNodeEx("Meshes", ImGuiTreeNodeFlags_Framed))
        {
            int MeshIndex = 0;

            for(auto mesh : meshes)
            {
                String8 meshName = !mesh->GetName().empty() ? Str8StdS(mesh->GetName()) : PushStr8F(Application::Get().GetFrameArena(), "Mesh %i", MeshIndex);

                if(ImGui::TreeNodeEx((const char*)meshName.str, ImGuiTreeNodeFlags_Framed))
                {
                    auto stats = mesh->GetStats();

                    ImGui::Indent();
                    ImGui::Columns(2);

                    Lumos::ImGuiUtilities::Property("Triangle Count", stats.TriangleCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                    Lumos::ImGuiUtilities::Property("Vertex Count", stats.VertexCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                    Lumos::ImGuiUtilities::Property("Index Count", stats.IndexCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                    Lumos::ImGuiUtilities::Property("Optimise Threshold", stats.OptimiseThreshold, 0.0f, 0.0f, 0.0f, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                    Lumos::ImGuiUtilities::PropertyConst("Material", mesh->GetMaterial() ? (mesh->GetMaterial()->GetName().empty() ? "No Name" : mesh->GetMaterial()->GetName().c_str()) : "Empty");

                    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                    auto callback               = std::bind(&Lumos::Graphics::Mesh::SetAndLoadMaterial, mesh, std::placeholders::_1);
                    if(payload != NULL && payload->IsDataType("AssetFile"))
                    {
                        auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                        if(Lumos::Editor::GetEditor()->IsMaterialFile(filePath))
                        {
                            if(ImGui::BeginDragDropTarget())
                            {
                                // Drop directly on to node and append to the end of it's children list.
                                if(ImGui::AcceptDragDropPayload("AssetFile"))
                                {
                                    callback(filePath);
                                    ImGui::EndDragDropTarget();

                                    ImGui::Columns(1);
                                    ImGui::TreePop();
                                    ImGui::TreePop();
                                    ImGui::TreePop();
                                    return;
                                }

                                ImGui::EndDragDropTarget();
                            }
                        }
                    }

                    ImGui::Columns(1);

                    ImGui::Unindent();
                    ImGui::TreePop();
                }

                MeshIndex++;
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed))
        {
            Lumos::Graphics::Material* MaterialShown[1000];
            uint32_t MaterialCount = 0;
            for(auto mesh : meshes)
            {
                auto material       = mesh->GetMaterial();
                std::string matName = material ? material->GetName() : "";

                bool materialFound = false;
                for(uint32_t i = 0; i < MaterialCount; i++)
                {
                    if(MaterialShown[i] == material.get())
                        materialFound = true;
                }

                if(materialFound)
                    continue;

                MaterialShown[MaterialCount++] = material.get();

                if(matName.empty())
                {
                    matName = "Material";
                    matName += std::to_string(matIndex);
                }

                matIndex++;
                ImGui::Indent();

                static bool materialNameUpdated = false;
                static std::string renamedMaterialName;
                if(materialNameUpdated)
                {
                    if(matName == renamedMaterialName)
                    {
                        materialNameUpdated = false;
                        ImGui::SetNextItemOpen(true);
                    }
                }
                if(!material)
                {
                    ImGui::TextUnformatted("Empty Material");
                    if(ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                        mesh->SetMaterial(Lumos::CreateSharedPtr<Lumos::Graphics::Material>());
                }
                else if(ImGui::TreeNodeEx((void*)(intptr_t)material.get(), ImGuiTreeNodeFlags_Framed, matName.c_str()))
                {
                    using namespace Lumos;
                    ImGui::Indent();
                    // Lumos::ImGuiUtilities::PushID();
                    ImGui::PushID((int)(uintptr_t)material.get());
                    if(ImGui::Button("Save to file"))
                    {

                        Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
                        Lumos::String8 filePath  = Lumos::Str8Lit("//Assets/Materials/"); // +matName + ".lmat";
                        Lumos::String8 physicalPath;
                        if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, filePath, &physicalPath, true))
                        {
                            Lumos::String8 fullPath = Lumos::PushStr8F(scratch.arena, "%s%s.lmat", (char*)physicalPath.str, matName.c_str());
                            std::stringstream storage;
                            {
                                cereal::JSONOutputArchive output { storage };
                                Lumos::Graphics::save(output, *material.get());
                            }

                            FileSystem::WriteTextFile(fullPath, Str8StdS(storage.str()));
                        }
                        Lumos::ArenaTempEnd(scratch);
                    }

                    if(Lumos::ImGuiUtilities::InputText(matName, "##materialName"))
                    {
                        materialNameUpdated = true;
                        renamedMaterialName = matName;
                        material->SetName(matName);
                    }

                    bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

                    bool twoSided     = material->GetFlag(Lumos::Graphics::Material::RenderFlags::TWOSIDED);
                    bool depthTested  = material->GetFlag(Lumos::Graphics::Material::RenderFlags::DEPTHTEST);
                    bool alphaBlended = material->GetFlag(Lumos::Graphics::Material::RenderFlags::ALPHABLEND);
                    bool castShadows  = !material->GetFlag(Lumos::Graphics::Material::RenderFlags::NOSHADOW);

                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();

                    if(ImGuiUtilities::Property("Alpha Blended", alphaBlended))
                        material->SetFlag(Lumos::Graphics::Material::RenderFlags::ALPHABLEND, alphaBlended);

                    if(ImGuiUtilities::Property("Two Sided", twoSided))
                        material->SetFlag(Lumos::Graphics::Material::RenderFlags::TWOSIDED, twoSided);

                    if(ImGuiUtilities::Property("Depth Tested", depthTested))
                        material->SetFlag(Lumos::Graphics::Material::RenderFlags::DEPTHTEST, depthTested);

                    if(ImGuiUtilities::Property("Cast Shadows", castShadows))
                        material->SetFlag(Lumos::Graphics::Material::RenderFlags::NOSHADOW, !castShadows);

                    ImGuiUtilities::Property("Alpha Cutoff", material->GetProperties()->alphaCutoff, 0.0f, 1.0f, 0.1f);

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("WorkFlow");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    int workFlow = (int)material->GetProperties()->workflow;

                    const char* workFlows[]     = { "Separate Textures", "Metallic Roughness", "Specular Glossiness" };
                    const char* workFlowCurrent = workFlows[workFlow];
                    if(ImGui::BeginCombo("##WorkflowCombo", workFlowCurrent, 0)) // The second parameter is the label previewed before opening the combo.
                    {
                        for(int n = 0; n < 3; n++)
                        {
                            bool is_selected = (workFlow == n);
                            if(ImGui::Selectable(workFlows[n], workFlowCurrent))
                            {
                                workFlow                            = n;
                                material->GetProperties()->workflow = (float)workFlow;
                            }
                            if(is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);

                    Graphics::MaterialProperties* prop = material->GetProperties();
                    auto colour                        = Vec4();
                    float normal                       = 0.0f;
                    auto& textures                     = material->GetTextures();
                    Vec2 textureSize                   = Vec2(100.0f, 100.0f);
                    TextureWidget("Albedo", material.get(), textures.albedo.get(), flipImage, prop->albedoMapFactor, prop->albedoColour, std::bind(&Graphics::Material::SetAlbedoTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI(), true);

                    TextureWidget("Normal", material.get(), textures.normal.get(), flipImage, prop->normalMapFactor, normal, false, std::bind(&Graphics::Material::SetNormalTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI());

                    TextureWidget("Metallic", material.get(), textures.metallic.get(), flipImage, prop->metallicMapFactor, prop->metallic, true, std::bind(&Graphics::Material::SetMetallicTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI());

                    TextureWidget("Roughness", material.get(), textures.roughness.get(), flipImage, prop->roughnessMapFactor, prop->roughness, true, std::bind(&Graphics::Material::SetRoughnessTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI());

                    if(ImGui::TreeNodeEx("Reflectance", ImGuiTreeNodeFlags_Framed))
                    {
                        ImGui::SliderFloat("##Reflectance", &prop->reflectance, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }

                    TextureWidget("AO", material.get(), textures.ao.get(), flipImage, prop->occlusionMapFactor, normal, false, std::bind(&Graphics::Material::SetAOTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI());

                    TextureWidget("Emissive", material.get(), textures.emissive.get(), flipImage, prop->emissiveMapFactor, prop->emissive, true, std::bind(&Graphics::Material::SetEmissiveTexture, material, std::placeholders::_1), textureSize * Application::Get().GetWindowDPI());

                    material->SetMaterialProperites(*prop);
                    ImGui::Unindent();
                    ImGui::TreePop();
                    // Lumos::ImGuiUtilities::PopID();
                    ImGui::PopID();
                }
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }

        if(modelRef->GetSkeleton())
            if(ImGui::TreeNodeEx("Animations", ImGuiTreeNodeFlags_Framed))
            {
                ImGui::Indent();
                ImGui::Columns(2);

                int jointCount = modelRef->GetSkeleton()->GetSkeleton().num_joints();
                Lumos::ImGuiUtilities::Property("Joint Count", jointCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

                const auto& animations = modelRef->GetAnimations();
                int animCount          = (int)animations.Size();
                Lumos::ImGuiUtilities::Property("Animation Count", animCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

                if(animCount > 0)
                {
                    ImGui::Columns(1);
                    static bool testPlayAnimation = false;
                    if(Application::Get().GetEditorState() == EditorState::Preview)
                    {
                        if(!testPlayAnimation && ImGui::Button("Play Animation"))
                        {
                            testPlayAnimation = true;
                        }

                        if(testPlayAnimation && ImGui::Button("Stop Animation"))
                        {
                            testPlayAnimation = false;
                        }

                        if(!testPlayAnimation)
                        {
                            static float animTime = 0.0f;
                            if(ImGui::SliderFloat("Animation Preview", &animTime, 0.0f, 1.0f))
                            {
                                modelRef->UpdateAnimation(Engine::GetTimeStep(), animTime);
                            }
                        }

                        if(testPlayAnimation)
                            modelRef->UpdateAnimation(Engine::GetTimeStep());
                    }
                    else
                    {
                        testPlayAnimation = false;
                    }

                    ImGui::Columns(2);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Current Animation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    TDArray<const char*> animNames(Application::Get().GetFrameArena());
                    animNames.Reserve(animCount);

                    for(auto& anim : animations)
                    {
                        animNames.PushBack(anim->GetName().c_str());
                    }

                    uint32_t currentIndex   = modelRef->GetCurrentAnimationIndex();
                    const char* currentAnim = animNames[currentIndex];
                    if(ImGui::BeginCombo("##AnimationCombo", currentAnim, 0)) // The second parameter is the label previewed before opening the combo.
                    {
                        for(int n = 0; n < animNames.Size(); n++)
                        {
                            bool is_selected = (currentIndex == n);
                            if(ImGui::Selectable(animNames[n], currentAnim))
                            {
                                currentIndex = n;
                                modelRef->SetCurrentAnimationIndex(n);
                            }
                            if(is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::TreePop();
            }
        ImGui::Unindent();

        Lumos::ImGuiUtilities::PopID();
        ImGui::Columns(1);
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Environment>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& environment = reg.get<Lumos::Graphics::Environment>(e);
        // Disable image until texturecube is supported
        // Lumos::ImGuiUtilities::Image(environment.GetEnvironmentMap(), Vec2(200, 200));

        uint8_t mode       = environment.GetMode();
        Lumos::Vec4 params = environment.GetParameters();
        ImGui::PushItemWidth(-1);

        const char* modes[]      = { "Textures", "Preetham", "Generic" };
        const char* mode_current = modes[mode];
        if(ImGui::BeginCombo("", mode_current, 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 3; n++)
            {
                bool is_selected = (mode_current == modes[n]);
                if(ImGui::Selectable(modes[n], mode_current))
                {
                    environment.SetMode(n);
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(mode == 0)
        {
            ImGui::TextUnformatted("File Path");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static char filePath[INPUT_BUF_SIZE];
            strcpy(filePath, environment.GetFilePath().c_str());

            if(ImGui::InputText("##filePath", filePath, IM_ARRAYSIZE(filePath), 0))
            {
                environment.SetFilePath(filePath);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("File Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static char fileType[INPUT_BUF_SIZE];
            strcpy(fileType, environment.GetFileType().c_str());

            if(ImGui::InputText("##fileType", fileType, IM_ARRAYSIZE(fileType), 0))
            {
                environment.SetFileType(fileType);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Width");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int width = environment.GetWidth();

            if(ImGui::DragInt("##Width", &width))
            {
                environment.SetWidth(width);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int height = environment.GetHeight();

            if(ImGui::DragInt("##Height", &height))
            {
                environment.SetHeight(height);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Num Mips");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int numMips = environment.GetNumMips();
            if(ImGui::InputInt("##NumMips", &numMips))
            {
                environment.SetNumMips(numMips);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }
        else if(mode == 1)
        {
            bool valueUpdated = false;
            valueUpdated |= Lumos::ImGuiUtilities::Property("Turbidity", params.x, 1.7f, 100.0f, 0.01f);
            valueUpdated |= Lumos::ImGuiUtilities::Property("Azimuth", params.y, -1000.0f, 1000.f, 0.01f);
            valueUpdated |= Lumos::ImGuiUtilities::Property("Inclination", params.z, -1000.0f, 1000.f, 0.01f);

            if(valueUpdated)
                environment.SetParameters(params);
        }

        ImGui::Columns(1);
        if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0)))
            environment.Load();

        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::TextureMatrixComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        using namespace Lumos;
        auto& textureMatrix = reg.get<Lumos::TextureMatrixComponent>(e);
        Mat4& mat           = textureMatrix.GetMatrix();

        Vec3 position;
        Vec3 scale;
        Quat rotation;
        mat.Decompose(position, rotation, scale);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", Maths::ValuePtr(position)))
        {
            Lumos::Maths::SetTranslation(mat, position);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Rotation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Rotation", Maths::ValuePtr(rotation)))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch       = Lumos::Maths::Max(pitch, -89.9f);
            Lumos::Maths::SetRotation(mat, Vec3(pitch, rotation.y, rotation.z));
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Scale", Maths::ValuePtr(scale), 0.1f))
        {
            Lumos::Maths::SetScale(mat, scale);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::DefaultCameraController>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& controllerComp = reg.get<Lumos::DefaultCameraController>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Controller Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* controllerTypes[] = { "Editor", "FPS", "ThirdPerson", "2D", "Custom" };
        std::string currentController = Lumos::DefaultCameraController::CameraControllerTypeToString(controllerComp.GetType());
        if(ImGui::BeginCombo("", currentController.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 5; n++)
            {
                bool is_selected = (currentController.c_str() == controllerTypes[n]);
                if(ImGui::Selectable(controllerTypes[n], currentController.c_str()))
                {
                    controllerComp.SetControllerType(Lumos::DefaultCameraController::StringToControllerType(controllerTypes[n]));
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if(controllerComp.GetController())
            controllerComp.GetController()->OnImGui();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::ParticleEmitter>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        Lumos::ParticleEmitter& emitter = reg.get<Lumos::ParticleEmitter>(e);

        PropertySet("Max Particles", emitter.GetParticleCount, emitter.SetParticleCount);

        PropertySet("Initial Size", emitter.GetParticleSize, emitter.SetParticleSize);
        PropertySet("Initial LifeTime", emitter.GetParticleLife, emitter.SetParticleLife);
        PropertySet("Initial InitialVelocity", emitter.GetInitialVelocity, emitter.SetInitialVelocity);
        PropertySet("Gravity", emitter.GetGravity, emitter.SetGravity);

        auto colour = emitter.GetInitialColour();
        if(Lumos::ImGuiUtilities::Property("Initial InitialColour", colour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty))
        {
            emitter.SetInitialColour(colour);
        }

        PropertySet("Position Spread", emitter.GetSpread, emitter.SetSpread);
        PropertySet("Velocity Spread", emitter.GetVelocitySpread, emitter.SetVelocitySpread);
        PropertySet("Life Spread", emitter.GetLifeSpread, emitter.SetLifeSpread);

        PropertySet("Fade In", emitter.GetFadeIn, emitter.SetFadeIn);
        PropertySet("Fade Out", emitter.GetFadeOut, emitter.SetFadeOut);
        PropertySet("Particle Rate", emitter.GetParticleRate, emitter.SetParticleRate);
        PropertySet("Launch Particles", emitter.GetNumLaunchParticles, emitter.SetNumLaunchParticles);
        PropertySet("Sort Particles", emitter.GetSortParticles, emitter.SetSortParticles);
        PropertySet("Depth Write", emitter.GetDepthWrite, emitter.SetDepthWrite);

        Lumos::ParticleEmitter::BlendType blendtype = emitter.GetBlendType();
        static const char* possibleBlendTypes[3]    = { "Additive", "Alpha", "Off" };

        int selectedIndex = (int)blendtype;

        bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Blend Type", possibleBlendTypes, 3, &selectedIndex);
        if(updated)
            emitter.SetBlendType((Lumos::ParticleEmitter::BlendType)selectedIndex);

        Lumos::ParticleEmitter::AlignedType alignType = emitter.GetAlignedType();
        static const char* possibleAlignTypes[3]      = { "2D", "3D", "Off" };

        int selectedIndexAlign = (int)alignType;

        updated = Lumos::ImGuiUtilities::PropertyDropdown("Align Type", possibleAlignTypes, 3, &selectedIndexAlign);
        if(updated)
            emitter.SetAlignedType((Lumos::ParticleEmitter::AlignedType)selectedIndexAlign);

        PropertySet("Is Animated", emitter.GetIsAnimated, emitter.SetIsAnimated);
        if(emitter.GetIsAnimated())
        {
            uint32_t numTextureRows = emitter.GetAnimatedTextureRows();
            if(Lumos::ImGuiUtilities::Property("Animated Texture Rows", numTextureRows))
            {
                emitter.SetAnimatedTextureRows((uint8_t)numTextureRows);
            }
        }

        using namespace Lumos;
        ImGui::AlignTextToFramePadding();
        auto& tex      = emitter.GetTexture();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

        auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
        auto callback               = std::bind(&Lumos::ParticleEmitter::SetTextureFromFile, &emitter, std::placeholders::_1);
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
        auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

        bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
        bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
        if(tex && showTexture)
        {
            if(ImGui::ImageButton((const char*)(tex.get()), reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
            {
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
            }

            if(ImGui::IsItemHovered() && tex)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
                ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(tex)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                ImGui::EndTooltip();
            }
        }
        else
        {
            if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
            {
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
            }
        }

        if(payload != NULL && payload->IsDataType("AssetFile"))
        {
            auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
            if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
            {
                if(ImGui::BeginDragDropTarget())
                {
                    // Drop directly on to node and append to the end of it's children list.
                    if(ImGui::AcceptDragDropPayload("AssetFile"))
                    {
                        callback(filePath);
                        ImGui::EndDragDropTarget();

                        ImGui::Columns(1);
                        ImGui::Separator();
                        ImGui::PopStyleVar(1);
                        return;
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }

        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if(tex)
        {
            ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
            ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Listener>(entt::registry& reg, entt::registry::entity_type e)
    {
    }
}

namespace Lumos
{
    InspectorPanel::InspectorPanel()
    {
        m_Name       = ICON_MDI_INFORMATION " Inspector###inspector";
        m_SimpleName = "Inspector";
    }

    static bool init = false;
    void InspectorPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(init)
            return;

        init = true;

        auto& registry = scene->GetRegistry();
        auto& iconMap  = m_Editor->GetComponentIconMap();

#define TRIVIAL_COMPONENT(ComponentType, ComponentName)                      \
    {                                                                        \
        std::string Name;                                                    \
        if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
            Name += iconMap[typeid(ComponentType).hash_code()];              \
        else                                                                 \
            Name += iconMap[typeid(Editor).hash_code()];                     \
        Name += "\t";                                                        \
        Name += (ComponentName);                                             \
        m_EnttEditor.registerComponent<ComponentType>(Name.c_str());         \
    }
        TRIVIAL_COMPONENT(Maths::Transform, "Transform");
        // TRIVIAL_COMPONENT(Graphics::Model, "Model");
        TRIVIAL_COMPONENT(Graphics::ModelComponent, "ModelComponent");
        TRIVIAL_COMPONENT(Camera, "Camera");
        TRIVIAL_COMPONENT(AxisConstraintComponent, "AxisConstraint");
        TRIVIAL_COMPONENT(SpringConstraintComponent, "SpringConstraint");
        TRIVIAL_COMPONENT(RigidBody3DComponent, "Physics3D");
        TRIVIAL_COMPONENT(RigidBody2DComponent, "Physics2D");
        TRIVIAL_COMPONENT(SoundComponent, "Sound");
        TRIVIAL_COMPONENT(Graphics::AnimatedSprite, "Animated Sprite");
        TRIVIAL_COMPONENT(Graphics::Sprite, "Sprite");
        TRIVIAL_COMPONENT(Graphics::Light, "Light");
        TRIVIAL_COMPONENT(LuaScriptComponent, "LuaScript");
        TRIVIAL_COMPONENT(Graphics::Environment, "Environment");
        TRIVIAL_COMPONENT(TextureMatrixComponent, "Texture Matrix");
        TRIVIAL_COMPONENT(DefaultCameraController, "Default Camera Controller");
        TRIVIAL_COMPONENT(Listener, "Listener");
        TRIVIAL_COMPONENT(TextComponent, "Text");
        TRIVIAL_COMPONENT(ParticleEmitter, "ParticleEmitter");
    }

    void InspectorPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        const auto& selectedEntities = m_Editor->GetSelected();

        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            ImGuiUtilities::PushID();

            Scene* currentScene = Application::Get().GetSceneManager()->GetCurrentScene();

            if(!currentScene)
            {
                m_Editor->SetSelected({});
                ImGuiUtilities::PopID();
                ImGui::End();
                return;
            }

            auto& registry = currentScene->GetRegistry();

            // Handle no selection
            if(selectedEntities.empty())
            {
                ImGui::TextDisabled("No entity selected");
                ImGuiUtilities::PopID();
                ImGui::End();
                return;
            }

            // Handle multiple entity selection
            if(selectedEntities.size() > 1)
            {
                DrawMultiEntityInspector(currentScene, selectedEntities);
                ImGuiUtilities::PopID();
                ImGui::End();
                return;
            }

            // Single entity selection - validate
            if(!registry.valid(selectedEntities.front()))
            {
                m_Editor->SetSelected({});
                ImGuiUtilities::PopID();
                ImGui::End();
                return;
            }

            auto selected         = selectedEntities.front();
            Entity SelectedEntity = { selected, currentScene };

            // active checkbox
            auto activeComponent = registry.try_get<ActiveComponent>(selected);
            bool active          = activeComponent ? activeComponent->active : true;
            if(ImGui::Checkbox("##ActiveCheckbox", &active))
            {
                if(!activeComponent)
                    registry.emplace<ActiveComponent>(selected, active);
                else
                    activeComponent->active = active;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(ICON_MDI_CUBE);
            ImGui::SameLine();

            bool hasName     = registry.all_of<NameComponent>(selected);
            std::string name = selected.GetName();

            if(m_DebugMode)
            {
                if(selected.Valid())
                {
                    ImGui::Text("entt ID: %u", static_cast<uint32_t>(selected));
                }
                else
                {
                    ImGui::TextUnformatted("INVALID ENTITY");
                }
            }

            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 4.0f);
            {
                ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                if(ImGuiUtilities::InputText(name, "##InspectorNameChange"))
                    registry.get_or_emplace<NameComponent>(selected).name = name;
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_FLOPPY))
                ImGui::OpenPopup("SavePrefab");

            ImGuiUtilities::Tooltip("Save Entity As Prefab");

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_TUNE))
                ImGui::OpenPopup("SetDebugMode");
            ImGui::PopStyleColor();

            if(ImGui::BeginPopup("SetDebugMode", 3))
            {
                if(SelectedEntity.HasComponent<PrefabComponent>() && ImGui::Button("Revert To Prefab"))
                {
                    auto path = SelectedEntity.GetComponent<PrefabComponent>().Path;
                    m_Editor->UnSelect(selected);
                    SelectedEntity.Destroy();

                    SelectedEntity = Application::Get().GetSceneManager()->GetCurrentScene()->InstantiatePrefab(path);
                    selected       = SelectedEntity;
                    m_Editor->SetSelected(selected);
                }

                if(ImGui::Selectable("Debug Mode", m_DebugMode))
                {
                    m_DebugMode = !m_DebugMode;
                }
                ImGui::EndPopup();
            }

            DebugView = m_DebugMode;

            if(ImGui::BeginPopupModal("SavePrefab", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Save Current Entity as a Prefab?\n\n");
                ImGui::Separator();

                static std::string prefabName = SelectedEntity.GetName();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Name : ");
                ImGui::SameLine();
                ImGuiUtilities::InputText(prefabName, "##PrefabNameChange");

                static std::string prefabNamePath = "//Assets/Prefabs/";
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Path : ");
                ImGui::SameLine();
                ImGuiUtilities::InputText(prefabNamePath, "##PrefabPathChange");

                if(ImGui::Button("OK", ImVec2(120, 0)))
                {
                    Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
                    Lumos::String8 physicalPath;
                    FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(prefabNamePath), &physicalPath, true);
                    std::string FullPath = std::string((const char*)physicalPath.str, physicalPath.size) + prefabName + std::string(".lprefab");
                    Application::Get().GetSceneManager()->GetCurrentScene()->SavePrefab({ selected, Application::Get().GetSceneManager()->GetCurrentScene() }, FullPath);
                    ImGui::CloseCurrentPopup();
                    ArenaTempEnd(scratch);
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if(ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            if(m_DebugMode)
            {
                auto idComponent = registry.try_get<IDComponent>(selected);

                if(idComponent)
                {
                    ImGui::Columns(2);
                    Lumos::ImGuiUtilities::Property("UUID", (uint64_t&)idComponent->ID, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

                    auto hierarchyComp = registry.try_get<Hierarchy>(selected);

                    if(hierarchyComp)
                    {
                        uint32_t childCount = static_cast<uint32_t>(hierarchyComp->m_ChildCount);
                        Lumos::ImGuiUtilities::Property("Child Count", childCount, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

                        if(registry.valid(hierarchyComp->Parent()))
                        {
                            idComponent = registry.try_get<IDComponent>(hierarchyComp->Parent());
                            Lumos::ImGuiUtilities::Property("Parent UUID", (uint64_t&)idComponent->ID, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                        }
                        else
                        {
                            auto nullString = std::string("NULL");
                            Lumos::ImGuiUtilities::Property("Parent UUID", nullString, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
                        }

                        entt::entity child = hierarchyComp->First();

                        if(child != entt::null)
                            ImGui::TextUnformatted("Children : ");
                        ImGui::Indent(24.0f);
                        ImGui::NextColumn();
                        ImGui::NextColumn();

                        while(child != entt::null)
                        {
                            idComponent = registry.try_get<IDComponent>(child);

                            Lumos::ImGuiUtilities::Property("UUID", (uint64_t&)idComponent->ID, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

                            auto hierarchy = registry.try_get<Hierarchy>(child);

                            if(hierarchy)
                            {
                                child = hierarchy->Next();
                            }
                        }

                        ImGui::Unindent(24.0f);
                        ImGui::NextColumn();
                    }

                    ImGui::Columns(1);
                    ImGui::Separator();
                }
            }

            if(registry.try_get<PrefabComponent>(selected))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
                ImGui::Separator();
                ImGui::Text("Prefab %s", registry.get<PrefabComponent>(selected).Path.c_str());
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            ImGui::BeginChild("Components", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);
            auto entityHandle = (entt::entity)selected.GetHandle();
            m_EnttEditor.RenderImGui(registry, entityHandle);
            ImGui::EndChild();

            ImGuiUtilities::PopID();
        }
        ImGui::End();
    }

    void InspectorPanel::DrawMultiEntityInspector(Scene* scene, const std::vector<Entity>& entities)
    {
        auto& registry = scene->GetRegistry();

        // Header showing selection count
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
        ImGui::Text(ICON_MDI_SELECT_ALL " %zu entities selected", entities.size());
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Count valid entities with transforms
        int validCount = 0;
        for(auto entity : entities)
        {
            if(entity.Valid() && entity.HasComponent<Maths::Transform>())
                validCount++;
        }

        if(validCount == 0)
        {
            ImGui::TextDisabled("No valid entities with transforms");
            return;
        }

        // Transform editing section
        if(ImGui::CollapsingHeader(ICON_MDI_VECTOR_LINE " Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            ImGui::TextDisabled("Changes apply to all selected entities");
            ImGui::Spacing();

            // Position offset
            static Vec3 positionOffset = Vec3(0.0f);
            ImGui::Text("Position Offset");
            bool posChanged = false;
            posChanged |= ImGui::DragFloat("X##PosOffset", &positionOffset.x, 0.1f);
            ImGui::SameLine();
            posChanged |= ImGui::DragFloat("Y##PosOffset", &positionOffset.y, 0.1f);
            ImGui::SameLine();
            posChanged |= ImGui::DragFloat("Z##PosOffset", &positionOffset.z, 0.1f);
            ImGui::SameLine();
            if(ImGui::Button("Apply##Pos"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                    {
                        transform->SetLocalPosition(transform->GetLocalPosition() + positionOffset);
                    }
                }
                positionOffset = Vec3(0.0f);
            }

            ImGui::Spacing();

            // Rotation offset (in degrees)
            static Vec3 rotationOffset = Vec3(0.0f);
            ImGui::Text("Rotation Offset (degrees)");
            bool rotChanged = false;
            rotChanged |= ImGui::DragFloat("X##RotOffset", &rotationOffset.x, 1.0f);
            ImGui::SameLine();
            rotChanged |= ImGui::DragFloat("Y##RotOffset", &rotationOffset.y, 1.0f);
            ImGui::SameLine();
            rotChanged |= ImGui::DragFloat("Z##RotOffset", &rotationOffset.z, 1.0f);
            ImGui::SameLine();
            if(ImGui::Button("Apply##Rot"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                    {
                        Vec3 currentRot = transform->GetLocalOrientation().ToEuler();
                        transform->SetLocalOrientation(Quat(currentRot + rotationOffset));
                    }
                }
                rotationOffset = Vec3(0.0f);
            }

            ImGui::Spacing();

            // Scale multiplier
            static Vec3 scaleMultiplier = Vec3(1.0f);
            ImGui::Text("Scale Multiplier");
            bool scaleChanged = false;
            scaleChanged |= ImGui::DragFloat("X##ScaleMult", &scaleMultiplier.x, 0.01f, 0.01f, 100.0f);
            ImGui::SameLine();
            scaleChanged |= ImGui::DragFloat("Y##ScaleMult", &scaleMultiplier.y, 0.01f, 0.01f, 100.0f);
            ImGui::SameLine();
            scaleChanged |= ImGui::DragFloat("Z##ScaleMult", &scaleMultiplier.z, 0.01f, 0.01f, 100.0f);
            ImGui::SameLine();
            if(ImGui::Button("Apply##Scale"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                    {
                        transform->SetLocalScale(transform->GetLocalScale() * scaleMultiplier);
                    }
                }
                scaleMultiplier = Vec3(1.0f);
            }

            ImGui::Spacing();

            // Uniform scale
            static float uniformScale = 1.0f;
            ImGui::Text("Uniform Scale");
            ImGui::DragFloat("##UniformScale", &uniformScale, 0.01f, 0.01f, 100.0f);
            ImGui::SameLine();
            if(ImGui::Button("Apply##UniformScale"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                    {
                        transform->SetLocalScale(transform->GetLocalScale() * uniformScale);
                    }
                }
                uniformScale = 1.0f;
            }

            ImGui::Unindent();
        }

        ImGui::Separator();

        // Quick actions
        if(ImGui::CollapsingHeader(ICON_MDI_FLASH " Quick Actions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            if(ImGui::Button(ICON_MDI_RESTART " Reset Position"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                        transform->SetLocalPosition(Vec3(0.0f));
                }
            }
            ImGui::SameLine();

            if(ImGui::Button(ICON_MDI_RESTART " Reset Rotation"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                        transform->SetLocalOrientation(Quat());
                }
            }
            ImGui::SameLine();

            if(ImGui::Button(ICON_MDI_RESTART " Reset Scale"))
            {
                for(auto entity : entities)
                {
                    if(!entity.Valid())
                        continue;
                    if(entity.HasComponent<EditorLockComponent>())
                        continue;
                    auto transform = entity.TryGetComponent<Maths::Transform>();
                    if(transform)
                        transform->SetLocalScale(Vec3(1.0f));
                }
            }

            ImGui::Unindent();
        }

        ImGui::Separator();

        // Show shared component types
        if(ImGui::CollapsingHeader(ICON_MDI_PUZZLE " Shared Components"))
        {
            ImGui::Indent();
            ImGui::TextDisabled("Components present on all selected entities:");
            ImGui::Spacing();

            // Check for common components
            bool allHaveTransform  = true;
            bool allHaveModel      = true;
            bool allHaveSprite     = true;
            bool allHaveLight      = true;
            bool allHaveRigidBody  = true;
            bool allHaveCamera     = true;

            for(auto entity : entities)
            {
                if(!entity.Valid())
                    continue;
                if(!entity.HasComponent<Maths::Transform>())
                    allHaveTransform = false;
                if(!entity.HasComponent<Graphics::ModelComponent>())
                    allHaveModel = false;
                if(!entity.HasComponent<Graphics::Sprite>())
                    allHaveSprite = false;
                if(!entity.HasComponent<Graphics::Light>())
                    allHaveLight = false;
                if(!entity.HasComponent<RigidBody3DComponent>() && !entity.HasComponent<RigidBody2DComponent>())
                    allHaveRigidBody = false;
                if(!entity.HasComponent<Camera>())
                    allHaveCamera = false;
            }

            if(allHaveTransform)
                ImGui::BulletText(ICON_MDI_AXIS_ARROW " Transform");
            if(allHaveModel)
                ImGui::BulletText(ICON_MDI_CUBE " Model");
            if(allHaveSprite)
                ImGui::BulletText(ICON_MDI_IMAGE " Sprite");
            if(allHaveLight)
                ImGui::BulletText(ICON_MDI_LIGHTBULB " Light");
            if(allHaveRigidBody)
                ImGui::BulletText(ICON_MDI_SOCCER " RigidBody");
            if(allHaveCamera)
                ImGui::BulletText(ICON_MDI_CAMERA " Camera");

            if(!allHaveTransform && !allHaveModel && !allHaveSprite && !allHaveLight && !allHaveRigidBody && !allHaveCamera)
                ImGui::TextDisabled("No common components found");

            ImGui::Unindent();
        }
    }

    void InspectorPanel::SetDebugMode(bool mode)
    {
        m_DebugMode = mode;
    }
}
