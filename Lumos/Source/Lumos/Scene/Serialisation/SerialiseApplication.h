#pragma once
#include "Core/Application.h"
namespace Lumos
{
    template <typename Archive>
    void save(Archive& archive, const Application& application)
    {
        int projectVersion = 8;

        archive(cereal::make_nvp("Project Version", projectVersion));

        // Version 1

        std::string path;

        // Window size and full screen shouldnt be in project

        // Version 8 removed width and height
        archive(cereal::make_nvp("RenderAPI", application.m_ProjectSettings.RenderAPI),
                cereal::make_nvp("Fullscreen", application.m_ProjectSettings.Fullscreen),
                cereal::make_nvp("VSync", application.m_ProjectSettings.VSync),
                cereal::make_nvp("ShowConsole", application.m_ProjectSettings.ShowConsole),
                cereal::make_nvp("Title", application.m_ProjectSettings.Title));
        // Version 2

        const auto& paths = application.m_SceneManager->GetSceneFilePaths();
        std::vector<std::string> newPaths;
        for(auto& path : paths)
        {
            std::string newPath;
            FileSystem::Get().AbsolutePathToFileSystem((const char*)path.str, newPath);
            newPaths.push_back((const char*)path.str);
        }
        archive(cereal::make_nvp("Scenes", newPaths));
        // Version 3
        archive(cereal::make_nvp("SceneIndex", application.m_SceneManager->GetCurrentSceneIndex()));
        // Version 4
        archive(cereal::make_nvp("Borderless", application.m_ProjectSettings.Borderless));
        // Version 5
        archive(cereal::make_nvp("EngineAssetPath", application.m_ProjectSettings.m_EngineAssetPath));
        // Version 6
        archive(cereal::make_nvp("GPUIndex", application.m_ProjectSettings.DesiredGPUIndex));
    }

    template <typename Archive>
    void load(Archive& archive, Application& application)
    {
        int sceneIndex = 0;
        archive(cereal::make_nvp("Project Version", application.m_ProjectSettings.ProjectVersion));

        std::string test;
        if(application.m_ProjectSettings.ProjectVersion < 8)
        {
            archive(cereal::make_nvp("RenderAPI", application.m_ProjectSettings.RenderAPI),
                    cereal::make_nvp("Width", application.m_ProjectSettings.Width),
                    cereal::make_nvp("Height", application.m_ProjectSettings.Height),
                    cereal::make_nvp("Fullscreen", application.m_ProjectSettings.Fullscreen),
                    cereal::make_nvp("VSync", application.m_ProjectSettings.VSync),
                    cereal::make_nvp("ShowConsole", application.m_ProjectSettings.ShowConsole),
                    cereal::make_nvp("Title", application.m_ProjectSettings.Title));
        }
        else
        {
            archive(cereal::make_nvp("RenderAPI", application.m_ProjectSettings.RenderAPI),
                    cereal::make_nvp("Fullscreen", application.m_ProjectSettings.Fullscreen),
                    cereal::make_nvp("VSync", application.m_ProjectSettings.VSync),
                    cereal::make_nvp("ShowConsole", application.m_ProjectSettings.ShowConsole),
                    cereal::make_nvp("Title", application.m_ProjectSettings.Title));
        }
        if(application.m_ProjectSettings.ProjectVersion > 2)
        {
            std::vector<std::string> sceneFilePaths;
            archive(cereal::make_nvp("Scenes", sceneFilePaths));

            for(auto& filePath : sceneFilePaths)
            {
                application.m_SceneManager->AddFileToLoadList(filePath.c_str());
            }

            if(sceneFilePaths.size() == sceneIndex)
                application.AddDefaultScene();
        }
        if(application.m_ProjectSettings.ProjectVersion > 3)
        {
            archive(cereal::make_nvp("SceneIndex", sceneIndex));
            application.m_SceneManager->SwitchScene(sceneIndex);
        }
        if(application.m_ProjectSettings.ProjectVersion > 4)
        {
            archive(cereal::make_nvp("Borderless", application.m_ProjectSettings.Borderless));
        }

        if(application.m_ProjectSettings.ProjectVersion > 5)
        {
            archive(cereal::make_nvp("EngineAssetPath", application.m_ProjectSettings.m_EngineAssetPath));
        }
        else
            application.m_ProjectSettings.m_EngineAssetPath = "/Users/jmorton/dev/Lumos/Lumos/Assets/";

        if(application.m_ProjectSettings.ProjectVersion > 6)
            archive(cereal::make_nvp("GPUIndex", application.m_ProjectSettings.DesiredGPUIndex));
    }
}
