#include "LM.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/BoundingSphere.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/MeshComponent.h"
#include "ECS/Component/MaterialComponent.h"
#include "ECS/Component/TransformComponent.h"

#include "Graphics/API/Texture.h"
#include "Utilities/AssetsManager.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Matrix4.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef LUMOS_DIST
#define TINYGLTF_NOEXCEPTION
#endif
#include <tinygltf/tiny_gltf.h>

namespace Lumos
{
	String AlbedoTexName = "baseColorTexture";
	String NormalTexName = "normalTexture";
	String MetallicTexName = "metallicRoughnessTexture";
	String GlossTexName = "metallicRoughnessTexture";
	String AOTexName = "occlusionTexture";
	String EmissiveTexName = "emissiveTexture";

	struct GLTFTexture
	{
		tinygltf::Image* Image;
		tinygltf::Sampler* Sampler;
	};

	static std::map<int32_t, size_t> ComponentSize
	{
	{ TINYGLTF_COMPONENT_TYPE_BYTE,			  sizeof(int8_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,  sizeof(uint8_t) },
	{ TINYGLTF_COMPONENT_TYPE_SHORT,		  sizeof(int16_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, sizeof(uint16_t) },
	{ TINYGLTF_COMPONENT_TYPE_INT,			  sizeof(int32_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,   sizeof(uint32_t) },
	{ TINYGLTF_COMPONENT_TYPE_FLOAT,		  sizeof(float) },
	{ TINYGLTF_COMPONENT_TYPE_DOUBLE,		  sizeof(double) }
	};


	static std::map<int, int> GLTF_COMPONENT_LENGTH_LOOKUP = {
		{ TINYGLTF_TYPE_SCALAR, 1 },
	{ TINYGLTF_TYPE_VEC2, 2 },
	{ TINYGLTF_TYPE_VEC3, 3 },
	{ TINYGLTF_TYPE_VEC4, 4 },
	{ TINYGLTF_TYPE_MAT2, 4 },
	{ TINYGLTF_TYPE_MAT3, 9 },
	{ TINYGLTF_TYPE_MAT4, 16 }
	};

	static std::map<int, int> GLTF_COMPONENT_BYTE_SIZE_LOOKUP = {
		{ TINYGLTF_COMPONENT_TYPE_BYTE , 1 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1 },
	{ TINYGLTF_COMPONENT_TYPE_SHORT, 2 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, 4 },
	{ TINYGLTF_COMPONENT_TYPE_FLOAT, 4 }
	};

	static Graphics::TextureWrap GetWrapMode(int mode)
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT: return Graphics::TextureWrap::REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return Graphics::TextureWrap::CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return Graphics::TextureWrap::MIRRORED_REPEAT;
		default: return Graphics::TextureWrap::REPEAT;
		}
	}

	static Graphics::TextureFilter GetFilter(int value)
	{
		switch (value)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return Graphics::TextureFilter::NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return Graphics::TextureFilter::LINEAR;
		default: return Graphics::TextureFilter::LINEAR;
		}
	}

