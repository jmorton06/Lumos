#include "Precompiled.h"
#include "ModelComponent.h"
#include "Core/Application.h"
#include "Utilities/AssetManager.h"

namespace Lumos::Graphics
{
    ModelComponent::ModelComponent(const std::string& path)
    {
        LoadFromLibrary(path);
    }

    void ModelComponent::LoadFromLibrary(const std::string& path)
    {
        ModelRef = Application::Get().GetAssetManager()->AddAsset(path, CreateSharedPtr<Graphics::Model>(path)).data.As<Graphics::Model>();

        // ModelRef = Application::Get().GetModelLibrary()->GetAsset(path);
    }

}
