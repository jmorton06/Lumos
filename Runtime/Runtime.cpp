#include <string>
#include <Lumos/Core/Application.h>
#ifdef LUMOS_PLATFORM_IOS
#include <Lumos/Platform/iOS/iOSOS.h>
#endif
#include <Lumos/Core/EntryPoint.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/Font.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Maths/Vector4.h>
#include <Lumos/Maths/Matrix4.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Core/CommandLine.h>
#include <Lumos/Core/CoreSystem.h>
#include <Lumos/Core/Asset/AssetPacker.h>
#include <Lumos/Core/Core.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <imgui/imgui.h>
#include <sol/sol.hpp>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>  // For GetFullPathName
#endif

using namespace Lumos;

// Find .lmproj in a directory
static std::string FindLmproj(const std::string& dir)
{
    // TODO: proper directory scan. For now, try common patterns.
    // The project name usually matches the folder name.
    std::string folder = dir;
    if(!folder.empty() && (folder.back() == '/' || folder.back() == '\\'))
        folder.pop_back();

    // Extract folder name
    size_t sep = folder.find_last_of("/\\");
    std::string name = (sep != std::string::npos) ? folder.substr(sep + 1) : folder;

    std::string path = dir;
    if(path.back() != '/' && path.back() != '\\')
        path += "/";

    std::string candidate = path + name + ".lmproj";
    if(FileSystem::FileExists(Str8StdS(candidate)))
        return candidate;

    return "";
}

class Runtime : public Application
{
    friend class Application;

public:
    explicit Runtime()
        : Application()
    {
        Application::SetInstance(this);
    }

    ~Runtime()
    {
    }

    void OnUpdate(const TimeStep& dt) override
    {
        Application::OnUpdate(dt);
    }

    void OnEvent(Event& e) override
    {
        Application::OnEvent(e);
    }

    void Init() override
    {
        // Check --project=path CLI arg
        CommandLine* cmdline = Internal::CoreSystem::GetCmdLine();
        String8 projectOpt      = cmdline->OptionString(Str8Lit("project"));
        bool projectMode        = projectOpt.size > 0;

        if(projectMode)
        {
            std::string projectDir((const char*)projectOpt.str, projectOpt.size);
            std::string projFile = FindLmproj(projectDir);

            if(!projFile.empty())
            {
#ifdef LUMOS_PLATFORM_WINDOWS
				char resolved[MAX_PATH]; // Use MAX_PATH for Windows
				std::string projRoot = StringUtilities::GetFileLocation(projFile);
				DWORD result = GetFullPathNameA(projRoot.c_str(), MAX_PATH, resolved, NULL);
				if(result > 0)
				{
					projRoot = std::string(resolved) + "\\";
				}
				else
				{
					LERROR("GetFullPathName error");
				}
#else
                // Resolve to absolute path so it works regardless of CWD changes
                char resolved[PATH_MAX];
                std::string projRoot = StringUtilities::GetFileLocation(projFile);
                if(realpath(projRoot.c_str(), resolved))
                    projRoot = std::string(resolved) + "/";
#endif

                LINFO("Loading tool project: %s", projFile.c_str());
                m_ProjectSettings.m_ProjectRoot = projRoot;

                // Extract project name from filename
                std::string filename = projFile.substr(projFile.find_last_of("/\\") + 1);
                size_t dot           = filename.find_last_of('.');
                m_ProjectSettings.m_ProjectName = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
            }
            else
            {
                LERROR("No .lmproj found in project dir: %s", std::string((const char*)projectOpt.str, projectOpt.size).c_str());
            }
        }
#ifdef LUMOS_BUNDLED_PROJECT_NAME
        else
        {
            // Standalone bundled app — project is embedded as a bundle resource
            const char* bundledName = LUMOS_BUNDLED_PROJECT_NAME;

            #ifdef LUMOS_PLATFORM_IOS
            m_ProjectSettings.m_ProjectRoot = OS::Get().GetAssetPath() + "/" + bundledName + "/";
            #else
            std::string exeDir = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath());
            m_ProjectSettings.m_ProjectRoot = exeDir + "../Resources/" + bundledName + "/";
            if(!FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_ProjectRoot)))
                m_ProjectSettings.m_ProjectRoot = exeDir + bundledName + "/";
            #endif

            // Extract project name from folder
            std::string projFile = FindLmproj(m_ProjectSettings.m_ProjectRoot);
            if(!projFile.empty())
            {
                std::string filename = projFile.substr(projFile.find_last_of("/\\") + 1);
                size_t dot = filename.find_last_of('.');
                m_ProjectSettings.m_ProjectName = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
            }
            else
            {
                m_ProjectSettings.m_ProjectName = bundledName;
            }
            LINFO("Bundled project: %s at %s", m_ProjectSettings.m_ProjectName.c_str(), m_ProjectSettings.m_ProjectRoot.c_str());
        }
