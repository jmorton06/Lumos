#include "Precompiled.h"
#include "ModelComponent.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"

namespace Lumos::Graphics
{
    ModelComponent::ModelComponent(const std::string& path)
    {
        LoadFromLibrary(path);
    }

    void ModelComponent::LoadFromLibrary(const std::string& path)
    {
        String8 Path = Str8StdS(path);
        ModelRef     = Application::Get().GetAssetManager()->AddAsset(Path, CreateSharedPtr<Graphics::Model>(path)).data.As<Graphics::Model>();

        // ModelRef = Application::Get().GetModelLibrary()->GetAsset(path);
    }

}
