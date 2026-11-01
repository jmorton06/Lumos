#include "Editor.h"
#include <Lumos/Core/OS/Bookmarks.h>
#include <Lumos/Core/OS/FileDialogs.h>
#include "SceneViewPanel.h"
#include "GameViewPanel.h"
#include "ExportPanel.h"
#include <Lumos/Export/ExportProject.h>
#include <chrono>
#include <unordered_map>
#include "PreviewDraw.h"
#include "ImportPanel.h"
#include "EditorPanel.h"
#include "TerrainSculpt.h"
#include <Lumos/Core/Asset/AssetImporter.h>
#include <Lumos/Core/Asset/AssetPacker.h>
#include <Lumos/Core/Asset/AssetManager.h>

#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Utilities/Timer.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/Version.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Audio/AudioManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Events/ApplicationEvent.h>
#include <Lumos/Events/GestureEvent.h>
#include <Lumos/Scene/Component/Components.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Scene/Component/RigidBody2DComponent.h>
#include <Lumos/Scene/Component/RigidBody3DComponent.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <sol/sol.hpp>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h>
#include <Lumos/Graphics/MeshFactory.h>
#include <Lumos/Graphics/Sprite.h>
#include <Lumos/Graphics/AnimatedSprite.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/Renderers/GridRenderer.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Scene/EntityFactory.h>
#include <Lumos/Graphics/Environment.h>
#include <Lumos/Graphics/Animation/AnimationController.h>
#include <Lumos/Graphics/ShaderCompiler.h>

#include <fstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <Lumos/Core/BusyOverlay.h>
#ifdef LUMOS_PLATFORM_IOS
#include <Lumos/Platform/iOS/iOSOS.h>
#endif
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Embedded/EmbedAsset.h>
#include <Lumos/Embedded/lumosLogo.inl>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Maths/BoundingBox.h>
#include <Lumos/Maths/BoundingSphere.h>
#include <Lumos/Maths/Rect.h>
#include <Lumos/Maths/Frustum.h>
#include <Lumos/Maths/Plane.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Core/LMLog.h>
#include <Lumos/Core/String.h>
#include <Lumos/Core/CommandLine.h>
#include <Lumos/Core/CoreSystem.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Core/Undo.h>
#include "EditorUI.h"

#include <Lumos/Graphics/Gizmo.h>
#include <cereal/version.hpp>
#ifdef LUMOS_PLATFORM_IOS
#include <cereal/external/rapidjson/document.h>
#include <cereal/external/rapidjson/prettywriter.h>
#include <cereal/external/rapidjson/stringbuffer.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#endif

#ifdef LUMOS_PLATFORM_IOS
namespace
{
    // The project built into the app (ExampleProject unless the build named another): its folder and .lmproj.
    void FindBundledProject(std::string& folder, std::string& file)
    {
        folder = "ExampleProject";
        file   = "Example.lmproj";
        std::error_code error;
        for(const auto& dir : std::filesystem::directory_iterator(Lumos::OS::Get().GetAssetPath(), error))
        {
            if(!dir.is_directory(error))
                continue;
            for(const auto& entry : std::filesystem::directory_iterator(dir.path(), error))
            {
                if(entry.path().extension() == ".lmproj")
                {
                    folder = dir.path().filename().string();
                    file   = entry.path().filename().string();
                    return;
                }
            }
        }
    }

#ifdef LUMOS_PLATFORM_IOS
    // The project inside the app is read-only, so the first run copies it to Documents. It can
    // be gigabytes: on the launch thread that ran past iOS's launch limit and the app was killed
    // half way through (leaving half a project). It is copied on a worker into "<name>.partial"
    // and renamed when it is all there; the editor starts with no project and opens it then.
    std::atomic<int> s_BundleCopy { 0 }; // 0 nothing to do, 1 copying, 2 done, 3 failed
    std::string s_BundleCopyTo;

    namespace fs = std::filesystem;