#else
        else
        {
            // Default: find ExampleProject
            TDArray<std::string> projectLocations = {
                OS::Get().GetExecutablePath() + "../../../../../ExampleProject/",
                "/Users/jmorton/Dev/Lumos/ExampleProject/Example.lmproj",
                "ExampleProject/Example.lmproj",
                "../ExampleProject/Example.lmproj",
                OS::Get().GetExecutablePath() + "/ExampleProject/Example.lmproj",
                OS::Get().GetExecutablePath() + "/../ExampleProject/Example.lmproj",
                OS::Get().GetExecutablePath() + "/../../ExampleProject/Example.lmproj"
            };

#if defined(LUMOS_PLATFORM_IOS)
            projectLocations.Clear();
#endif

            for(auto& path : projectLocations)
            {
                if(FileSystem::FileExists(Str8StdS(path)))
                {
                    LINFO("Loaded Project %s", path.c_str());
                    m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(path);
                    m_ProjectSettings.m_ProjectName = "Example";
                    break;
                }
            }

#ifdef LUMOS_PLATFORM_MACOS
            m_ProjectSettings.m_ProjectName = "Example";
            m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "../../../../../ExampleProject/";
            if(!FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_ProjectRoot)))
            {
                m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "/ExampleProject/";
                if(!FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_ProjectRoot)))
                {
                    m_ProjectSettings.m_ProjectRoot = "../../ExampleProject/";
                }
            }
#elif defined(LUMOS_PLATFORM_IOS)
            {
                // Bundle is read-only — copy ExampleProject to Documents on first run
                std::string bundlePath  = OS::Get().GetAssetPath() + "ExampleProject";
                std::string docsDir     = OS::Get().GetCurrentWorkingDirectory() + "/LumosEditor/ExampleProject";
                std::string projectFile = docsDir + "/Example.lmproj";

                if(!FileSystem::FileExists(Str8StdS(projectFile)))
                    iOSOS::CopyBundleFolder(bundlePath, docsDir);

                m_ProjectSettings.m_ProjectRoot = docsDir + "/";
            }
            m_ProjectSettings.m_ProjectName = "Example";
#endif
        }
#endif

        // Handle --pack-assets=path (headless packing mode)
        {
            CommandLine* cmd = Internal::CoreSystem::GetCmdLine();
            String8 packAssetsOpt = cmd->OptionString(Str8Lit("pack-assets"));
            if(packAssetsOpt.size > 0)
            {
                std::string assetsDir = m_ProjectSettings.m_ProjectRoot + "Assets/";
                std::string outputPath((const char*)packAssetsOpt.str, packAssetsOpt.size);
                PackSettings settings;
                settings.ExcludePaths.PushBack(Str8Lit("Cache/"));
                settings.ExcludePaths.PushBack(Str8Lit("Imported/"));
                bool ok = AssetPacker::Pack(Str8StdS(outputPath), Str8StdS(assetsDir), settings);
                LINFO("Pack %s: %s", ok ? "succeeded" : "failed", outputPath.c_str());
                exit(ok ? 0 : 1);
            }
        }

        Application::Init();
        Application::SetEditorState(EditorState::Play);

        // Handle --scene flag for Runtime
        {
            CommandLine* cmd = Internal::CoreSystem::GetCmdLine();
            String8 sceneOpt = cmd->OptionString(Str8Lit("scene"));
            if(sceneOpt.size > 0)
            {
                std::string sceneName((const char*)sceneOpt.str, sceneOpt.size);
                auto& scenes = GetSceneManager()->GetScenes();
                for(size_t i = 0; i < scenes.Size(); i++)
                {
                    if(scenes[i]->GetSceneName() == sceneName)
                    {
                        GetSceneManager()->SwitchScene((int)i);
                        break;
                    }
                }
            }
        }

        // If AppScript is set in the project, create a script entity
        if(!m_ProjectSettings.AppScript.empty())
        {
            LINFO("Project Script : %s", m_ProjectSettings.AppScript.c_str());

            if(GetSceneManager()->GetScenes().Empty())
            {
                LINFO("Adding Empty Scene");
                Scene* scene = new Scene("Empty Scene");

                GetSceneManager()->EnqueueScene(scene);
                GetSceneManager()->SwitchScene(0);
            }

            if(GetSceneManager()->GetScenes()[0])
            {
                LINFO("Adding Script To Scene");

                Entity scriptEntity = GetSceneManager()->GetScenes()[0]->CreateEntity(m_ProjectSettings.Title);
                scriptEntity.AddComponent<LuaScriptComponent>(std::string(m_ProjectSettings.AppScript),  GetSceneManager()->GetScenes()[0].get());
            }

            GetSceneManager()->ApplySceneSwitch();
        }

        Application::Get().GetWindow()->SetWindowTitle(m_ProjectSettings.Title);
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Runtime::OnEvent));
    }

    void OnImGui() override
    {
        static bool bShowMetricsWindow = false;
        if(bShowMetricsWindow)
        {
            ImGui::Begin("Metrics");
            ImGui::Text("FPS : %.2f", (float)Engine::Get().Statistics().FramesPerSecond);
            ImGui::Text("Scene : %s", GetSceneManager()->GetCurrentScene()->GetSceneName().c_str());

            i32 sceneIndex = GetSceneManager()->GetCurrentSceneIndex();
            if(ImGui::InputInt("Scene Index", &sceneIndex))
                GetSceneManager()->SwitchScene(sceneIndex);
            ImGui::End();
        }

        Application::OnImGui();
    }
};

Lumos::Application* Lumos::CreateApplication()
{
    return new ::Runtime();
}
