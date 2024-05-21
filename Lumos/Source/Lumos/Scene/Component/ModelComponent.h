#pragma once
#include "Graphics/Model.h"

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
            : ModelRef(CreateSharedPtr<Model>(primitive))
        {
        }

        ModelComponent()
        {
        }

        void LoadFromLibrary(const std::string& path);
        void LoadPrimitive(PrimitiveType primitive)
        {
            ModelRef = CreateSharedPtr<Model>(primitive);
        }

        SharedPtr<Model> ModelRef;
    };
}