    // The bundle's scenes that the copy's .lmproj does not list are added to it; its own edits stay.
    void MergeProjectScenes(const fs::path& bundled, const fs::path& copy)
    {
        auto read = [](const fs::path& path, CEREAL_RAPIDJSON_NAMESPACE::Document& doc)
        {
            std::ifstream in(path);
            std::stringstream text;
            text << in.rdbuf();
            doc.Parse(text.str().c_str());
            return !doc.HasParseError() && doc.HasMember("value0") && doc["value0"].HasMember("Scenes") && doc["value0"]["Scenes"].IsArray();
        };
        CEREAL_RAPIDJSON_NAMESPACE::Document from, to;
        if(!read(bundled, from) || !read(copy, to))
            return;
        auto& have = to["value0"]["Scenes"];
        bool changed = false;
        for(auto& scene : from["value0"]["Scenes"].GetArray())
        {
            bool found = false;
            for(auto& existing : have.GetArray())
                found |= existing.IsString() && scene.IsString() && strcmp(existing.GetString(), scene.GetString()) == 0;
            if(!found && scene.IsString())
            {
                have.PushBack(CEREAL_RAPIDJSON_NAMESPACE::Value(scene.GetString(), to.GetAllocator()), to.GetAllocator());
                changed = true;
            }
        }
        if(!changed)
            return;
        CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
        CEREAL_RAPIDJSON_NAMESPACE::PrettyWriter<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 4);
        to.Accept(writer);
        std::ofstream(copy) << buffer.GetString();
        LINFO("Bundled project: new scenes added to %s", copy.string().c_str());
    }

    // A newer build of the app brings the copy in Documents up to date: files it does not have
    // are added, and files the bundle changed are replaced unless they were edited since the last
    // sync (that edit is kept). BundleManifest.txt remembers each file as the bundle had it.
    void SyncBundledProject(const std::string& bundlePath, const std::string& docsDir)
    {
        std::error_code ec;
        const fs::path bundle(bundlePath), docs(docsDir);
        const fs::path manifestPath = docs / ".lumos" / "BundleManifest.txt";

        struct Entry
        {
            long long Size = 0, Time = 0;
        };
        std::unordered_map<std::string, Entry> manifest;
        long long syncTime = 0;
        bool haveManifest  = false;
        {
            std::ifstream in(manifestPath);
            std::string line;
            if(in && std::getline(in, line) && sscanf(line.c_str(), "sync %lld", &syncTime) == 1)
            {
                haveManifest = true;
                Entry e;
                char rel[1024];
                while(std::getline(in, line))
                    if(sscanf(line.c_str(), "%lld %lld %1023[^\n]", &e.Size, &e.Time, rel) == 3)
                        manifest[rel] = e;
            }
        }

        std::string out;
        int added = 0, updated = 0, kept = 0;
        for(auto it = fs::recursive_directory_iterator(bundle, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
        {
            if(!it->is_regular_file(ec))
                continue;
            const std::string rel = fs::relative(it->path(), bundle, ec).generic_string();
            if(rel.rfind(".lumos", 0) == 0)
                continue;
            Entry now;
            now.Size = (long long)it->file_size(ec);
            now.Time = (long long)it->last_write_time(ec).time_since_epoch().count();
            out += std::to_string(now.Size) + " " + std::to_string(now.Time) + " " + rel + "\n";

            const fs::path dst = docs / rel;
            if(!fs::exists(dst, ec))
            {
                fs::create_directories(dst.parent_path(), ec);
                fs::copy_file(it->path(), dst, ec);
                added++;
                continue;
            }
            auto known = manifest.find(rel);
            if(haveManifest && known != manifest.end() && known->second.Size == now.Size && known->second.Time == now.Time)
                continue; // the bundle has not changed it
            if(it->path().extension() == ".lmproj")
            {
                MergeProjectScenes(it->path(), dst);
                continue;
            }
            const long long edited = (long long)fs::last_write_time(dst, ec).time_since_epoch().count();
            if(haveManifest && edited <= syncTime)
            {
                fs::copy_file(it->path(), dst, fs::copy_options::overwrite_existing, ec);
                updated++;
            }
            else
                kept++;
        }

        fs::create_directories(manifestPath.parent_path(), ec);
        std::ofstream(manifestPath) << "sync " << (long long)fs::file_time_type::clock::now().time_since_epoch().count() << "\n" << out;
        LINFO("Bundled project synced: %d added, %d updated, %d kept as edited", added, updated, kept);
    }

    void StartBundleCopy(const std::string& bundlePath, const std::string& docsDir)
    {
        if(s_BundleCopy.load() != 0)
            return;
        std::error_code ec;
        const std::string partial = docsDir + ".partial";
        // a copy that was cut short last time is thrown away
        if(std::filesystem::exists(partial, ec))
            std::filesystem::remove_all(partial, ec);
        if(std::filesystem::exists(docsDir, ec))
        {
            // already copied: bring it up to date with this build (only stats, plus the few changed files)
            static bool s_Synced = false;
            if(!s_Synced)
            {
                s_Synced = true;
                SyncBundledProject(bundlePath, docsDir);
            }
            return;
        }
        s_BundleCopyTo = docsDir;
        s_BundleCopy   = 1;
        std::thread([bundlePath, docsDir, partial]()
                    {
                        bool ok = Lumos::iOSOS::CopyBundleFolder(bundlePath, partial);
                        std::error_code error;
                        if(ok)
                            std::filesystem::rename(partial, docsDir, error);
                        s_BundleCopy = (ok && !error) ? 2 : 3; })
            .detach();
    }
#endif

    const std::string& BundledProjectFolder()
    {
        static std::string folder, file;
        if(folder.empty())
            FindBundledProject(folder, file);
        return folder;
    }

    const std::string& BundledProjectFile()
    {
        static std::string folder, file;
        if(file.empty())
            FindBundledProject(folder, file);
        return file;
    }
}
#endif

namespace Lumos
{
    Editor::Editor()
        : Application()
        , m_IniFile("")
    {
        Application::SetInstance(this);

        m_AppType = AppType::Editor;
    }

    Editor::~Editor()
    {
    }

    void Editor::OnQuit()
    {
        SaveEditorSettings();

        for(auto panel : m_Panels)
            panel->DestroyGraphicsResources();

        // Must go before the renderer is torn down — ~Editor runs after
        // Application::Release(), when the deletion queues are already gone.
        m_LogoTexture.reset();

        if(m_EditorUI)
        {
            m_EditorUI->Shutdown();
            delete m_EditorUI;
            m_EditorUI = nullptr;
        }

        m_GridRenderer.reset();
        m_Panels.clear();
        m_PreviewDraw->ReleaseResources();
        delete m_PreviewDraw;
        delete m_ImportPanel;
        delete m_TerrainSculpt;
        m_TerrainSculpt = nullptr;

        Application::OnQuit();
    }


    void Editor::Init()
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectSettings.AutoImportMeshes = true;
        Lumos::InitialiseUndo();

#ifdef LUMOS_PLATFORM_LINUX
        m_TempSceneSaveFilePath = std::filesystem::current_path().string();
#elif defined(LUMOS_PLATFORM_IOS)
        m_TempSceneSaveFilePath = OS::Get().GetCurrentWorkingDirectory();
#else
        m_TempSceneSaveFilePath = std::filesystem::temp_directory_path().string();
#endif

        bool deleteIniFile   = false;
        CommandLine* cmdline = Internal::CoreSystem::GetCmdLine();
        if(cmdline->OptionBool(Str8Lit("CleanEditorIni")))
        {
            LINFO("Deleting editor ini file");
            deleteIniFile = true;
        }

#ifdef LUMOS_PLATFORM_WINDOWS
        m_TempSceneSaveFilePath += "Lumos\\";
#else
        m_TempSceneSaveFilePath += "/Lumos/";
#endif
        if(!FileSystem::FolderExists(Str8StdS(m_TempSceneSaveFilePath)))
        {
            std::error_code ec;
            std::filesystem::create_directory(m_TempSceneSaveFilePath, ec);
        }

        std::string iniDefaultPath;
        std::vector<std::string> iniLocation;

#ifdef LUMOS_PLATFORM_MACOS
        {
            // Sandbox-friendly: use Application Support directory
            const char* home = std::getenv("HOME");
            std::string appSupport = std::string(home ? home : "/tmp") + "/Library/Application Support/LumosEditor/";
            std::error_code ec;
            std::filesystem::create_directories(appSupport, ec);
            iniDefaultPath = appSupport + "Editor.ini";
            iniLocation.push_back(iniDefaultPath);
            // Also check legacy locations for migration
            std::string execPath = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath());
            iniLocation.push_back(execPath + "Editor.ini");
            iniLocation.push_back(execPath + "../../../Editor.ini");
        }
#elif defined(LUMOS_PLATFORM_IOS)
        iniDefaultPath = OS::Get().GetCurrentWorkingDirectory() + "/Editor.ini";
        iniLocation.push_back(iniDefaultPath);
#else
        iniDefaultPath = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "Editor.ini";
        iniLocation.push_back(iniDefaultPath);
        iniLocation.push_back(StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "../../../Editor.ini");
#endif

        bool fileFound = false;
        std::string filePath;
        for(auto& path : iniLocation)
        {
            if(FileSystem::FileExists(Str8StdS(path)))
            {
                filePath = path;

                LINFO("Loaded Editor Ini file %s", path.c_str());
                if(deleteIniFile)
                {
                    std::error_code ec;
                    std::filesystem::remove_all(path, ec);
                }
                else
                {
                    m_IniFile = IniFile(filePath);

                    fileFound = true;
                    LoadEditorSettings();
                }
                break;
            }
        }

        if(!fileFound)
        {
            LINFO("Editor Ini not found");
            filePath = iniDefaultPath;
            LINFO("Creating Editor Ini %s", filePath.c_str());

            m_IniFile = IniFile(filePath);
            AddDefaultEditorSettings();
        }

#ifdef LUMOS_PLATFORM_IOS
        std::string bundlePath  = OS::Get().GetAssetPath() + BundledProjectFolder();
        std::string docsDir     = OS::Get().GetCurrentWorkingDirectory() + "/LumosEditor/" + BundledProjectFolder();

        StartBundleCopy(bundlePath, docsDir);
#endif

        // everything the editor draws is on the engine UI: no ImGui context at all
        Application::SetImGuiEnabled(false);
        Application::Init();
        Application::SetEditorState(EditorState::Preview);
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Editor::OnEvent));

        m_RequestedThumbnailPath = PushStr8FillByte(m_Arena, 256, 0);

        String8 pathCopy                = PushStr8Copy(m_FrameArena, m_ProjectSettings.m_ProjectRoot.c_str());
        pathCopy                        = StringUtilities::ResolveRelativePath(m_FrameArena, pathCopy);
        m_ProjectSettings.m_ProjectRoot = (const char*)pathCopy.str;

        {
            m_EditorCamera = CreateSharedPtr<Camera>(60.0f,
                                                     0.01f,
                                                     m_Settings.m_CameraFar,
                                                     (float)Application::Get().GetWindowSize().x / (float)Application::Get().GetWindowSize().y);
            m_CurrentCamera = m_EditorCamera.get();

            // Apply saved per-scene camera, or default
            Vec3 camPos(-31.0f, 12.0f, 51.0f);
            Quat camRot(0.0f, 0.0f, 0.0f, 1.0f);
            auto* scene = GetCurrentScene();
            if(scene)
            {
                m_LastSceneName = scene->GetSceneName();
                for(size_t i = 0; i < m_Settings.m_SceneCameraStates.Size(); i++)
                {
                    auto& s = m_Settings.m_SceneCameraStates[i];
                    if(s.sceneName == m_LastSceneName)
                    {
                        camPos = Vec3(s.posX, s.posY, s.posZ);
                        camRot = Quat(s.rotX, s.rotY, s.rotZ, s.rotW);
                        break;
                    }
                }
            }

            m_EditorCameraTransform.SetLocalPosition(camPos);
            m_EditorCameraTransform.SetLocalOrientation(camRot);
            m_EditorCameraTransform.SetWorldMatrix(Mat4(1.0f));
        }

        m_ComponentIconMap[typeid(Graphics::Light).hash_code()]          = ICON_MDI_LIGHTBULB;
        m_ComponentIconMap[typeid(Camera).hash_code()]                   = ICON_MDI_CAMERA;
        m_ComponentIconMap[typeid(SoundComponent).hash_code()]           = ICON_MDI_VOLUME_HIGH;
        m_ComponentIconMap[typeid(Graphics::Sprite).hash_code()]         = ICON_MDI_IMAGE;
        m_ComponentIconMap[typeid(Maths::Transform).hash_code()]         = ICON_MDI_VECTOR_LINE;
        m_ComponentIconMap[typeid(RigidBody2DComponent).hash_code()]     = ICON_MDI_SQUARE_OUTLINE;
        m_ComponentIconMap[typeid(RigidBody3DComponent).hash_code()]     = ICON_MDI_CUBE_OUTLINE;
        m_ComponentIconMap[typeid(Graphics::ModelComponent).hash_code()] = ICON_MDI_VECTOR_POLYGON;
        m_ComponentIconMap[typeid(Graphics::Model).hash_code()]          = ICON_MDI_VECTOR_POLYGON;
        m_ComponentIconMap[typeid(LuaScriptComponent).hash_code()]       = ICON_MDI_LANGUAGE_LUA;
        m_ComponentIconMap[typeid(Graphics::Environment).hash_code()]    = ICON_MDI_EARTH;
        m_ComponentIconMap[typeid(Editor).hash_code()]                   = ICON_MDI_SQUARE;
        m_ComponentIconMap[typeid(TextComponent).hash_code()]            = ICON_MDI_TEXT;

        // what the editor UI shows the Scene / Game views from, and the Export panel's state
        m_Panels.emplace_back(CreateSharedPtr<GameViewPanel>());
        m_Panels.emplace_back(CreateSharedPtr<SceneViewPanel>());
        m_Panels.emplace_back(CreateSharedPtr<ExportPanel>());
        m_TerrainSculpt = new TerrainSculpt();

        for(auto& panel : m_Panels)
            panel->SetEditor(this);

        m_EditorUI = new EditorUI();
        m_EditorUI->Init(this);
        // the editor is built on the screen UI; EditorFrame turns the dock on from there
        EnableScreenUI(true);

        // --panel=<name>: open a panel at startup (screenshots, tests); case and spaces ignored
        {
            String8 open = Internal::CoreSystem::GetCmdLine()->OptionString(Str8Lit("panel"));
            if(open.size > 0)
                m_EditorUI->FocusPanelByKey(std::string((const char*)open.str, open.size).c_str());
        }

        m_ImportPanel      = new ImportPanel();
        m_PreviewDraw      = new PreviewDraw();

        CreateGridRenderer();

        // App logo for the title bar.
        {
            Graphics::TextureDesc        ldesc;
            ldesc.minFilter = Graphics::TextureFilter::LINEAR;
            ldesc.magFilter = Graphics::TextureFilter::LINEAR;
            ldesc.wrap      = Graphics::TextureWrap::CLAMP;
            Graphics::TextureLoadOptions lopts;
            lopts.flipY     = true;
            m_LogoTexture   = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromSource(lumosLogoWidth, lumosLogoHeight, (void*)lumosLogo, ldesc, lopts));
        }


        m_SelectedEntities.clear();
        // m_SelectedEntity = entt::null;

        Application::Get().GetSystem<LumosPhysicsEngine>()->SetDebugDrawFlags(m_Settings.m_Physics3DDebugFlags);
        Application::Get().GetSystem<B2PhysicsEngine>()->SetDebugDrawFlags(m_Settings.m_Physics2DDebugFlags);

        if(auto& am = Application::Get().GetAssetManager())
            am->SetTextureHotReloadEnabled(true);

        OS::Get().SetWindowDecorations(false);
        Application::Get().GetWindow()->SetWindowTitle("Lumos Editor");

        m_PreviewDraw->CreateDefaultScene();
    }

    bool Editor::IsTextFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        static const char* const kText[] = { "txt", "md", "log", "csv", "lua", "Lua",
                                             "glsl", "glslh", "shader", "vert", "frag", "comp",
                                             "json", "ini", "cfg", "conf", "toml", "yaml", "yml", "xml", "plist", "lmproj", "meta" };
        for(const char* text : kText)
            if(extension == text)
                return true;

        return false;
    }

    bool Editor::IsFontFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        if(extension == "ttf")
            return true;

        return false;
    }

    bool Editor::IsAudioFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        if(extension == "ogg" || extension == "wav")
            return true;

        return false;
    }

    bool Editor::IsShaderFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        if(extension == "vert" || extension == "frag" || extension == "comp")
            return true;

        return false;
    }

    bool Editor::IsSceneFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        if(extension == "lsn")
            return true;

        return false;
    }

    bool Editor::IsModelFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);

        if(extension == "obj" || extension == "gltf" || extension == "glb" || extension == "fbx" || extension == "FBX" || extension == "lmesh")
            return true;

        return false;
    }

    bool Editor::IsTextureFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);
        extension             = StringUtilities::ToLower(extension);
        if(extension == "png" || extension == "tga" || extension == "jpg" || extension == "limg")
            return true;

        return false;
    }

    bool IsPrefab(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);
        extension             = StringUtilities::ToLower(extension);
        if(extension == "lprefab")
            return true;

        return false;
    }

    bool Editor::IsMaterialFile(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string extension = StringUtilities::GetFilePathExtension(filePath);
        extension             = StringUtilities::ToLower(extension);
        if(extension == "lmat")
            return true;

        return false;
    }

    Graphics::RenderAPI StringToRenderAPI(const std::string& name)
    {
#ifdef LUMOS_RENDER_API_VULKAN
        if(name == "Vulkan")
            return Graphics::RenderAPI::VULKAN;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
        if(name == "Direct3D11")
            return Graphics::RenderAPI::DIRECT3D;
#endif

        LERROR("Unsupported Graphics API");

        return Graphics::RenderAPI::VULKAN;
    }

    static std::string projectLocation = "../";

    static const float identityMatrix[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

    // What the editor does each frame before its UI is built.
    void Editor::EditorFrame()
    {
        LUMOS_PROFILE_FUNCTION();

        // --play: start the project playing once the editor is up (tests, screenshots)
        {
            static int s_PlayIn = Internal::CoreSystem::GetCmdLine()->OptionBool(Str8Lit("play")) ? 3 : -1;
            if(s_PlayIn > 0 && m_ProjectLoaded && --s_PlayIn == 0 && Application::Get().GetEditorState() == EditorState::Preview)
                TogglePlay();
        }

#if defined(LUMOS_PLATFORM_WINDOWS) || defined(LUMOS_PLATFORM_LINUX)
        // The window is borderless: its edges resize it. (macOS keeps a native frame.)
        if(!OS::Get().IsWindowMaximised() && !OS::Get().IsWindowFullscreen() && m_Window)
        {
            static bool s_Resizing = false;
            static int s_Edge      = 0;
            const Vec2 mouse       = Input::Get().GetMousePosition();
            const float w = (float)m_Window->GetWidth(), h = (float)m_Window->GetHeight();
            const float edge = 6.0f;
            int hit = 0;
            if(mouse.x >= 0.0f && mouse.y >= 0.0f && mouse.x <= w && mouse.y <= h)
            {
                if(mouse.x <= edge)     hit |= 4;
                if(mouse.x >= w - edge) hit |= 8;
                if(mouse.y <= edge)     hit |= 1;
                if(mouse.y >= h - edge) hit |= 2;
            }
            const int mask = s_Resizing ? s_Edge : hit;
            if(mask == 1 || mask == 2)
                UISetCursor(UICursor_ResizeNS);
            else if(mask == 4 || mask == 8)
                UISetCursor(UICursor_ResizeEW);
            else if(mask == (1 | 4) || mask == (2 | 8))
                UISetCursor(UICursor_ResizeNWSE);
            else if(mask == (1 | 8) || mask == (2 | 4))
                UISetCursor(UICursor_ResizeNESW);

            // a window manager that takes the resize over drives it itself
            if(!s_Resizing && hit && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft) && !OS::Get().BeginWindowResize(hit))
            {
                s_Resizing = true;
                s_Edge     = hit;
            }
            if(s_Resizing)
            {
                OS::Get().UpdateWindowResize();
                if(!Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft))
                    s_Resizing = false;
            }
        }
