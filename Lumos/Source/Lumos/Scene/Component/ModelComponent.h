#pragma once
#include "Graphics/Model.h"
#include <unordered_map>

namespace Lumos::Graphics
{
    struct ModelComponent
    {
        template <typename Archive>
        friend void save(Archive& archive, const ModelComponent& model);

        template <typename Archive>
        friend void load(Archive& archive, ModelComponent& model);

        ModelComponent(const SharedPtr<Model>& modelRef)
            : ModelRef(modelRef)
        {
        }

        ModelComponent(const std::string& path);

        ModelComponent(PrimitiveType primitive)
        {
            LoadPrimitive(primitive);
        }

        ModelComponent()
        {
        }

        void LoadFromLibrary(const std::string& path, bool previewOnly = false);
        void LoadPrimitive(PrimitiveType primitive);

        void ApplyMaterialOverrides();
        void ApplyAnimationOverride();

        // Reload all scene models whose source path matches. Returns true if any were found.
        static bool ReloadSceneModels(const std::string& sourcePath);

        SharedPtr<Model> ModelRef;
        std::unordered_map<u32, std::string> MaterialOverrides;
        std::string AnimationOverride;
    };
}
