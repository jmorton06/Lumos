#pragma once

#include "Core/Asset/Asset.h"
#include <ozz/base/memory/unique_ptr.h>

namespace ozz
{
    namespace animation
    {
        class Animation;
    }
}

namespace Lumos
{
    namespace Graphics
    {
        class Skeleton;
        class LUMOS_EXPORT Animation : public Asset
        {
        public:
            Animation(const std::string& filename, const std::string& animationName, SharedPtr<Skeleton> skeleton);
            Animation(const std::string& animationName, ozz::animation::Animation* animation, SharedPtr<Skeleton> skeleton);

            virtual ~Animation();

            const std::string& GetFilePath() const { return m_FilePath; }
            const std::string& GetName() const { return m_AnimationName; }
            const SharedPtr<Skeleton>& GetSkeleton() const { return m_Skeleton; }

            static AssetType GetStaticType() { return AssetType::Animation; }
            virtual AssetType GetAssetType() const override { return GetStaticType(); }

            const ozz::animation::Animation& GetAnimation() const
            {
                ASSERT(m_Animation, "Attempted to access null animation!");
                return *m_Animation;
            }

        private:
            SharedPtr<Skeleton> m_Skeleton;
            std::string m_FilePath;
            std::string m_AnimationName;
            ozz::unique_ptr<ozz::animation::Animation> m_Animation;
        };
    }

}