#endif

        if(m_EditorUI && m_Window)
        {
            if(!m_EditorUI->IsEnabled())
                m_EditorUI->SetEnabled(true);
            // the whole window: the editor UI keeps its own controls clear of a notch
            m_EditorUI->SetRect(Vec4(0.0f, 0.0f, (float)m_Window->GetWidth(), (float)m_Window->GetHeight()));
        }

        if(!m_ProjectLoaded)
            return;

        const bool ctrl  = Input::Get().GetKeyHeld(InputCode::Key::LeftControl) || Input::Get().GetKeyHeld(InputCode::Key::RightControl)
            || Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::RightSuper);
        const bool shift = Input::Get().GetKeyHeld(InputCode::Key::LeftShift) || Input::Get().GetKeyHeld(InputCode::Key::RightShift);
        const bool z     = Input::Get().GetKeyPressed(InputCode::Key::Z);
        const bool y     = Input::Get().GetKeyPressed(InputCode::Key::Y);
        if(ctrl && z && !shift)
            Lumos::Undo();
        else if(ctrl && (y || (z && shift)))
            Lumos::Redo();

        m_Settings.m_View2D = m_CurrentCamera->IsOrthographic();

        if(Application::Get().GetEditorState() == EditorState::Preview)
            Application::Get().GetSceneManager()->GetCurrentScene()->UpdateSceneGraph();
    }

    // ---- scenes / developer actions (the editor UI's menus call these) ----

    void Editor::NewScene(const char* name, bool defaultSetup)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 sceneName = Str8C((char*)(name && name[0] ? name : "NewScene"));
        String8 base      = sceneName;
        int sameNameCount = 0;
        String8 path      = PushStr8F(scratch.arena, "//Assets/Scenes/%.*s.lsn", (int)sceneName.size, (char*)sceneName.str);
        while(FileSystem::FileExists(path) || m_SceneManager->ContainsScene((const char*)PushStr8Copy(scratch.arena, sceneName).str))
        {
            sameNameCount++;
            sceneName = PushStr8F(scratch.arena, "%.*s%i", (int)base.size, (char*)base.str, sameNameCount);
            path      = PushStr8F(scratch.arena, "//Assets/Scenes/%.*s.lsn", (int)sceneName.size, (char*)sceneName.str);
        }
        auto scene = new Scene(std::string((const char*)sceneName.str, sceneName.size));
        ScratchEnd(scratch);

        if(defaultSetup)
        {
            auto cube = scene->GetEntityManager()->Create("Cube");
            cube.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);

            auto light     = scene->GetEntityManager()->Create("Light");
            light.AddComponent<Graphics::Light>();
            Mat4 lightView = Mat4::LookAt(Vec3(30.0f, 9.0f, 50.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
            light.GetTransform().SetLocalTransform(lightView);
            light.GetTransform().SetWorldMatrix(Mat4(1.0f));

            auto camera = scene->GetEntityManager()->Create("Camera");
            camera.AddComponent<Camera>();
            camera.GetComponent<Camera>().SetFar(10000);
            Mat4 viewMat = Mat4::LookAt(Vec3(-1.0f, 0.5f, 1.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
            camera.GetTransform().SetLocalTransform(viewMat);
            camera.GetTransform().SetWorldMatrix(Mat4(1.0f));

            auto bb = cube.GetComponent<Graphics::ModelComponent>().ModelRef->GetMeshes().Front()->GetBoundingBox();
            camera.GetTransform().SetLocalPosition(-(camera.GetTransform().GetForwardDirection()) * Maths::Distance(bb.Max(), bb.Min()));
            camera.GetTransform().SetWorldMatrix(Mat4(1.0f));

            auto environment = scene->GetEntityManager()->Create("Environment");
            environment.AddComponent<Graphics::Environment>();
            environment.GetComponent<Graphics::Environment>().Load();

            scene->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/");
        }
        m_SceneManager->EnqueueScene(scene);
        m_SceneManager->SwitchScene((int)(m_SceneManager->GetScenes().Size()) - 1);
    }

    void Editor::ReloadScene()
    {
        m_SceneManager->SwitchScene(m_SceneManager->GetCurrentSceneIndex());
    }

    bool Editor::IsAssetPackBusy()
    {
        return System::JobSystem::IsBusy(m_AssetPackContext);
    }

    void Editor::BuildAssetPack()
    {
        if(IsAssetPackBusy())
            return;
        std::string assetsDir  = m_ProjectSettings.m_ProjectRoot + "Assets/";
        std::string outputPath = m_ProjectSettings.m_ProjectRoot + "Assets.lpak";
        System::JobSystem::Execute(m_AssetPackContext, [this, assetsDir, outputPath](JobDispatchArgs)
                                   {
                                       PackSettings settings;
                                       settings.ExcludePaths.PushBack(Str8Lit("Cache/"));
                                       settings.ExcludePaths.PushBack(Str8Lit("Imported/"));
                                       m_AssetPackResult = AssetPacker::Pack(Str8StdS(outputPath), Str8StdS(assetsDir), settings); });
    }

    void Editor::OnScreenUI()
    {
        EditorFrame();
        if(!m_EditorUI)
            return;
        if(m_ProjectLoaded)
            m_EditorUI->OnScreenUI();
        else
            m_EditorUI->OnScreenUINoProject();
    }

    void Editor::TogglePlay()
    {
        const bool isPlay = Application::Get().GetEditorState() != EditorState::Preview;
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(isPlay);
        Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(isPlay);
        Application::Get().GetSystem<AudioManager>()->UpdateListener(Application::Get().GetCurrentScene());
        Application::Get().GetSystem<AudioManager>()->SetPaused(isPlay);
        Application::Get().SetEditorState(isPlay ? EditorState::Preview : EditorState::Play);
        if(isPlay)
        {
            m_SelectedEntities.clear();
            LoadCachedScene();
        }
        else
        {
            CacheScene();
            AddAppScriptEntity();
            Application::Get().GetCurrentScene()->OnInit();
        }
    }

    void Editor::TogglePause()
    {
        const EditorState state = Application::Get().GetEditorState();
        if(state == EditorState::Play)
            Application::Get().SetEditorState(EditorState::Paused);
        else if(state == EditorState::Paused)
            Application::Get().SetEditorState(EditorState::Play);
    }

    void Editor::SaveCurrentScene()
    {
        if(Application::Get().GetEditorState() != EditorState::Preview)
            return; // the scene on screen is the played copy
        Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/", false);
        if(m_EditorUI)
            m_EditorUI->NoteSceneSaved();
    }

    void Editor::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();

        // Save outgoing scene camera (skip during init when camera isn't created yet)
        if(m_EditorCamera && !m_LastSceneName.empty())
        {
            const Vec3& pos = m_EditorCameraTransform.GetLocalPosition();
            const Quat& rot = m_EditorCameraTransform.GetLocalOrientation();
            bool found      = false;
            for(size_t i = 0; i < m_Settings.m_SceneCameraStates.Size(); i++)
            {
                if(m_Settings.m_SceneCameraStates[i].sceneName == m_LastSceneName)
                {
                    auto& s = m_Settings.m_SceneCameraStates[i];
                    s.posX = pos.x; s.posY = pos.y; s.posZ = pos.z;
                    s.rotX = rot.x; s.rotY = rot.y; s.rotZ = rot.z; s.rotW = rot.w;
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                EditorSettings::SceneCameraState s;
                s.sceneName = m_LastSceneName;
                s.posX = pos.x; s.posY = pos.y; s.posZ = pos.z;
                s.rotX = rot.x; s.rotY = rot.y; s.rotZ = rot.z; s.rotW = rot.w;
                m_Settings.m_SceneCameraStates.PushBack(s);
            }
        }

        Application::OnNewScene(scene);
        m_HoveredEntity = {};
        m_SelectedEntities.clear();

        // Restore incoming scene camera
        if(scene && m_EditorCamera)
        {
            const auto& name = scene->GetSceneName();
            m_LastSceneName   = name;

            Vec3 camPos(-31.0f, 12.0f, 51.0f);
            Quat camRot(0.0f, 0.0f, 0.0f, 1.0f);
            for(size_t i = 0; i < m_Settings.m_SceneCameraStates.Size(); i++)
            {
                if(m_Settings.m_SceneCameraStates[i].sceneName == name)
                {
                    auto& s = m_Settings.m_SceneCameraStates[i];
                    camPos  = Vec3(s.posX, s.posY, s.posZ);
                    camRot  = Quat(s.rotX, s.rotY, s.rotZ, s.rotW);
                    break;
                }
            }
            m_EditorCameraTransform.SetLocalPosition(camPos);
            m_EditorCameraTransform.SetLocalOrientation(camRot);
        }
        else if(scene)
        {
            m_LastSceneName = scene->GetSceneName();
        }

        // Track scene in recent scenes list
        if(scene)
        {
            auto sceneName = scene->GetSceneName();
            if(!sceneName.empty())
            {
                auto it = std::find(m_Settings.m_RecentScenes.begin(), m_Settings.m_RecentScenes.end(), sceneName);
                if(it != m_Settings.m_RecentScenes.end())
                    m_Settings.m_RecentScenes.erase(it);
                m_Settings.m_RecentScenes.insert(m_Settings.m_RecentScenes.begin(), sceneName);
                if(m_Settings.m_RecentScenes.size() > 10)
                    m_Settings.m_RecentScenes.resize(10);
            }
        }

        for(auto panel : m_Panels)
        {
            panel->OnNewScene(scene);
        }
        if(m_TerrainSculpt)
            m_TerrainSculpt->OnNewScene(scene);

        std::string Configuration;
        std::string Platform;
        std::string RenderAPI;

#ifdef LUMOS_DEBUG
        Configuration = "Debug";
#elif LUMOS_RELEASE
        Configuration = "Release";
#elif LUMOS_PRODUCTION
        Configuration = "Production";
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
        Platform = "Windows";
#elif LUMOS_PLATFORM_LINUX
        Platform = "Linux";
#elif LUMOS_PLATFORM_MACOS
        Platform = "MacOS";
#elif LUMOS_PLATFORM_IOS
        Platform = "iOS";
#endif

        switch(Graphics::GraphicsContext::GetRenderAPI())
        {

#ifdef LUMOS_RENDER_API_VULKAN
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
        case Graphics::RenderAPI::VULKAN:
            RenderAPI = "Vulkan ( MoltenVK )";
            break;
#else
        case Graphics::RenderAPI::VULKAN:
            RenderAPI = "Vulkan";
            break;
#endif
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
        case DIRECT3D:
            RenderAPI = "Direct3D";
            break;
#endif
        default:
            break;
        }

        // std::stringstream Title;
        // Title << Platform << dash << RenderAPI << dash << Configuration << dash << scene->GetSceneName() << dash << Application::Get().GetWindow()->GetTitle();
        String8 title = PushStr8F(m_Arena, "%s - %s - %s - %s - %s", Platform.c_str(), RenderAPI.c_str(), Configuration.c_str(), scene->GetSceneName().c_str(), Application::Get().GetWindow()->GetTitle().c_str());
        Application::Get().GetWindow()->SetWindowTitle((const char*)(title.str));
    }

    void Editor::Draw3DGrid()
    {
        LUMOS_PROFILE_FUNCTION();
#if 1
        if(!m_GridRenderer || !Application::Get().GetSceneManager()->GetCurrentScene())
        {
            return;
        }

        DebugRenderer::DrawHairLine(Vec3(-5000.0f, 0.0f, 0.0f), Vec3(5000.0f, 0.0f, 0.0f), true, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        DebugRenderer::DrawHairLine(Vec3(0.0f, -5000.0f, 0.0f), Vec3(0.0f, 5000.0f, 0.0f), true, Vec4(0.0f, 1.0f, 0.0f, 1.0f));
        DebugRenderer::DrawHairLine(Vec3(0.0f, 0.0f, -5000.0f), Vec3(0.0f, 0.0f, 5000.0f), true, Vec4(0.0f, 0.0f, 1.0f, 1.0f));

        m_GridRenderer->SetDepthTarget(m_SceneRenderer->GetForwardData().m_DepthTexture);
        m_GridRenderer->BeginScene(Application::Get().GetSceneManager()->GetCurrentScene(), m_EditorCamera.get(), &m_EditorCameraTransform);
        m_GridRenderer->RenderScene();
#endif
    }

    bool Editor::OnFileDrop(WindowFileEvent& e)
    {
        FileOpenCallback(e.GetFilePath());
        return true;
    }

    void Editor::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowFileEvent>(BIND_EVENT_FN(Editor::OnFileDrop));

        // Handle three-finger swipe for undo/redo
        dispatcher.Dispatch<GestureSwipeEvent>([this](GestureSwipeEvent& event) {
            if(event.GetNumTouches() == 3)
            {
                if(event.GetDirection() == SwipeDirection::Left)
                {
                    Lumos::Undo();
                    LINFO("Undo triggered by gesture");
                    return true;
                }
                else if(event.GetDirection() == SwipeDirection::Right)
                {
                    Lumos::Redo();
                    LINFO("Redo triggered by gesture");
                    return true;
                }
            }
            return false;
        });

        // closing the window with work not saved asks first (the engine UI draws the question)
        if(e.GetEventType() == EventType::WindowClose && m_EditorUI && m_EditorUI->IsEnabled() && m_EditorUI->HasUnsavedWork())
        {
            m_EditorUI->RequestQuit();
            return;
        }

        Application::OnEvent(e);
    }

    Maths::Ray Editor::GetScreenRay(int x, int y, Camera* camera, int width, int height)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!camera)
            return Maths::Ray();

        float screenX = (float)x / (float)width;
        float screenY = (float)y / (float)height;

        bool flipY = true;

        return camera->GetScreenRay(screenX, screenY, m_EditorCameraTransform.GetWorldMatrix().Inverse(), flipY);
    }

    void Editor::OnUpdate(const TimeStep& ts)
    {
        LUMOS_PROFILE_FUNCTION();

#ifdef LUMOS_PLATFORM_IOS
        // the first run's copy of the bundled project: the overlay while it goes, then open it
        {
            static bool s_Shown = false;
            const int copy      = s_BundleCopy.load();
            if(copy == 1 && !s_Shown)
            {
                s_Shown = true;
                Busy::Begin("Preparing the example project");
                Busy::SetStep("Copying it out of the app - the first run only");
            }
            else if(copy >= 2 && s_Shown)
            {
                s_Shown = false;
                Busy::End();
                if(copy == 2 && !m_ProjectLoaded)
                    ProjectOpenCallback(s_BundleCopyTo + "/" + BundledProjectFile());
                else if(copy == 3)
                    LERROR("Could not copy the bundled project to %s", s_BundleCopyTo.c_str());
            }
        }
#endif

        using namespace Lumos;

        static float autoSaveTimer = 0.0f;
        if(m_AutoSaveSettingsTime > 0)
        {
            if(autoSaveTimer > m_AutoSaveSettingsTime)
            {
                SaveEditorSettings();
                autoSaveTimer = 0;
            }

            autoSaveTimer += (float)ts.GetMillis();
        }
        auto& registry = GetCurrentScene()->GetEntityManager()->GetRegistry();
        m_SelectedEntities.erase(std::remove_if(m_SelectedEntities.begin(), m_SelectedEntities.end(), [&registry](entt::entity entity)
                                                { return !registry.valid(entity); }),
                                 m_SelectedEntities.end());

        // the scene view exposes like the game's camera, so both views look the same;
        // with no camera in the scene, sunny 16 (f/16, 1/125 s, ISO 100)
        if(m_EditorCamera)
        {
            auto cameras = registry.view<Camera>();
            if(!cameras.empty())
            {
                const Camera& game = cameras.get<Camera>(cameras.front());
                m_EditorCamera->SetAperture(game.GetAperture());
                m_EditorCamera->SetShutterSpeed(game.GetShutterSpeed());
                m_EditorCamera->SetSensitivity(game.GetSensitivity());
            }
            else
            {
                m_EditorCamera->SetAperture(16.0f);
                m_EditorCamera->SetShutterSpeed(1.0f / 125.0f);
                m_EditorCamera->SetSensitivity(100.0f);
            }
        }

        if(m_EditorState == EditorState::Play)
            autoSaveTimer = 0.0f;

        if(Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape) && GetEditorState() != EditorState::Preview || m_QueuedScenePreviewEnd)
        {
            Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(true);
            Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(true);

            Application::Get().GetSystem<AudioManager>()->UpdateListener(Application::Get().GetCurrentScene());
            Application::Get().GetSystem<AudioManager>()->SetPaused(true);
            Application::Get().SetEditorState(EditorState::Preview);

            m_SelectedEntities.clear();
            LoadCachedScene();
            SetEditorState(EditorState::Preview);
            m_QueuedScenePreviewEnd = false;
        }

        if((Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::LeftControl))
           && Input::Get().GetKeyPressed(Lumos::InputCode::Key::Z))
        {
            Undo();
        }

        if(m_SceneViewActive)
        {
            auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();

            {
                const Vec2 mousePos = Input::Get().GetMousePosition();
                m_EditorCameraController.SetCamera(m_EditorCamera);

                // Make sure the camera is not controllable during transitions
                if(!m_TransitioningCamera)
                {
                    m_EditorCameraController.HandleMouse(m_EditorCameraTransform, (float)ts.GetSeconds(), mousePos.x, mousePos.y);
                    m_EditorCameraController.HandleKeyboard(m_EditorCameraTransform, (float)ts.GetSeconds());
                }

                m_EditorCameraTransform.SetWorldMatrix(Mat4(1.0f));

                if(!m_SelectedEntities.empty() && Input::Get().GetKeyPressed(InputCode::Key::F))
                {
                    // Calculate combined bounds for all selected entities
                    Vec3 minBounds(std::numeric_limits<float>::max());
                    Vec3 maxBounds(std::numeric_limits<float>::lowest());
                    bool hasValidBounds = false;

                    for(auto entity : m_SelectedEntities)
                    {
                        if(!registry.valid(entity))
                            continue;

                        auto transform = registry.try_get<Maths::Transform>(entity);
                        if(!transform)
                            continue;

                        Vec3 pos = transform->GetWorldPosition();
                        Vec3 entityMin = pos;
                        Vec3 entityMax = pos;

                        // Check for ModelComponent to get actual bounds
                        auto model = registry.try_get<Graphics::ModelComponent>(entity);
                        if(model && model->ModelRef)
                        {
                            auto& meshes = model->ModelRef->GetMeshes();
                            for(auto& mesh : meshes)
                            {
                                if(mesh)
                                {
                                    // Transform bounding box to world space
                                    auto bb = mesh->GetBoundingBox().Transformed(transform->GetWorldMatrix());

                                    entityMin = Maths::Min(entityMin, Maths::Min(bb.Min(), bb.Max()));
                                    entityMax = Maths::Max(entityMax, Maths::Max(bb.Min(), bb.Max()));
                                }
                            }
                        }

                        // Also consider sprite bounds
                        auto sprite = registry.try_get<Graphics::Sprite>(entity);
                        if(sprite)
                        {
                            Vec3 scale = transform->GetWorldScale();
                            entityMin = pos - scale * 0.5f;
                            entityMax = pos + scale * 0.5f;
                        }

                        minBounds = Maths::Min(minBounds, entityMin);
                        maxBounds = Maths::Max(maxBounds, entityMax);
                        hasValidBounds = true;
                    }

                    if(hasValidBounds)
                    {
                        Vec3 center = (minBounds + maxBounds) * 0.5f;
                        float distance = Maths::Max(2.0f, Maths::Distance(minBounds, maxBounds) * 1.2f);
                        FocusCamera(center, distance, 2.0f);
                    }
                }
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::O))
            {
                FocusCamera(Vec3(0.0f, 0.0f, 0.0f), 2.0f, 2.0f);
            }

            if(m_TransitioningCamera)
            {
                // Defines the tolerance for distance, beyond which a transition is considered completed
                constexpr float kTransitionCompletionDistanceTolerance = 0.01f;
                constexpr float kSpeedBaseFactor                       = 5.0f;

                const Vec3 cameraCurrentPosition = m_EditorCameraTransform.GetLocalPosition();

                m_EditorCameraTransform.SetLocalPosition(
                    Maths::Lerp(
                        cameraCurrentPosition,
                        m_CameraDestination,
                        Maths::Clamp(m_CameraTransitionSpeed * kSpeedBaseFactor * static_cast<float>(ts.GetSeconds()), 0.0f, 1.0f)));

                auto distanceToDestination = Maths::Distance(cameraCurrentPosition, m_CameraDestination);

                m_TransitioningCamera = distanceToDestination > kTransitionCompletionDistanceTolerance;
            }

            if(!Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight) && !TransformGizmo::IsUsing())
            {
                if(Input::Get().GetKeyPressed(InputCode::Key::Q))
                {
                    SetGizmoOperation(GizmoOperation_None);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::W))
                {
                    SetGizmoOperation(GizmoOperation_Translate);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::E))
                {
                    SetGizmoOperation(GizmoOperation_Rotate);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::R))
                {
                    SetGizmoOperation(GizmoOperation_Scale);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::T))
                {
                    SetGizmoOperation(GizmoOperation_Universal);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::Y))
                {
                    ToggleSnap();
                }
            }

            static float m_SceneSavePopupTimer = -1.0f;
            if(m_SceneSavePopupTimer > 0.0f)
                m_SceneSavePopupTimer -= (float)Engine::GetTimeStep().GetSeconds();

            if((Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || (Input::Get().GetKeyHeld(InputCode::Key::LeftControl))))
            {
                if(Input::Get().GetKeyPressed(InputCode::Key::S) && Application::Get().GetSceneActive())
                {
                    Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/scenes/", false);
                    m_SceneSavePopupTimer = 2.0f;
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::O))
                    Application::Get().GetSceneManager()->GetCurrentScene()->Deserialise(m_ProjectSettings.m_ProjectRoot + "Assets/scenes/", false);

                if(Input::Get().GetKeyPressed(InputCode::Key::X))
                {
                    for(auto entity : m_SelectedEntities)
                        SetCopiedEntity(entity, true);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::C))
                {
                    for(auto entity : m_SelectedEntities)
                        SetCopiedEntity(entity, false);
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::V) && !m_CopiedEntities.empty())
                {
                    for(auto entity : m_CopiedEntities)
                    {
                        Application::Get().GetCurrentScene()->DuplicateEntity({ entity, Application::Get().GetCurrentScene() });
                        if(entity.Valid())
                        {
                            // if(m_CopiedEntity == m_SelectedEntity)
                            //   m_SelectedEntity = entt::null;
                            Entity(entity, Application::Get().GetCurrentScene()).Destroy();
                        }
                    }
                }

                if(Input::Get().GetKeyPressed(InputCode::Key::D) && !m_SelectedEntities.empty())
                {
                    for(auto entity : m_CopiedEntities)
                        Application::Get().GetCurrentScene()->DuplicateEntity({ entity, Application::Get().GetCurrentScene() });
                }
            }
        }
        else
            m_EditorCameraController.StopMovement();

        Application::OnUpdate(ts);
    }

    void Editor::SetSelected(Entity entity)
    {
        if(!entity.Valid())
            return;
        if(std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end())
            return;

        m_SelectedEntities.push_back(entity);
    }

    void Editor::UnSelect(Entity entity)
    {
        auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);

        if(it != m_SelectedEntities.end())
        {
            m_SelectedEntities.erase(it);
        }
    }

    void Editor::FocusCamera(const Vec3& point, float distance, float speed)
    {
        LUMOS_PROFILE_FUNCTION();

        m_EditorCameraController.StopMovement();

        if(m_CurrentCamera->IsOrthographic())
        {
            m_EditorCameraTransform.SetLocalPosition(point);
            // m_CurrentCamera->SetScale(distance * 0.5f);
        }
        else
        {
            m_TransitioningCamera = true;

            m_CameraDestination     = point - m_EditorCameraTransform.GetForwardDirection() * distance;
            m_CameraTransitionSpeed = speed;
        }
    }

    bool Editor::OnWindowResize(WindowResizeEvent& e)
    {
        return false;
    }

    void Editor::RecompileShaders()
    {
        LUMOS_PROFILE_FUNCTION();

#ifdef LUMOS_SHADERC
        LINFO("Recompiling shaders with shaderc...");

        std::string shaderDir = ROOT_DIR "/Lumos/Assets/Shaders/";
        std::string spvDir    = shaderDir + "CompiledSPV/";

        int compiled = 0, failed = 0;

        auto compileFile = [&](const std::string& path, Graphics::ShaderType type) {
            std::ifstream file(path);
            if(!file.is_open()) return;

            std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            std::string filename = path.substr(path.find_last_of("/\\") + 1);
            auto result = Graphics::ShaderCompiler::Compile(source, type, filename);

            if(!result.success)
            {
                LERROR("Shader compile failed: %s - %s", filename.c_str(), result.error.c_str());
                failed++;
                return;
            }

            // Write .spv output
            std::string ext = (type == Graphics::ShaderType::VERTEX) ? ".vert.spv"
                            : (type == Graphics::ShaderType::FRAGMENT) ? ".frag.spv"
                            : ".comp.spv";
            size_t dotPos = filename.find_last_of('.');
            std::string baseName = (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
            std::string outPath  = spvDir + baseName + ext;

            std::ofstream out(outPath, std::ios::binary);
            if(out.is_open())
            {
                out.write(reinterpret_cast<const char*>(result.spirv.data()),
                         result.spirv.size() * sizeof(uint32_t));
                out.close();
                compiled++;
            }
        };

        // Iterate shader directory
        for(auto& entry : std::filesystem::directory_iterator(shaderDir))
        {
            if(!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::string path = entry.path().string();

            if(ext == ".vert")
                compileFile(path, Graphics::ShaderType::VERTEX);
            else if(ext == ".frag")
                compileFile(path, Graphics::ShaderType::FRAGMENT);
            else if(ext == ".comp")
                compileFile(path, Graphics::ShaderType::COMPUTE);
        }

        LINFO("Shader compilation done: %d compiled, %d failed", compiled, failed);
#else
        LINFO("Shader recompilation not available (shaderc not linked)");
#endif
    }

    void Editor::DebugDraw()
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry      = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
        Vec4 selectedColour = Vec4(0.9f);
        if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::MeshBoundingBoxes)
        {
            auto group = registry.group<Graphics::ModelComponent>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto& [model, trans] = group.get<Graphics::ModelComponent, Maths::Transform>(entity);
                auto& meshes               = model.ModelRef->GetMeshes();
                auto& worldTransform       = trans.GetWorldMatrix();

                for(auto mesh : meshes)
                {
                    auto bbCopy = mesh->GetBoundingBox().Transformed(worldTransform);
                    DebugRenderer::DebugDraw(bbCopy, selectedColour, true);
                }
            }
        }

        {
            auto group = registry.group<Graphics::ModelComponent>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto& [model, trans] = group.get<Graphics::ModelComponent, Maths::Transform>(entity);
                auto& worldTransform       = trans.GetWorldMatrix();
                if(model.ModelRef && model.ModelRef->GetAnimationController())
                {
                    model.ModelRef->GetAnimationController()->DebugDraw(worldTransform);
                }
            }
        }

        if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::SpriteBoxes)
        {
            auto group = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto& [sprite, trans] = group.get<Graphics::Sprite, Maths::Transform>(entity);

                {
                    auto& worldTransform = trans.GetWorldMatrix();

                    auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                    bb.Transform(trans.GetWorldMatrix());
                    DebugRenderer::DebugDraw(bb, selectedColour, true);
                }
            }

            auto animGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);

            for(auto entity : animGroup)
            {
                const auto& [sprite, trans] = animGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

                {
                    auto& worldTransform = trans.GetWorldMatrix();

                    auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                    bb.Transform(trans.GetWorldMatrix());
                    DebugRenderer::DebugDraw(bb, selectedColour, true);
                }
            }
        }

        if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::CameraFrustum)
        {
            auto cameraGroup = registry.group<Camera>(entt::get<Maths::Transform>);

            for(auto entity : cameraGroup)
            {
                const auto& [camera, trans] = cameraGroup.get<Camera, Maths::Transform>(entity);

                {
                    DebugRenderer::DebugDraw(camera.GetFrustum(trans.GetWorldMatrix().Inverse()), Vec4(0.9f));
                }
            }
        }

        if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::EntityNames)
        {
            auto transform = registry.view<Maths::Transform>();

            for(auto entity : transform)
            {
                Entity e = { entity, GetCurrentScene() };
                {
                    DebugRenderer::DrawTextWs(e.GetTransform().GetWorldPosition(), 20.0f, false, Vec4(1.0f), 0.0f, e.GetName());
                }
            }
        }

        static std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
        if(m_HoveredEntity)
        {
            float alpha    = (float)Maths::Sin(std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count() * 200.0);
            alpha          = Maths::Abs(alpha);
            Vec4 colour    = Vec4(0.9f, 0.9f, 0.1f, alpha);
            auto transform = m_HoveredEntity.TryGetComponent<Maths::Transform>();
            auto model     = m_HoveredEntity.TryGetComponent<Graphics::ModelComponent>();
            if(transform && model && model->ModelRef)
            {
                auto& meshes = model->ModelRef->GetMeshes();
                for(auto mesh : meshes)
                {
                    auto& worldTransform = transform->GetWorldMatrix();
                    auto bbCopy          = mesh->GetBoundingBox().Transformed(worldTransform);
                    DebugRenderer::DebugDraw(bbCopy, colour, true);
                }
            }
            auto sprite = m_HoveredEntity.TryGetComponent<Graphics::Sprite>();
            if(transform && sprite)
            {
                {
                    auto& worldTransform = transform->GetWorldMatrix();

                    auto bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetPosition() + sprite->GetScale()));
                    bb.Transform(worldTransform);
                    DebugRenderer::DebugDraw(bb, colour, true);
                }
            }
            auto animSprite = m_HoveredEntity.TryGetComponent<Graphics::AnimatedSprite>();
            if(transform && animSprite)
            {
                auto& worldTransform = transform->GetWorldMatrix();

                auto bb = Maths::BoundingBox(Maths::Rect(animSprite->GetPosition(), animSprite->GetPosition() + animSprite->GetScale()));
                bb.Transform(worldTransform);
                DebugRenderer::DebugDraw(bb, colour, true);
            }
            auto camera = m_HoveredEntity.TryGetComponent<Camera>();
            if(camera && transform)
            {
                DebugRenderer::DebugDraw(camera->GetFrustum(transform->GetWorldMatrix().Inverse()), colour);
            }

            auto light = m_HoveredEntity.TryGetComponent<Graphics::Light>();
            if(light && transform)
            {
                DebugRenderer::DebugDraw(light, transform->GetWorldOrientation(), colour);
            }

            auto sound = m_HoveredEntity.TryGetComponent<SoundComponent>();
            if(sound)
            {
                DebugRenderer::DebugDraw(sound->GetSoundNode(), colour);
            }
        }
        else
            startTime = std::chrono::steady_clock::now();

        for(auto m_SelectedEntity : m_SelectedEntities)
            if(registry.valid(m_SelectedEntity)) // && Application::Get().GetEditorState() == EditorState::Preview)
            {
                auto transform = registry.try_get<Maths::Transform>(m_SelectedEntity);

                auto model = registry.try_get<Graphics::ModelComponent>(m_SelectedEntity);
                if(transform && model && model->ModelRef)
                {
                    auto& meshes = model->ModelRef->GetMeshes();
                    for(auto mesh : meshes)
                    {
                        auto& worldTransform = transform->GetWorldMatrix();
                        auto bbCopy          = mesh->GetBoundingBox().Transformed(worldTransform);
                        DebugRenderer::DebugDraw(bbCopy, selectedColour, true);
                    }
                }

                auto sprite = registry.try_get<Graphics::Sprite>(m_SelectedEntity);
                if(transform && sprite)
                {
                    {
                        auto& worldTransform = transform->GetWorldMatrix();

                        auto bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetPosition() + sprite->GetScale()));
                        bb.Transform(worldTransform);
                        DebugRenderer::DebugDraw(bb, selectedColour, true);
                    }
                }

                auto animSprite = registry.try_get<Graphics::AnimatedSprite>(m_SelectedEntity);
                if(transform && animSprite)
                {
                    {
                        auto& worldTransform = transform->GetWorldMatrix();

                        auto bb = Maths::BoundingBox(Maths::Rect(animSprite->GetPosition(), animSprite->GetPosition() + animSprite->GetScale()));
                        bb.Transform(worldTransform);
                        DebugRenderer::DebugDraw(bb, selectedColour, true);
                    }
                }

                auto camera = registry.try_get<Camera>(m_SelectedEntity);
                if(camera && transform)
                {
                    DebugRenderer::DebugDraw(camera->GetFrustum(transform->GetWorldMatrix().Inverse()), Vec4(0.9f));
                }

                auto light = registry.try_get<Graphics::Light>(m_SelectedEntity);
                if(light && transform)
                {
                    DebugRenderer::DebugDraw(light, transform->GetWorldOrientation(), Vec4(Vec3(light->Colour), 0.2f));
                }

                auto sound = registry.try_get<SoundComponent>(m_SelectedEntity);
                if(sound)
                {
                    DebugRenderer::DebugDraw(sound->GetSoundNode(), Vec4(0.8f, 0.8f, 0.8f, 0.2f));
                }

                auto phys3D = registry.try_get<RigidBody3DComponent>(m_SelectedEntity);
                if(phys3D)
                {
                    auto cs = phys3D->GetRigidBody()->GetCollisionShape();
                    if(cs)
                        cs->DebugDraw(phys3D->GetRigidBody());
                }
            }
    }

    void Editor::ExitApp()
    {
        if(m_EditorState != EditorState::Play)
        {
            SetAppState(Lumos::AppState::Closing);
        }
        else
        {
            m_QueuedScenePreviewEnd = true;
        }
    }


    void Editor::SelectObject(const Maths::Ray& ray, bool hoveredOnly)
    {
        LUMOS_PROFILE_FUNCTION();
        auto scene                  = Application::Get().GetSceneManager()->GetCurrentScene();
        auto& registry              = scene->GetRegistry();
        float closestEntityDist     = Maths::M_INFINITY;
        Entity currentClosestEntity = {};

        auto group = registry.group<Graphics::ModelComponent>(entt::get<Maths::Transform>);

        static Timer timer;
        static float timeSinceLastSelect = 0.0f;

        for(auto entity : group)
        {
            // Skip locked entities
            if(registry.any_of<EditorLockComponent>(entity))
                continue;

            const auto& [model, trans] = group.get<Graphics::ModelComponent, Maths::Transform>(entity);

            if(!model.ModelRef)
            {
                LERROR("Model is Null");
                continue;
            }

            auto& meshes = model.ModelRef->GetMeshes();

            for(auto mesh : meshes)
            {
                auto& worldTransform = trans.GetWorldMatrix();

                auto bbCopy = mesh->GetBoundingBox().Transformed(worldTransform);
                float distance;
                ray.Intersects(bbCopy, distance);

                if(distance < Maths::M_INFINITY)
                {
                    if(distance < closestEntityDist)
                    {
                        closestEntityDist    = distance;
                        currentClosestEntity = { entity, scene };
                    }
                }
            }
        }
        if(!hoveredOnly)
            if(!m_SelectedEntities.empty())
            {
                if(registry.valid(currentClosestEntity) && IsSelected(currentClosestEntity))
                {
                    if(timer.GetElapsedS() - timeSinceLastSelect < 1.0f)
                    {
                        auto& trans = registry.get<Maths::Transform>(currentClosestEntity);
                        auto& model = registry.get<Graphics::ModelComponent>(currentClosestEntity);
                        auto bb     = model.ModelRef->GetMeshes().Front()->GetBoundingBox().Transformed(trans.GetWorldMatrix());

                        FocusCamera(trans.GetWorldPosition(), Maths::Distance(bb.Max(), bb.Min()), 2.0f);
                    }
                    else
                    {
                        UnSelect(currentClosestEntity);
                    }
                }

                timeSinceLastSelect = timer.GetElapsedS();

                auto ctrl = Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || (Input::Get().GetKeyHeld(InputCode::Key::LeftControl));

                if(!ctrl)
                    m_SelectedEntities.clear();

                SetSelected(currentClosestEntity);
                return;
            }

        auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

        for(auto entity : spriteGroup)
        {
            // Skip locked entities
            if(registry.any_of<EditorLockComponent>(entity))
                continue;

            const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

            auto& worldTransform = trans.GetWorldMatrix();
            auto bb              = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
            bb.Transform(trans.GetWorldMatrix());

            float distance;
            ray.Intersects(bb, distance);
            if(distance < Maths::M_INFINITY)
            {
                if(distance < closestEntityDist)
                {
                    closestEntityDist    = distance;
                    currentClosestEntity = { entity, scene };
                }
            }
        }

        auto animSpriteGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);

        for(auto entity : animSpriteGroup)
        {
            // Skip locked entities
            if(registry.any_of<EditorLockComponent>(entity))
                continue;

            const auto& [sprite, trans] = animSpriteGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

            auto& worldTransform = trans.GetWorldMatrix();
            auto bb              = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
            bb.Transform(trans.GetWorldMatrix());
            float distance;
            ray.Intersects(bb, distance);
            if(distance < Maths::M_INFINITY)
            {
                if(distance < closestEntityDist)
                {
                    closestEntityDist    = distance;
                    currentClosestEntity = { entity, scene };
                }
            }
        }

        /*       if (hoveredOnly)
               {
                   if(IsSelected(currentClosestEntity))
                   {
                       auto& trans  = registry.get<Maths::Transform>(currentClosestEntity);
                       auto& sprite = registry.get<Graphics::Sprite>(currentClosestEntity);
                       auto bb      = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));

                       FocusCamera(trans.GetWorldPosition(), Maths::Distance(bb.Max(), bb.Min()), 2.0f);
                   }
               }*/

        if(hoveredOnly)
            SetHoveredEntity(Entity(currentClosestEntity, Application::Get().GetSceneManager()->GetCurrentScene()));
        else
            SetSelected(currentClosestEntity);
    }


    void Editor::OnDebugDraw()
    {
        // Scene view hidden: its overlays have nowhere to show. The Game view only draws debug lines when the
        // scene turns them on, so the systems' drawing (physics shapes...) is skipped too unless it does.
        if(Application::Get().GetDisableMainSceneRenderer())
        {
            Scene* scene = Application::Get().GetCurrentScene();
            if(scene && scene->GetSettings().RenderSettings.DebugRenderEnabled)
                Application::OnDebugDraw();
            return;
        }
        Application::OnDebugDraw();
        DebugDraw();
    }

    void Editor::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();

        bool isProfiling       = false;
        static bool firstFrame = true;
