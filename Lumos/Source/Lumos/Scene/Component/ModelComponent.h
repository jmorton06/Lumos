#pragma once
#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Core/VFS.h"
#include <cereal/cereal.hpp>

namespace Lumos::Graphics
{
    struct ModelComponent
    {
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
        
		SharedPtr<Model> ModelRef;
		
		template <typename Archive>
            void save(Archive& archive) const
		{
            if(!ModelRef || ModelRef->GetMeshes().size() == 0)
				return;
            {
                std::string newPath;
                
                
                if(ModelRef->GetPrimitiveType() == PrimitiveType::File )
                    VFS::Get().AbsoulePathToVFS(ModelRef->GetFilePath(), newPath);
                else
                    newPath = "Primitive";

                //For now this saved material will be overriden by materials in the model file
                auto material = std::unique_ptr<Material>(ModelRef->GetMeshes().front()->GetMaterial().get());
                archive(cereal::make_nvp("PrimitiveType", ModelRef->GetPrimitiveType()), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                material.release();
            }
		}
		
		template <typename Archive>
            void load(Archive& archive)
		{
			auto material = std::unique_ptr<Graphics::Material>();
			
			std::string filePath;
			PrimitiveType primitiveType;
			
            archive(cereal::make_nvp("PrimitiveType", primitiveType), cereal::make_nvp("FilePath", filePath), cereal::make_nvp("Material", material));
			
			if(primitiveType != PrimitiveType::File)
			{
				ModelRef = CreateSharedPtr<Model>(primitiveType);
				ModelRef->GetMeshes().back()->SetMaterial(SharedPtr<Material>(material.get()));
				material.release();
			}
			else
			{
				LoadFromLibrary(filePath);
			}
		}
	};
}
