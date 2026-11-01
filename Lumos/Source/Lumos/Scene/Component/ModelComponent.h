#pragma once
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Animation/AnimationController.h"
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

        // MaterialOverrides -> InstanceMaterials: this entity only, the shared model is not touched
        void ApplyMaterialOverrides();
        // mesh `mesh` of this entity wears the .lmat at path (kept in the scene)
        void SetMaterialOverride(u32 mesh, const std::string& path);
        // writes a material as an .lmat (VFS path)
        static bool WriteMaterialFile(const Material& material, const std::string& path);
        void ApplyAnimationOverride();

        // Reload all scene models whose source path matches. Returns true if any were found.
        static bool ReloadSceneModels(const std::string& sourcePath);

        SharedPtr<Model> ModelRef;
        std::unordered_map<u32, std::string> MaterialOverrides;
        std::unordered_map<u32, SharedPtr<Material>> InstanceMaterials;
        std::string AnimationOverride;
        // false = not drawn, but still casts a shadow and keeps any physics body.
        // Runtime only (a first person character hiding its own body); not saved.
        bool Visible = true;

        // This entity's own animation (clock, blend weights, pose): entities sharing a model each move
        // on their own. Made on first use; follows the model's current clip until a script drives it.
        SharedPtr<AnimationController> Animator;
        const Model* AnimatorSource = nullptr;
        u32 AnimatorClipCount       = 0;
        bool AnimatorScripted       = false;
        AnimationController* GetAnimator();
        void TickAnimation(float dt);
    };
}