#if LUMOS_PROFILE
        isProfiling = tracy::GetProfiler().IsConnected();
#endif
        if(!isProfiling && m_Settings.m_SleepOutofFocus && !Application::Get().GetWindow()->GetWindowFocus() && m_EditorState != EditorState::Play && !firstFrame)
            OS::Get().Delay(1000000);

        // The scene view can leave features out. Its renderer is the one drawn here, and it reads the scene's
        // settings as it goes: they are changed for the length of this call and put back, so nothing else
        // (the game view, the Scene Settings panel, a save) ever sees the difference.
        Scene* liveScene     = Application::Get().GetCurrentScene();
        const uint32_t off   = (liveScene) ? m_Settings.m_SceneViewOff : 0;
        Scene::SceneRenderSettings keptSettings;
        if(off)
        {
            Scene::SceneRenderSettings& live = liveScene->GetSettings().RenderSettings;
            keptSettings                     = live;
            if(off & SceneViewOff_Shadows)
                live.ShadowsEnabled = live.ContactShadowsEnabled = false;
            if(off & SceneViewOff_Bloom)
                live.BloomEnabled = false;
            if(off & SceneViewOff_SSAO)
                live.SSAOEnabled = false;
            if(off & SceneViewOff_SSR)
                live.SSREnabled = false;
            if(off & SceneViewOff_AntiAliasing)
                live.FXAAEnabled = live.SMAAEnabled = live.TAAEnabled = false;
            if(off & SceneViewOff_DepthOfField)
                live.DepthOfFieldEnabled = false;
            if(off & SceneViewOff_PostEffects)
                live.DebandingEnabled = live.FilmicGrainEnabled = live.VignetteEnabled = live.ChromaticAberationEnabled = live.SharpenEnabled = live.MotionBlurEnabled = live.VolumetricFogEnabled = false;
            if(off & SceneViewOff_Skybox)
                live.SkyboxRenderEnabled = false;
        }

        Application::OnRender();

        if(off)
            liveScene->GetSettings().RenderSettings = keptSettings;

        if(m_DrawPreview)
        {
            m_PreviewDraw->Draw();
            m_PreviewDraw->DeletePreviewModel();
            m_DrawPreview = false;
        }

        if(m_SavePreviewTexture)
        {
            String8 texturePath       = PushStr8F(m_FrameArena, "%s_thumbnail.png", (const char*)m_RequestedThumbnailPath.str);
            String8 basePath          = PushStr8F(m_FrameArena, "%sAssets", Application::Get().GetProjectSettings().m_ProjectRoot.c_str());
            String8 assetCachePath    = StringUtilities::AbsolutePathToRelativeFileSystemPath(m_FrameArena, texturePath, Str8Lit("//Assets"), Str8Lit("//Assets/Cache"));
            String8 cacheAbsolutePath = StringUtilities::AbsolutePathToRelativeFileSystemPath(m_FrameArena, assetCachePath, Str8Lit("//Assets"), basePath);

            m_PreviewDraw->SaveTexture(cacheAbsolutePath, false, 0.0f);
            m_SavePreviewTexture = false;
        }

        // Save texture next frame after its finishes drawing
        if(m_QueuePreviewSave)
        {
            m_QueuePreviewSave   = false;
            m_SavePreviewTexture = true;
        }

        for(int i = 0; i < int(m_Panels.size()); i++)
        {
            m_Panels[i]->OnRender();
        }

        if(m_Settings.m_ShowGrid && !m_EditorCamera->IsOrthographic())
            Draw3DGrid();

        firstFrame = false;
    }

    void Editor::DrawPreview()
    {
        LUMOS_PROFILE_FUNCTION();
        m_DrawPreview = true;
    }

    void Editor::SavePreview()
    {
        LUMOS_PROFILE_FUNCTION();
        m_SavePreviewTexture = true;
    }

    void Editor::RequestThumbnail(String8 asset)
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_QueuePreviewSave)
            return;
        LINFO("Requesting thumbnail %s", (const char*)asset.str);
        MemorySet(m_RequestedThumbnailPath.str, 0, 256);
        MemoryCopy(m_RequestedThumbnailPath.str, asset.str, asset.size);
        m_RequestedThumbnailPath.size = asset.size;

        String8 extension = StringUtilities::Str8PathSkipLastPeriod(asset);
        if(strcmp((char*)extension.str, "lmat") == 0)
        {
            m_PreviewDraw->LoadMaterial(asset);
        }
        else
        {
            // Assume mesh
            m_PreviewDraw->LoadMesh(asset);
        }

        m_DrawPreview      = true;
        m_QueuePreviewSave = true;
    }

    void Editor::FileOpenCallback(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();

        auto path = filePath;
        path      = StringUtilities::BackSlashesToSlashes(path);

        // Normalize to VFS path for consistent hashing
        if(path.find("//") != 0)
        {
            String8 vfsPath;
            if(FileSystem::Get().AbsolutePathToFileSystem(m_FrameArena, Str8StdS(path), vfsPath, false))
                path = ToStdString(vfsPath);
        }

        if(IsTextFile(path) && m_EditorUI)
        {
            String8 physicalPath;
            if(FileSystem::Get().ResolvePhysicalPath(m_FrameArena, Str8StdS(path), &physicalPath))
                m_EditorUI->OpenScript((const char*)physicalPath.str);
        }
        else if(IsModelFile(path))
        {
            // Check if already imported (has up-to-date .meta)
            if(!AssetImporter::NeedsImport(path))
            {
                Entity modelEntity = Application::Get().GetSceneManager()->GetCurrentScene()->GetEntityManager()->Create(StringUtilities::GetFileName(path));
                modelEntity.AddComponent<Graphics::ModelComponent>(path);
                m_SelectedEntities.clear();
                SetSelected(modelEntity);
            }
            else
            {
                // Show import panel for new/stale models
                m_ImportPanel->Show(path);
                if(m_EditorUI)
                    UIDockFocusPanel(m_EditorUI->GetDockSpace(), "Import");
            }
        }
        else if(IsAudioFile(path))
        {
            String8 physicalPath;
            Lumos::FileSystem::Get().ResolvePhysicalPath(m_FrameArena, Str8StdS(path), &physicalPath);
            auto sound = Sound::Create(ToStdString(physicalPath), StringUtilities::GetFilePathExtension(path));

            auto soundNode = SharedPtr<SoundNode>(SoundNode::Create());
            soundNode->SetSound(sound);
            soundNode->SetVolume(1.0f);
            soundNode->SetPosition(Vec3(0.1f, 10.0f, 10.0f));
            soundNode->SetLooping(true);
            soundNode->SetIsGlobal(false);
            soundNode->SetPaused(false);
            soundNode->SetReferenceDistance(1.0f);
            soundNode->SetRadius(30.0f);

            Entity entity = Application::Get().GetSceneManager()->GetCurrentScene()->GetEntityManager()->Create(StringUtilities::GetFileName(path));
            entity.AddComponent<SoundComponent>(soundNode);
            entity.GetOrAddComponent<Maths::Transform>();
            SetSelected(entity);
        }
        else if(IsSceneFile(path))
        {
            int index = Application::Get().GetSceneManager()->EnqueueSceneFromFile(path.c_str());
            Application::Get().GetSceneManager()->SwitchScene(index);
        }
        else if(IsTextureFile(path))
        {
            auto entity  = Application::Get().GetSceneManager()->GetCurrentScene()->CreateEntity(StringUtilities::GetFileName(path));
            auto& sprite = entity.AddComponent<Graphics::Sprite>();
            entity.GetOrAddComponent<Maths::Transform>();

            SharedPtr<Graphics::Texture2D> texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
            sprite.SetTexture(texture);
        }
        else if(IsPrefab(path))
        {
            m_SceneManager->GetCurrentScene()->InstantiatePrefab(path);
        }
    }

    void Editor::ImportModelAsScene(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        Entity parent = EntityFactory::ImportGLTFScene(Application::Get().GetSceneManager()->GetCurrentScene(), filePath);
        if(!parent)
            return;
        m_SelectedEntities.clear();
        SetSelected(parent);
    }

    void Editor::ShowNewProjectDialog()
    {
        if(m_EditorUI)
            m_EditorUI->ShowProjectHub(true);
        else
            m_NewProjectPopupOpen = true;
    }

    const Editor::RecentProject& Editor::GetRecentProject(const std::string& lmproj)
    {
        static std::unordered_map<std::string, RecentProject> s_Info;
        auto found = s_Info.find(lmproj);
        if(found != s_Info.end())
            return found->second;

        RecentProject info;
        Export::ProjectInfo project;
        info.Missing = !project.Load(lmproj);
        info.Title   = info.Missing ? StringUtilities::RemoveFilePathExtension(StringUtilities::GetFileName(lmproj)) : project.Title;
        info.Folder  = StringUtilities::GetFileLocation(lmproj);
        if(const char* home = getenv("HOME"); home && info.Folder.rfind(home, 0) == 0)
            info.Folder = "~" + info.Folder.substr(strlen(home));

        // newest thing under Builds/
        std::error_code ec;
        std::filesystem::file_time_type newest {};
        std::string newestName;
        for(auto it = std::filesystem::directory_iterator(project.Root + "Builds", ec); !ec && it != std::filesystem::directory_iterator(); it.increment(ec))
            for(auto jt = std::filesystem::directory_iterator(it->path(), ec); !ec && jt != std::filesystem::directory_iterator(); jt.increment(ec))
            {
                auto t = std::filesystem::last_write_time(jt->path(), ec);
                if(newestName.empty() || t > newest)
                    newest = t, newestName = it->path().filename().string();
            }
        if(!newestName.empty())
        {
            auto age    = std::chrono::duration_cast<std::chrono::minutes>(std::filesystem::file_time_type::clock::now() - newest).count();
            info.Export = "Exported for " + newestName + ", " + (age < 60 ? std::to_string(age) + " min ago" : age < 60 * 48 ? std::to_string(age / 60) + " h ago" : std::to_string(age / (60 * 24)) + " days ago");
        }
        return s_Info.emplace(lmproj, info).first->second;
    }

    void Editor::RemoveRecentProject(size_t index)
    {
        if(index < m_Settings.m_RecentProjects.size())
            m_Settings.m_RecentProjects.erase(m_Settings.m_RecentProjects.begin() + (std::ptrdiff_t)index);
    }

    void Editor::CreateProject(const std::string& location, const std::string& name)
    {
        if(name.empty() || location.empty())
            return;
        m_ProjectLoadError.clear();
        std::string folder = location;
        if(folder.back() != '/')
            folder += '/';
        Application::Get().OpenNewProject(folder, name);

        if(!m_ProjectLoaded)
        {
            m_ProjectLoadError = "Could not create " + folder + name;
            return;
        }
        const std::string lmproj = folder + name + "/" + name + ".lmproj";
        if(FileSystem::FileExists(Str8StdS(lmproj)) && std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), lmproj) == m_Settings.m_RecentProjects.end())
            m_Settings.m_RecentProjects.push_back(lmproj);
        for(int i = 0; i < int(m_Panels.size()); i++)
            m_Panels[i]->OnNewProject();
    }

    std::string Editor::DefaultProjectLocation() const
    {
        // beside the project that is open, else the user's Documents
        const auto& settings = Application::Get().GetProjectSettings();
        if(m_ProjectLoaded && !settings.m_ProjectRoot.empty())
        {
            std::string root = settings.m_ProjectRoot;
            while(root.size() > 1 && root.back() == '/')
                root.pop_back();
            const size_t slash = root.find_last_of('/');
            if(slash != std::string::npos && slash > 0)
                return root.substr(0, slash);
        }
        if(const char* home = getenv("HOME"))
            return std::string(home) + "/Documents";
        return OS::Get().GetExecutablePath();
    }

