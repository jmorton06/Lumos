#pragma once
#include "Core/Application.h"
namespace Lumos
{
    template <typename Archive>
    void save(Archive& archive, const Application& application)
    {
        int projectVersion = 12;

        archive(cereal::make_nvp("Project Version", projectVersion));

        // Version 1

        std::string path;

        // Version 8 removed width and height, version 9 restores them (needed for tools)
        archive(cereal::make_nvp("RenderAPI", application.m_ProjectSettings.RenderAPI),
                cereal::make_nvp("Fullscreen", application.m_ProjectSettings.Fullscreen),
                cereal::make_nvp("VSync", application.m_ProjectSettings.VSync),
                cereal::make_nvp("ShowConsole", application.m_ProjectSettings.ShowConsole),
                cereal::make_nvp("Title", application.m_ProjectSettings.Title));
        // Version 2

        ArenaTemp Scratch = ScratchBegin(0, 0);

        const auto& paths = application.m_SceneManager->GetSceneFilePaths();
        std::vector<String8> newPaths;
        for(auto& path : paths)
        {
            String8 newPath;
            FileSystem::Get().AbsolutePathToFileSystem(Scratch.arena, path, newPath);
            newPaths.push_back(newPath);
            ;
        }
        archive(cereal::make_nvp("Scenes", newPaths));
        // Version 3
        archive(cereal::make_nvp("SceneIndex", application.m_SceneManager->GetCurrentSceneIndex()));
        // Version 4
        archive(cereal::make_nvp("Borderless", application.m_ProjectSettings.Borderless));
        // Version 5 - EngineAssetPath computed at runtime, write empty for compat
        {
            std::string empty;
            archive(cereal::make_nvp("EngineAssetPath", empty));
        }
        // Version 6
        archive(cereal::make_nvp("GPUIndex", application.m_ProjectSettings.DesiredGPUIndex));
        // Version 9 - tool fields (ignored by games)
        archive(cereal::make_nvp("Width", application.m_ProjectSettings.Width),
                cereal::make_nvp("Height", application.m_ProjectSettings.Height),
                cereal::make_nvp("AppScript", application.m_ProjectSettings.AppScript));
        // Version 10 - distribution fields
        archive(cereal::make_nvp("BundleIdentifier", application.m_ProjectSettings.BundleIdentifier),
                cereal::make_nvp("Version", application.m_ProjectSettings.Version),
                cereal::make_nvp("BuildNumber", application.m_ProjectSettings.BuildNumber),
                cereal::make_nvp("IconPath", application.m_ProjectSettings.IconPath));
        // Version 10 - splash screen
        archive(cereal::make_nvp("SplashPath", application.m_ProjectSettings.SplashPath),
                cereal::make_nvp("SplashBGR", application.m_ProjectSettings.SplashBGColour[0]),
                cereal::make_nvp("SplashBGG", application.m_ProjectSettings.SplashBGColour[1]),
                cereal::make_nvp("SplashBGB", application.m_ProjectSettings.SplashBGColour[2]),
                cereal::make_nvp("SplashBGA", application.m_ProjectSettings.SplashBGColour[3]));
        // Version 11 - start scene by name
        archive(cereal::make_nvp("StartScene", application.m_ProjectSettings.StartScene));
        // Version 12 - auto-import meshes
        archive(cereal::make_nvp("AutoImportMeshes", application.m_ProjectSettings.AutoImportMeshes));
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
            std::vector<String8> sceneFilePaths;
            archive(cereal::make_nvp("Scenes", sceneFilePaths));

            for(auto& filePath : sceneFilePaths)
            {
                application.m_SceneManager->AddFileToLoadList((const char*)filePath.str);
            }

            if(sceneFilePaths.size() == sceneIndex)
                application.AddDefaultScene();
        }
        if(application.m_ProjectSettings.ProjectVersion > 3)
        {
            archive(cereal::make_nvp("SceneIndex", sceneIndex));
        }
        if(application.m_ProjectSettings.ProjectVersion > 4)
        {
            archive(cereal::make_nvp("Borderless", application.m_ProjectSettings.Borderless));
        }

        if(application.m_ProjectSettings.ProjectVersion > 5)
        {
            // Read and discard - EngineAssetPath computed at runtime via SetEngineAssetPath()
            std::string ignored;
            archive(cereal::make_nvp("EngineAssetPath", ignored));
        }

        if(application.m_ProjectSettings.ProjectVersion > 6)
            archive(cereal::make_nvp("GPUIndex", application.m_ProjectSettings.DesiredGPUIndex));

        // Version 9 - tool fields
        if(application.m_ProjectSettings.ProjectVersion > 8)
        {
            archive(cereal::make_nvp("Width", application.m_ProjectSettings.Width),
                    cereal::make_nvp("Height", application.m_ProjectSettings.Height),
                    cereal::make_nvp("AppScript", application.m_ProjectSettings.AppScript));
        }
        // Version 10 - distribution fields
        if(application.m_ProjectSettings.ProjectVersion > 9)
        {
            archive(cereal::make_nvp("BundleIdentifier", application.m_ProjectSettings.BundleIdentifier),
                    cereal::make_nvp("Version", application.m_ProjectSettings.Version),
                    cereal::make_nvp("BuildNumber", application.m_ProjectSettings.BuildNumber),
                    cereal::make_nvp("IconPath", application.m_ProjectSettings.IconPath));

            // Splash screen (added to v10, optional for older v10 files)
            try
            {
                archive(cereal::make_nvp("SplashPath", application.m_ProjectSettings.SplashPath),
                        cereal::make_nvp("SplashBGR", application.m_ProjectSettings.SplashBGColour[0]),
                        cereal::make_nvp("SplashBGG", application.m_ProjectSettings.SplashBGColour[1]),
                        cereal::make_nvp("SplashBGB", application.m_ProjectSettings.SplashBGColour[2]),
                        cereal::make_nvp("SplashBGA", application.m_ProjectSettings.SplashBGColour[3]));
            }
            catch(...)
            {
            }
        }

        // Version 11 - start scene by name
        if(application.m_ProjectSettings.ProjectVersion > 10)
        {
            try
            {
                archive(cereal::make_nvp("StartScene", application.m_ProjectSettings.StartScene));
            }
            catch(...)
            {
            }
        }

        // Version 12 - auto-import meshes
        if(application.m_ProjectSettings.ProjectVersion > 11)
        {
            try
            {
                archive(cereal::make_nvp("AutoImportMeshes", application.m_ProjectSettings.AutoImportMeshes));
            }
            catch(...)
            {
            }
        }

        // Switch to start scene: prefer name (v11+), fall back to index
        if(!application.m_ProjectSettings.StartScene.empty())
            application.m_SceneManager->SwitchScene(application.m_ProjectSettings.StartScene.c_str());
        else
            application.m_SceneManager->SwitchScene(sceneIndex);
    }
}
