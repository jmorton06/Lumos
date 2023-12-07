#pragma once
#include "MeshFactory.h"
#include "Mesh.h"
#include "Material.h"
#include "Core/OS/FileSystem.h"
#include "Core/Asset.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    namespace Graphics
    {
        class Model : public Asset
        {
        public:
            Model() = default;
            Model(const std::string& filePath);
            Model(const SharedPtr<Mesh>& mesh, PrimitiveType type);
            Model(PrimitiveType type);

            ~Model();

            std::vector<SharedPtr<Mesh>>& GetMeshesRef() { return m_Meshes; }
            const std::vector<SharedPtr<Mesh>>& GetMeshes() const { return m_Meshes; }
            void AddMesh(SharedPtr<Mesh> mesh) { m_Meshes.push_back(mesh); }

            template <typename Archive>
            void save(Archive& archive) const
            {
                if(m_Meshes.size() > 0)
                {
                    std::string newPath;
                    FileSystem::Get().AbsolutePathToFileSystem(m_FilePath, newPath);

                    auto material = std::unique_ptr<Material>(m_Meshes.front()->GetMaterial().get());
                    archive(cereal::make_nvp("PrimitiveType", m_PrimitiveType), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                    material.release();
                }
            }

            template <typename Archive>
            void load(Archive& archive)
            {
                auto material = std::unique_ptr<Graphics::Material>();

                archive(cereal::make_nvp("PrimitiveType", m_PrimitiveType), cereal::make_nvp("FilePath", m_FilePath), cereal::make_nvp("Material", material));

                m_Meshes.clear();

                if(m_PrimitiveType != PrimitiveType::File)
                {
                    m_Meshes.push_back(SharedPtr<Mesh>(CreatePrimative(m_PrimitiveType)));
                    m_Meshes.back()->SetMaterial(SharedPtr<Material>(material.get()));
                    material.release();
                }
                else
                {
                    LoadModel(m_FilePath);
                    // TODO: This should load material changes from editor
                    // m_Meshes.back()->SetMaterial(SharedPtr<Material>(material.get()));
                    // material.release();
                }
            }

            const std::string& GetFilePath() const { return m_FilePath; }
            PrimitiveType GetPrimitiveType() { return m_PrimitiveType; }
            void SetPrimitiveType(PrimitiveType type) { m_PrimitiveType = type; }
            SET_ASSET_TYPE(AssetType::Model);

        private:
            PrimitiveType m_PrimitiveType = PrimitiveType::None;
            std::vector<SharedPtr<Mesh>> m_Meshes;
            std::string m_FilePath;

            void LoadOBJ(const std::string& path);
            void LoadGLTF(const std::string& path);
            void LoadFBX(const std::string& path);

        public:
            void LoadModel(const std::string& path);
        };
    }
}