#ifdef LUMOS_PLATFORM_IOS
    namespace
    {
        // Folders picked in Files (iCloud Drive, On My iPhone...) are only reachable through a bookmark after a relaunch.
        std::string BookmarkFile()
        {
            return FileSystem::UserDataPath("LumosEditor") + "/ProjectBookmarks.txt";
        }

        void SaveProjectBookmark(const std::string& folder)
        {
            const std::string bookmark = Bookmarks::Create(folder);
            if(bookmark.empty())
                return;
            if(FILE* file = fopen(BookmarkFile().c_str(), "a"))
            {
                fprintf(file, "%s\t%s\n", folder.c_str(), bookmark.c_str());
                fclose(file);
            }
        }

        // Start access again to the picked folder this project sits in. Newest entry wins.
        void RestoreProjectAccess(const std::string& projectPath)
        {
            FILE* file = fopen(BookmarkFile().c_str(), "r");
            if(!file)
                return;
            std::string found;
            static char line[8192];
            while(fgets(line, sizeof(line), file))
            {
                char* tab = strchr(line, '\t');
                if(!tab)
                    continue;
                *tab = 0;
                if(projectPath.compare(0, strlen(line), line) == 0)
                {
                    found = tab + 1;
                    while(!found.empty() && (found.back() == '\n' || found.back() == '\r'))
                        found.pop_back();
                }
            }
            fclose(file);
            if(!found.empty())
                Bookmarks::Resolve(found);
        }
    }
