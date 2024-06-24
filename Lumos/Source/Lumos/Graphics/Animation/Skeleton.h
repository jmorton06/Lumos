#pragma once

#include "Core/Asset/Asset.h"
#include <ozz/base/memory/unique_ptr.h>

namespace ozz
{
    namespace animation
    {
        class Skeleton;
    }
}

namespace Lumos
{
    namespace Graphics
    {
        class Skeleton : public Asset
        {
        public:
            Skeleton(const std::string& filename);
            Skeleton(ozz::animation::Skeleton* skeleton);

            virtual ~Skeleton();

            const std::string& GetFilePath() const { return m_FilePath; }

            static AssetType GetStaticType() { return AssetType::Skeleton; }
            virtual AssetType GetAssetType() const override { return GetStaticType(); }

            bool Valid() { return m_Skeleton != nullptr; }
            const ozz::animation::Skeleton& GetSkeleton() const
            {
                LUMOS_ASSERT(m_Skeleton, "Attempted to access null skeleton!");
                return *m_Skeleton;
            }

        private:
            std::string m_FilePath;
            ozz::unique_ptr<ozz::animation::Skeleton> m_Skeleton;
        };
    }
}