	std::vector<std::shared_ptr<Material>> LoadMaterials(tinygltf::Model &gltfModel)
    {
        std::vector<std::shared_ptr<Graphics::Texture2D>> loadedTextures;
        std::vector<std::shared_ptr<Material>> loadedMaterials;
        for (tinygltf::Texture &gltfTexture : gltfModel.textures)
        {
            GLTFTexture imageAndSampler{};

            if (gltfTexture.source != -1)
            {
                imageAndSampler.Image = &gltfModel.images.at(gltfTexture.source);
            }

            if (gltfTexture.sampler != -1)
            {
                imageAndSampler.Sampler = &gltfModel.samplers.at(gltfTexture.sampler);
            }

            if (imageAndSampler.Image)
            {
                Graphics::TextureParameters params;
                if(gltfTexture.sampler != -1)
                    params = Graphics::TextureParameters(GetFilter(imageAndSampler.Sampler->minFilter), GetWrapMode(imageAndSampler.Sampler->wrapS));

				Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromSource(imageAndSampler.Image->width, imageAndSampler.Image->height, imageAndSampler.Image->image.data(), params);
                if (texture2D)
                    loadedTextures.push_back(std::shared_ptr<Graphics::Texture2D>(texture2D));
            }
        }

        for (tinygltf::Material &mat : gltfModel.materials)
        {
            std::shared_ptr<Material> pbrMaterial = std::make_shared<Material>();
            PBRMataterialTextures textures;
            MaterialProperties properties;
            
            // metallic-roughness workflow:
            auto baseColorTexture = mat.values.find("baseColorTexture");
            auto metallicRoughnessTexture = mat.values.find("metallicRoughnessTexture");
            auto baseColorFactor = mat.values.find("baseColorFactor");
            auto roughnessFactor = mat.values.find("roughnessFactor");
            auto metallicFactor = mat.values.find("metallicFactor");
            
            // common workflow:
            auto normalTexture = mat.additionalValues.find("normalTexture");
            auto emissiveTexture = mat.additionalValues.find("emissiveTexture");
            auto occlusionTexture = mat.additionalValues.find("occlusionTexture");
            //auto emissiveFactor = mat.additionalValues.find("emissiveFactor");
            //auto alphaCutoff = mat.additionalValues.find("alphaCutoff");
            //auto alphaMode = mat.additionalValues.find("alphaMode");

            if (baseColorTexture != mat.values.end())
            {
                textures.albedo = loadedTextures[gltfModel.textures[baseColorTexture->second.TextureIndex()].source];
            }
            
            if (normalTexture != mat.additionalValues.end())
            {
                textures.normal = loadedTextures[gltfModel.textures[normalTexture->second.TextureIndex()].source];
            }

			if (emissiveTexture != mat.additionalValues.end())
			{
				textures.emissive = loadedTextures[gltfModel.textures[emissiveTexture->second.TextureIndex()].source];
			}
            
            if (metallicRoughnessTexture != mat.values.end())
            {
                textures.specular = loadedTextures[gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()].source];
				properties.workflow = PBR_WORKFLOW_METALLIC_ROUGHNESS;
            }

			if (occlusionTexture != mat.additionalValues.end())
			{
				textures.ao = loadedTextures[gltfModel.textures[occlusionTexture->second.TextureIndex()].source];
			}
            
            if (roughnessFactor != mat.values.end())
            {
                properties.roughnessColour = static_cast<float>(roughnessFactor->second.Factor());
            }
            
            if (metallicFactor != mat.values.end())
            {
                properties.specularColour = Maths::Vector4(static_cast<float>(metallicFactor->second.Factor()));
            }
            
            if (baseColorFactor != mat.values.end())
            {
                properties.albedoColour = Maths::Vector4((float)baseColorFactor->second.ColorFactor()[0],(float)baseColorFactor->second.ColorFactor()[1],(float)baseColorFactor->second.ColorFactor()[2],1.0f);
            }
            
            // Extensions
            auto specularGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (specularGlossinessWorkflow != mat.extensions.end())
            {
                if (specularGlossinessWorkflow->second.Has("diffuseTexture"))
                {
                    int index = specularGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
                    textures.albedo = loadedTextures[gltfModel.textures[index].source];

                }
                
                if (specularGlossinessWorkflow->second.Has("specularGlossinessTexture"))
                {
                    int index = specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("index").Get<int>();
                    textures.roughness = loadedTextures[gltfModel.textures[index].source];
                }
                
                if (specularGlossinessWorkflow->second.Has("diffuseFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("diffuseFactor");
                    properties.albedoColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                    properties.albedoColour.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
                    properties.albedoColour.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
                    properties.albedoColour.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
                }
                if (specularGlossinessWorkflow->second.Has("specularFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("specularFactor");
                    properties.specularColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                    properties.specularColour.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
                    properties.specularColour.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
                    properties.specularColour.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
                }
                if (specularGlossinessWorkflow->second.Has("glossinessFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("glossinessFactor");
                    properties.roughnessColour = Maths::Vector4(1.0f - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>()));
                }
            }

            pbrMaterial->SetTextures(textures);
            pbrMaterial->SetMaterialProperites(properties);
            loadedMaterials.push_back(pbrMaterial);
        }

        return loadedMaterials;
    }
    
	Graphics::Mesh* LoadMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, std::vector<std::shared_ptr<Material>>& materials)
    {
        for (auto& primitive : mesh.primitives)
        {
            const tinygltf::Accessor &indices = model.accessors[primitive.indices];
            
            const u32 numVertices = static_cast<u32>(indices.count);
			Graphics::Vertex* tempvertices = lmnew Graphics::Vertex[numVertices];
            u32* indicesArray = lmnew u32[numVertices];
            
            size_t maxNumVerts = 0;
            
            std::shared_ptr<Maths::BoundingSphere> boundingBox = std::make_shared<Maths::BoundingSphere>();
           
            for (auto& attribute : primitive.attributes)
            {
                // Get accessor info
                auto& accessor = model.accessors.at(attribute.second);
                auto& bufferView = model.bufferViews.at(accessor.bufferView);
                auto& buffer = model.buffers.at(bufferView.buffer);
                int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(accessor.type);
                int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(accessor.componentType);
                
                // Extra vertex data from buffer
                size_t bufferOffset = bufferView.byteOffset + accessor.byteOffset;
                int bufferLength = static_cast<int>(accessor.count) * componentLength * componentTypeByteSize;
                auto first = buffer.data.begin() + bufferOffset;
                auto last = buffer.data.begin() + bufferOffset + bufferLength;
                std::vector<u8> data = std::vector<u8>(first, last);
                
                // -------- Position attribute -----------
                
                if (attribute.first == "POSITION")
                {
                    size_t positionCount = accessor.count;
                    maxNumVerts = Maths::Max(maxNumVerts, positionCount);
                    Maths::Vector3Simple* positions = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < positionCount; ++p)
                    {
                        //positions[p] = glm::vec3(matrix * glm::vec4(positions[p], 1.0f));
                        tempvertices[p].Position = Maths::ToVector(positions[p]);
                        
                        boundingBox->ExpandToFit(tempvertices[p].Position);
                    }
                }
                
                // -------- Normal attribute -----------
                
                else if (attribute.first == "NORMAL")
                {
                    size_t normalCount = accessor.count;
                    maxNumVerts = Maths::Max(maxNumVerts, normalCount);
                    Maths::Vector3Simple* normals = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < normalCount; ++p)
                    {
                        tempvertices[p].Normal = Maths::ToVector(normals[p]);
                    }
                }
                
                // -------- Texcoord attribute -----------
                
                else if (attribute.first == "TEXCOORD_0")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = Maths::Max(maxNumVerts, uvCount);
                    Maths::Vector2Simple* uvs = reinterpret_cast<Maths::Vector2Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].TexCoords = ToVector(uvs[p]);
                    }
                }
                
                // -------- Colour attribute -----------
                
                else if (attribute.first == "COLOR_0")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = Maths::Max(maxNumVerts, uvCount);
                    Maths::Vector4Simple* colours = reinterpret_cast<Maths::Vector4Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].Colours = ToVector(colours[p]);
                    }
                }
                
                // -------- Tangent attribute -----------
                
                else if (attribute.first == "TANGENT")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = Maths::Max(maxNumVerts, uvCount);
                    Maths::Vector3Simple* uvs = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].Tangent = ToVector(uvs[p]);
                    }
                }
            }

            std::shared_ptr<Graphics::VertexArray> va;
            va.reset(Graphics::VertexArray::Create());
            
			Graphics::VertexBuffer* buffer = Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC);
            buffer->SetData(sizeof(Graphics::Vertex) * numVertices, tempvertices);
            
            Graphics::BufferLayout layout;
            layout.Push<Maths::Vector3>("position");
            layout.Push<Maths::Vector4>("colour");
            layout.Push<Maths::Vector2>("texCoord");
            layout.Push<Maths::Vector3>("normal");
            layout.Push<Maths::Vector3>("tangent");
            buffer->SetLayout(layout);
            
            va->PushBuffer(buffer);
            
            // -------- Indices ----------
            {
                // Get accessor info
                auto indexAccessor = model.accessors.at(primitive.indices);
                auto indexBufferView = model.bufferViews.at(indexAccessor.bufferView);
                auto indexBuffer = model.buffers.at(indexBufferView.buffer);
                
                int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
                int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);
                
                // Extra index data
                size_t bufferOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
                int bufferLength = static_cast<int>(indexAccessor.count) * componentLength * componentTypeByteSize;
                auto first = indexBuffer.data.begin() + bufferOffset;
                auto last = indexBuffer.data.begin() + bufferOffset + bufferLength;
                std::vector<u8> data = std::vector<u8>(first, last);
                
                size_t indicesCount = indexAccessor.count;
                if (componentTypeByteSize == 2)
                {
                    uint16_t* in = reinterpret_cast<uint16_t*>(data.data()); //TODO: Test different models to check size - u32 or 16
                    for (auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indicesArray[iCount] = in[iCount];
                    }
                }
                else if (componentTypeByteSize == 4)
                {
                    uint32_t* in = reinterpret_cast<uint32_t*>(data.data());
                    for (auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indicesArray[iCount] = in[iCount];
                    }
                }
            }
            
            std::shared_ptr<Graphics::IndexBuffer> ib;
            ib.reset(Graphics::IndexBuffer::Create(indicesArray, numVertices));

            auto lMesh = lmnew Graphics::Mesh(va, ib, boundingBox);
            
            delete[] tempvertices;
            delete[] indicesArray;
            
            return lMesh;
        }
        
        return nullptr;
    }
    
    void LoadNode(int nodeIndex, Entity* parent, tinygltf::Model& model, std::vector<std::shared_ptr<Material>>& materials, std::vector<Graphics::Mesh*>& meshes)
    {
        if (nodeIndex < 0)
        {
            return;
        }
        
        auto& node = model.nodes[nodeIndex];
        
        auto name = node.name;
        if(name == "")
            name = "Mesh : " + StringFormat::ToString(nodeIndex);
        auto meshEntity = EntityManager::Instance()->CreateEntity(name);
        meshEntity->AddComponent<TransformComponent>();
        
        if(parent)
            parent->AddChild(meshEntity);
        
        if(node.mesh >= 0)
        {
           
            auto lMesh = std::shared_ptr<Graphics::Mesh>(meshes[node.mesh]);
            meshEntity->AddComponent<MeshComponent>(lMesh);

			int materialIndex = model.meshes[node.mesh].primitives[0].material;
			if(materialIndex >= 0)
				meshEntity->AddComponent<MaterialComponent>(materials[materialIndex]);

			/*if (node.skin >= 0)
			{
            }*/
        }
        
        TransformComponent* transform = meshEntity->GetTransformComponent();
        
        if (!node.scale.empty())
        {
            transform->GetTransform().SetLocalScale(Maths::Vector3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2])));
        }
        if (!node.rotation.empty())
        {
            transform->GetTransform().SetLocalOrientation(Maths::Quaternion(static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]), static_cast<float>(node.rotation[3])));
        }
        if (!node.translation.empty())
        {
            transform->GetTransform().SetLocalPosition(Maths::Vector3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2])));
        }
        if (!node.matrix.empty())
        {
            auto lTransform = Maths::Matrix4(reinterpret_cast<float*>(node.matrix.data()));
            transform->GetTransform().SetLocalTransform(lTransform);
            transform->GetTransform().ApplyTransform(); // this creates S, R, T vectors from local matrix
        }

		transform->GetTransform().UpdateMatrices();
        
        if (!node.children.empty())
        {
            for (int child : node.children)
            {
                LoadNode(child, meshEntity, model, materials, meshes);
            }
        }
    }

    Entity* ModelLoader::LoadGLTF(const String& path)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string ext = StringFormat::GetFilePathExtension(path);

		loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
		loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

		bool ret;

		if (ext == "glb") // assume binary glTF.
		{
			ret = tinygltf::TinyGLTF().LoadBinaryFromFile(&model, &err, &warn, path);
		}
		else // assume ascii glTF.
		{
			ret = tinygltf::TinyGLTF().LoadASCIIFromFile(&model, &err, &warn, path);
		}

		if (!err.empty())
		{
			LUMOS_CORE_ERROR(err);
		}

		if (!warn.empty())
		{
			LUMOS_CORE_ERROR(warn);
		}

		if (!ret)
		{
			LUMOS_CORE_ERROR("Failed to parse glTF");
		}

        auto LoadedMaterials = LoadMaterials(model);
        
        String directory = path.substr(0, path.find_last_of('/'));
        
        String name = directory.substr(directory.find_last_of('/') + 1);

		auto entity = EntityManager::Instance()->CreateEntity(name);
		entity->AddComponent<TransformComponent>();

        auto meshes = std::vector<Graphics::Mesh*>();
        
        for (auto& mesh : model.meshes)
        {
            meshes.emplace_back(LoadMesh(model,mesh,LoadedMaterials));
        }
        
        const tinygltf::Scene &gltfScene = model.scenes[Maths::Max(0, model.defaultScene)];
        for (size_t i = 0; i < gltfScene.nodes.size(); i++)
        {
            LoadNode(gltfScene.nodes[i], entity, model, LoadedMaterials, meshes);
        }
        
		return entity;
	}

}