#endif

    void Editor::BrowseForProject()
    {
#ifdef LUMOS_PLATFORM_IOS
        // the system picker: iCloud Drive, On My iPhone, anything Files can see. Pick the project's folder.
        {
            const std::string folder = FileDialogs::PickFolder();
            if(folder.empty())
                return;
            std::string project;
            std::error_code error;
            for(const auto& entry : std::filesystem::directory_iterator(folder, error))
                if(entry.path().extension() == ".lmproj")
                    project = entry.path().string();
            if(project.empty())
            {
                m_ProjectLoadError = "No .lmproj in that folder (still downloading from iCloud?)";
                return;
            }
            SaveProjectBookmark(folder);
            ProjectOpenCallback(project);
            return;
        }
#endif
#ifdef LUMOS_PLATFORM_LINUX
        std::string path = OS::Get().GetExecutablePath() + "/../../../";
        String8 pathCopy = PushStr8Copy(m_FrameArena, path.c_str());
        pathCopy         = StringUtilities::ResolveRelativePath(m_FrameArena, pathCopy);
        path             = (const char*)pathCopy.str;
#else
        // start beside the project that is open, when there is one
        const auto& settings = Application::Get().GetProjectSettings();
        std::string path     = settings.m_ProjectRoot.empty() ? OS::Get().GetExecutablePath() : settings.m_ProjectRoot;
#endif
        {
            UIFileDialogDesc dialog;
            dialog.Title       = "Open Project";
            dialog.StartFolder = path.c_str();
            dialog.Extension   = ".lmproj";
            dialog.OnPicked    = [](const char* picked, void* user) { ((Editor*)user)->ProjectOpenCallback(picked); };
            dialog.User        = this;
            UIOpenFileDialog(dialog);
        }
    }

    void Editor::ProjectOpenCallback(const std::string& filePath)
    {
        m_ProjectLoadError.clear();
#ifdef LUMOS_PLATFORM_IOS
        RestoreProjectAccess(filePath);
#endif

        if(FileSystem::FileExists(Str8StdS(filePath)))
        {
            auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), filePath);
            if(it == m_Settings.m_RecentProjects.end())
                m_Settings.m_RecentProjects.push_back(filePath);
        }

        Application::Get().OpenProject(filePath);

        if(!m_ProjectLoaded)
        {
            m_ProjectLoadError = "Failed to load: " + filePath;
            return;
        }

        for(int i = 0; i < int(m_Panels.size()); i++)
        {
            m_Panels[i]->OnNewProject();
        }
    }

    void Editor::SaveEditorSettings()
    {
        LUMOS_PROFILE_FUNCTION();
        m_IniFile.RemoveAll();
        m_IniFile.SetOrAdd("ShowGrid", m_Settings.m_ShowGrid);
        m_IniFile.SetOrAdd("ShowGizmos", m_Settings.m_ShowGizmos);
        m_IniFile.SetOrAdd("ShowViewSelected", m_Settings.m_ShowViewSelected);
        m_IniFile.SetOrAdd("SnapAmount", m_Settings.m_SnapAmount);
        m_IniFile.SetOrAdd("SnapQuizmo", m_Settings.m_SnapQuizmo);
        m_IniFile.SetOrAdd("DebugDrawFlags", m_Settings.m_DebugDrawFlags);
        m_IniFile.SetOrAdd("PhysicsDebugDrawFlags", Application::Get().GetSystem<LumosPhysicsEngine>()->GetDebugDrawFlags());
        m_IniFile.SetOrAdd("PhysicsDebugDrawFlags2D", Application::Get().GetSystem<B2PhysicsEngine>()->GetDebugDrawFlags());
        m_IniFile.SetOrAdd("ProjectRoot", m_ProjectSettings.m_ProjectRoot);
        m_IniFile.SetOrAdd("ProjectName", m_ProjectSettings.m_ProjectName);
        m_IniFile.SetOrAdd("SleepOutofFocus", m_Settings.m_SleepOutofFocus);
        m_IniFile.SetOrAdd("UIMinimal", m_Settings.m_UIMinimal);
        m_IniFile.SetOrAdd("MaximiseOnPlay", m_Settings.m_FullScreenOnPlay);
        m_IniFile.SetOrAdd("SceneViewOff", (int)m_Settings.m_SceneViewOff);
        m_IniFile.SetOrAdd("GameFreeAspect", m_Settings.m_FreeAspect);
        m_IniFile.SetOrAdd("GameFixedAspect", m_Settings.m_FixedAspect);
        m_IniFile.SetOrAdd("SceneViewScale", m_Settings.m_SceneViewScale);
        m_IniFile.SetOrAdd("GameViewScale", m_Settings.m_GameViewScale);
        m_IniFile.SetOrAdd("GameFixedRes", m_Settings.m_GameFixedRes);
        m_IniFile.SetOrAdd("GameResW", m_Settings.m_GameResW);
        m_IniFile.SetOrAdd("GameResH", m_Settings.m_GameResH);
        m_IniFile.SetOrAdd("UISceneBarHidden", m_Settings.m_UISceneBarHidden);
        m_IniFile.SetOrAdd("UIPlayWorkspace", m_Settings.m_UIPlayWorkspace);
        m_IniFile.SetOrAdd("UIGameBarHidden", m_Settings.m_UIGameBarHidden);
        m_IniFile.SetOrAdd("UIAssetTileScale", m_Settings.m_UIAssetTileScale);
        m_IniFile.SetOrAdd("InspectorFollows", m_Settings.m_InspectorFollows);
        m_IniFile.SetOrAdd("CameraSpeed", m_Settings.m_CameraSpeed);
        m_IniFile.SetOrAdd("CameraNear", m_Settings.m_CameraNear);
        m_IniFile.SetOrAdd("CameraFar", m_Settings.m_CameraFar);

        // Save current camera to per-scene state
        {
            auto* scene = GetCurrentScene();
            if(scene && !scene->GetSceneName().empty())
            {
                const Vec3& pos     = m_EditorCameraTransform.GetLocalPosition();
                const Quat& rot     = m_EditorCameraTransform.GetLocalOrientation();
                const auto& name    = scene->GetSceneName();
                bool found          = false;
                for(size_t i = 0; i < m_Settings.m_SceneCameraStates.Size(); i++)
                {
                    if(m_Settings.m_SceneCameraStates[i].sceneName == name)
                    {
                        auto& s = m_Settings.m_SceneCameraStates[i];
                        s.posX = pos.x; s.posY = pos.y; s.posZ = pos.z;
                        s.rotX = rot.x; s.rotY = rot.y; s.rotZ = rot.z; s.rotW = rot.w;
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    EditorSettings::SceneCameraState s;
                    s.sceneName = name;
                    s.posX = pos.x; s.posY = pos.y; s.posZ = pos.z;
                    s.rotX = rot.x; s.rotY = rot.y; s.rotZ = rot.z; s.rotW = rot.w;
                    m_Settings.m_SceneCameraStates.PushBack(s);
                }
            }
        }

        // Persist per-scene camera states
        m_IniFile.SetOrAdd("SceneCamCount", int(m_Settings.m_SceneCameraStates.Size()));
        for(int i = 0; i < int(m_Settings.m_SceneCameraStates.Size()); i++)
        {
            auto& s = m_Settings.m_SceneCameraStates[i];
            std::string prefix = "SceneCam" + std::to_string(i);
            m_IniFile.SetOrAdd(prefix + "Name", s.sceneName);
            m_IniFile.SetOrAdd(prefix + "PX", s.posX);
            m_IniFile.SetOrAdd(prefix + "PY", s.posY);
            m_IniFile.SetOrAdd(prefix + "PZ", s.posZ);
            m_IniFile.SetOrAdd(prefix + "RX", s.rotX);
            m_IniFile.SetOrAdd(prefix + "RY", s.rotY);
            m_IniFile.SetOrAdd(prefix + "RZ", s.rotZ);
            m_IniFile.SetOrAdd(prefix + "RW", s.rotW);
        }

        std::sort(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end());
        m_Settings.m_RecentProjects.erase(std::unique(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end()), m_Settings.m_RecentProjects.end());
        m_IniFile.SetOrAdd("RecentProjectCount", int(m_Settings.m_RecentProjects.size()));

        for(int i = 0; i < int(m_Settings.m_RecentProjects.size()); i++)
        {
            m_IniFile.SetOrAdd("RecentProject" + std::to_string(i), m_Settings.m_RecentProjects[i]);
        }

        // Recent scenes
        std::sort(m_Settings.m_RecentScenes.begin(), m_Settings.m_RecentScenes.end());
        m_Settings.m_RecentScenes.erase(std::unique(m_Settings.m_RecentScenes.begin(), m_Settings.m_RecentScenes.end()), m_Settings.m_RecentScenes.end());
        m_IniFile.SetOrAdd("RecentSceneCount", int(m_Settings.m_RecentScenes.size()));
        for(int i = 0; i < int(m_Settings.m_RecentScenes.size()); i++)
        {
            m_IniFile.SetOrAdd("RecentScene" + std::to_string(i), m_Settings.m_RecentScenes[i]);
        }

        m_IniFile.Rewrite();
    }

    void Editor::AddDefaultEditorSettings()
    {
        LUMOS_PROFILE_FUNCTION();
        LINFO("Setting default editor settings");
        m_ProjectSettings.m_ProjectRoot = "../../ExampleProject/";

#ifdef LUMOS_PLATFORM_MACOS
        {
            std::string execPath = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath());
            std::string bundlePath = execPath + "../Resources/ExampleProject/";

            if(Lumos::FileSystem::FolderExists(Str8StdS(bundlePath)))
            {
                // Sandboxed/distributed app: copy bundled project to writable Application Support
                const char* home = std::getenv("HOME");
                std::string appSupport = std::string(home ? home : "/tmp") + "/Library/Application Support/LumosEditor/ExampleProject/";
                std::string projectFile = appSupport + "Example.lmproj";

                std::error_code ec;
                if(!std::filesystem::exists(projectFile, ec))
                {
                    LINFO("Copying bundled ExampleProject to %s", appSupport.c_str());
                    std::filesystem::create_directories(appSupport, ec);
                    std::filesystem::copy(bundlePath, appSupport, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
                    if(ec)
                        LWARN("Failed to copy ExampleProject: %s", ec.message().c_str());
                }
                m_ProjectSettings.m_ProjectRoot = appSupport;
            }
            else
            {
                // Dev build: relative to build output
                m_ProjectSettings.m_ProjectRoot = execPath + "../../../../../ExampleProject/";
                if(!Lumos::FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_ProjectRoot)))
                {
                    m_ProjectSettings.m_ProjectRoot = execPath + "/ExampleProject/";
                    if(!Lumos::FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_ProjectRoot)))
                    {
                        m_ProjectSettings.m_ProjectRoot = "../../ExampleProject/";
                    }
                }
            }
        }
#elif defined(LUMOS_PLATFORM_IOS)
        {
            // Bundle is read-only on iOS — copy ExampleProject to Documents on first run
            std::string bundlePath  = OS::Get().GetAssetPath() + BundledProjectFolder();
            std::string docsDir     = OS::Get().GetCurrentWorkingDirectory() + "/LumosEditor/" + BundledProjectFolder();
            std::string projectFile = docsDir + "/" + BundledProjectFile();

            StartBundleCopy(bundlePath, docsDir);

            m_ProjectSettings.m_ProjectRoot = docsDir + "/";
        }
#elif defined(LUMOS_PLATFORM_LINUX)
        m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "/../../ExampleProject/";
#endif

        m_ProjectSettings.m_ProjectName = "Example";

        m_IniFile.Add("ShowGrid", m_Settings.m_ShowGrid);
        m_IniFile.Add("ShowGizmos", m_Settings.m_ShowGizmos);
        m_IniFile.Add("ShowViewSelected", m_Settings.m_ShowViewSelected);
        m_IniFile.Add("TransitioningCamera", m_TransitioningCamera);
        m_IniFile.Add("SnapAmount", m_Settings.m_SnapAmount);
        m_IniFile.Add("SnapQuizmo", m_Settings.m_SnapQuizmo);
        m_IniFile.Add("DebugDrawFlags", m_Settings.m_DebugDrawFlags);
        m_IniFile.Add("PhysicsDebugDrawFlags", 0);
        m_IniFile.Add("PhysicsDebugDrawFlags2D", 0);
        m_IniFile.Add("ProjectRoot", m_ProjectSettings.m_ProjectRoot);
        m_IniFile.Add("ProjectName", m_ProjectSettings.m_ProjectName);
        m_IniFile.Add("SleepOutofFocus", m_Settings.m_SleepOutofFocus);
        m_IniFile.Add("InspectorFollows", m_Settings.m_InspectorFollows);
        m_IniFile.Add("RecentProjectCount", 0);
        m_IniFile.Add("CameraSpeed", m_Settings.m_CameraSpeed);
        m_IniFile.Add("CameraNear", m_Settings.m_CameraNear);
        m_IniFile.Add("CameraFar", m_Settings.m_CameraFar);
        m_IniFile.Add("SceneCamCount", 0);
        m_IniFile.Add("RecentSceneCount", 0);

        m_IniFile.Rewrite();
    }

    void Editor::LoadEditorSettings()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Settings.m_ShowGrid         = m_IniFile.GetOrDefault("ShowGrid", m_Settings.m_ShowGrid);
        m_Settings.m_ShowGizmos       = m_IniFile.GetOrDefault("ShowGizmos", m_Settings.m_ShowGizmos);
        m_Settings.m_ShowViewSelected = m_IniFile.GetOrDefault("ShowViewSelected", m_Settings.m_ShowViewSelected);
        // a fly-to that was under way when the settings were saved must not lock the camera on the next start
        m_TransitioningCamera         = false;
        m_Settings.m_SnapAmount       = m_IniFile.GetOrDefault("SnapAmount", m_Settings.m_SnapAmount);
        m_Settings.m_SnapQuizmo       = m_IniFile.GetOrDefault("SnapQuizmo", m_Settings.m_SnapQuizmo);
        m_Settings.m_DebugDrawFlags   = m_IniFile.GetOrDefault("DebugDrawFlags", m_Settings.m_DebugDrawFlags);

        m_ProjectSettings.m_ProjectRoot = m_IniFile.GetOrDefault("ProjectRoot", std::string("../../ExampleProject/"));
        m_ProjectSettings.m_ProjectName = m_IniFile.GetOrDefault("ProjectName", std::string("Example"));
        // --project=<folder or .lmproj> opens that instead of the last one
        {
            String8 opt = Internal::CoreSystem::GetCmdLine()->OptionString(Str8Lit("project"));
            Export::ProjectInfo project;
            if(opt.size > 0 && project.Load(std::string((const char*)opt.str, opt.size)))
            {
                m_ProjectSettings.m_ProjectRoot = project.Root;
                m_ProjectSettings.m_ProjectName = project.Name;
            }
        }

        ArenaTemp arena                 = ScratchBegin(nullptr, 0);
        String8 pathCopy                = PushStr8Copy(arena.arena, m_ProjectSettings.m_ProjectRoot.c_str());
        pathCopy                        = StringUtilities::ResolveRelativePath(arena.arena, pathCopy);
        m_ProjectSettings.m_ProjectRoot = (const char*)pathCopy.str;
        ScratchEnd(arena);

        m_Settings.m_Physics2DDebugFlags = m_IniFile.GetOrDefault("PhysicsDebugDrawFlags2D", 0);
        m_Settings.m_Physics3DDebugFlags = m_IniFile.GetOrDefault("PhysicsDebugDrawFlags", 0);
        m_Settings.m_SleepOutofFocus     = m_IniFile.GetOrDefault("SleepOutofFocus", true);
#if defined(LUMOS_PLATFORM_IOS)
        // the touch layout only exists on the engine UI
#endif
        m_Settings.m_UIMinimal           = m_IniFile.GetOrDefault("UIMinimal", false);
        m_Settings.m_FullScreenOnPlay    = m_IniFile.GetOrDefault("MaximiseOnPlay", false);
        m_Settings.m_SceneViewOff        = (uint32_t)m_IniFile.GetOrDefault("SceneViewOff", 0) & SceneViewOff_All;
        m_Settings.m_FreeAspect          = m_IniFile.GetOrDefault("GameFreeAspect", true);
        m_Settings.m_FixedAspect         = m_IniFile.GetOrDefault("GameFixedAspect", 16.0f / 9.0f);
        m_Settings.m_SceneViewScale      = Maths::Clamp((float)m_IniFile.GetOrDefault("SceneViewScale", 1.0f), 0.25f, 2.0f);
        m_Settings.m_GameViewScale       = Maths::Clamp((float)m_IniFile.GetOrDefault("GameViewScale", 1.0f), 0.25f, 2.0f);
        m_Settings.m_GameFixedRes        = m_IniFile.GetOrDefault("GameFixedRes", false);
        m_Settings.m_GameResW            = Maths::Clamp((int)m_IniFile.GetOrDefault("GameResW", 1920), 64, 8192);
        m_Settings.m_GameResH            = Maths::Clamp((int)m_IniFile.GetOrDefault("GameResH", 1080), 64, 8192);
        m_Settings.m_UISceneBarHidden    = m_IniFile.GetOrDefault("UISceneBarHidden", false);
        m_Settings.m_UIPlayWorkspace     = m_IniFile.GetOrDefault("UIPlayWorkspace", false);
        m_Settings.m_UIGameBarHidden     = m_IniFile.GetOrDefault("UIGameBarHidden", false);
        m_Settings.m_UIAssetTileScale    = m_IniFile.GetOrDefault("UIAssetTileScale", 1.0f);
        m_Settings.m_InspectorFollows    = m_IniFile.GetOrDefault("InspectorFollows", true);
        m_Settings.m_CameraSpeed         = m_IniFile.GetOrDefault("CameraSpeed", 1000.0f);
        m_Settings.m_CameraNear          = m_IniFile.GetOrDefault("CameraNear", 0.01f);
        m_Settings.m_CameraFar           = m_IniFile.GetOrDefault("CameraFar", 1000.0f);

        // Per-scene camera states
        {
            int camCount = m_IniFile.GetOrDefault("SceneCamCount", 0);
            for(int i = 0; i < camCount; i++)
            {
                std::string prefix = "SceneCam" + std::to_string(i);
                std::string name   = m_IniFile.GetOrDefault(prefix + "Name", std::string());
                if(!name.empty())
                {
                    EditorSettings::SceneCameraState s;
                    s.sceneName = name;
                    s.posX = m_IniFile.GetOrDefault(prefix + "PX", -31.0f);
                    s.posY = m_IniFile.GetOrDefault(prefix + "PY", 12.0f);
                    s.posZ = m_IniFile.GetOrDefault(prefix + "PZ", 51.0f);
                    s.rotX = m_IniFile.GetOrDefault(prefix + "RX", 0.0f);
                    s.rotY = m_IniFile.GetOrDefault(prefix + "RY", 0.0f);
                    s.rotZ = m_IniFile.GetOrDefault(prefix + "RZ", 0.0f);
                    s.rotW = m_IniFile.GetOrDefault(prefix + "RW", 1.0f);
                    m_Settings.m_SceneCameraStates.PushBack(s);
                }
            }
        }

        m_EditorCameraController.SetSpeed(m_Settings.m_CameraSpeed);

        int recentProjectCount  = 0;
        std::string projectPath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");

        if(FileSystem::FileExists(Str8StdS(projectPath)))
        {
            auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), projectPath);
            if(it == m_Settings.m_RecentProjects.end())
                m_Settings.m_RecentProjects.push_back(projectPath);
        }

        recentProjectCount = m_IniFile.GetOrDefault("RecentProjectCount", 0);
        for(int i = 0; i < recentProjectCount; i++)
        {
            projectPath = m_IniFile.GetOrDefault("RecentProject" + std::to_string(i), std::string());

            if(FileSystem::FileExists(Str8StdS(projectPath)))
            {
                auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), projectPath);
                if(it == m_Settings.m_RecentProjects.end())
                    m_Settings.m_RecentProjects.push_back(projectPath);
            }
        }

        std::sort(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end());
        m_Settings.m_RecentProjects.erase(std::unique(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end()), m_Settings.m_RecentProjects.end());

        // Recent scenes
        int recentSceneCount = m_IniFile.GetOrDefault("RecentSceneCount", 0);
        for(int i = 0; i < recentSceneCount; i++)
        {
            std::string scenePath = m_IniFile.GetOrDefault("RecentScene" + std::to_string(i), std::string());
            if(!scenePath.empty())
            {
                auto it = std::find(m_Settings.m_RecentScenes.begin(), m_Settings.m_RecentScenes.end(), scenePath);
                if(it == m_Settings.m_RecentScenes.end())
                    m_Settings.m_RecentScenes.push_back(scenePath);
            }
        }
    }

    SharedPtr<Graphics::Texture2D> Editor::GetPreviewTexture() const
    {
        return m_PreviewDraw->m_PreviewTexture;
    }

    const char* Editor::GetIconFontIcon(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        if(IsTextFile(filePath))
        {
            return ICON_MDI_FILE_XML;
        }
        else if(IsModelFile(filePath))
        {
            return ICON_MDI_SHAPE;
        }
        else if(IsAudioFile(filePath))
        {
            return ICON_MDI_FILE_MUSIC;
        }
        else if(IsTextureFile(filePath))
        {
            return ICON_MDI_FILE_IMAGE;
        }

        return ICON_MDI_FILE;
    }

    void Editor::CreateGridRenderer()
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_GridRenderer)
            m_GridRenderer = CreateSharedPtr<Graphics::GridRenderer>(uint32_t(Application::Get().m_SceneViewWidth), uint32_t(Application::Get().m_SceneViewHeight));
    }

    const SharedPtr<Graphics::GridRenderer>& Editor::GetGridRenderer()
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_GridRenderer)
            m_GridRenderer = CreateSharedPtr<Graphics::GridRenderer>(uint32_t(Application::Get().m_SceneViewWidth), uint32_t(Application::Get().m_SceneViewHeight));
        return m_GridRenderer;
    }

    // Like the Runtime: a project's App Script (tools / apps such as LumosTunes) runs on an entity of
    // its own. Added to the play copy only - stopping reloads the cached scene, which drops it again.
    void Editor::AddAppScriptEntity()
    {
        auto& settings = Application::Get().GetProjectSettings();
        Scene* scene   = Application::Get().GetCurrentScene();
        if(settings.AppScript.empty() || !scene)
            return;
        for(auto entity : scene->GetRegistry().view<LuaScriptComponent>())
            if(scene->GetRegistry().get<LuaScriptComponent>(entity).GetFilePath() == settings.AppScript)
                return; // the scene already carries it
        Entity e = scene->CreateEntity(settings.Title.empty() ? std::string("App Script") : settings.Title);
        e.AddComponent<LuaScriptComponent>(std::string(settings.AppScript), scene);
    }

    void Editor::CacheScene()
    {
        LUMOS_PROFILE_FUNCTION();
        Application::Get().GetCurrentScene()->Serialise(m_TempSceneSaveFilePath, false);
    }

    void Editor::LoadCachedScene()
    {
        LUMOS_PROFILE_FUNCTION();

        if(FileSystem::FileExists(Str8StdS(std::string(m_TempSceneSaveFilePath + Application::Get().GetCurrentScene()->GetSceneName() + ".lsn"))))
        {
            Application::Get().GetCurrentScene()->Deserialise(m_TempSceneSaveFilePath, false);
        }
        else
        {
            String8 physicalPath;
            if(Lumos::FileSystem::Get().ResolvePhysicalPath(m_FrameArena, Str8StdS(std::string("//Assets/Scenes/" + Application::Get().GetCurrentScene()->GetSceneName() + ".lsn")), &physicalPath))
            {
                auto newPath = StringUtilities::RemoveName(ToStdString(physicalPath));
                Application::Get().GetCurrentScene()->Deserialise(newPath, false);
            }
        }
    }

    void Editor::SetEditorScriptsPath(const std::string& path)
    {
        m_EditorScriptPath = PushStr8Copy(m_Arena, path.c_str());
    }
}
