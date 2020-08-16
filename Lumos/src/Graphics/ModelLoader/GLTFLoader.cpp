#include "lmpch.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"

#include "Graphics/API/Texture.h"
#include "Maths/Maths.h"

#include "Maths/Transform.h"
#include "Core/Application.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef LUMOS_DIST
#	define TINYGLTF_NOEXCEPTION
#endif
#include <tinygltf/tiny_gltf.h>

namespace Lumos::Graphics
{
	std::string AlbedoTexName = "baseColorTexture";
	std::string NormalTexName = "normalTexture";
	std::string MetallicTexName = "metallicRoughnessTexture";
	std::string GlossTexName = "metallicRoughnessTexture";
	std::string AOTexName = "occlusionTexture";
	std::string EmissiveTexName = "emissiveTexture";

	struct GLTFTexture
	{
		tinygltf::Image* Image;
		tinygltf::Sampler* Sampler;
	};

	static std::map<int32_t, size_t> ComponentSize{
		{TINYGLTF_COMPONENT_TYPE_BYTE, sizeof(int8_t)},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, sizeof(uint8_t)},
		{TINYGLTF_COMPONENT_TYPE_SHORT, sizeof(int16_t)},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, sizeof(uint16_t)},
		{TINYGLTF_COMPONENT_TYPE_INT, sizeof(int32_t)},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, sizeof(uint32_t)},
		{TINYGLTF_COMPONENT_TYPE_FLOAT, sizeof(float)},
		{TINYGLTF_COMPONENT_TYPE_DOUBLE, sizeof(double)}};

	static std::map<int, int> GLTF_COMPONENT_LENGTH_LOOKUP = {
		{TINYGLTF_TYPE_SCALAR, 1},
		{TINYGLTF_TYPE_VEC2, 2},
		{TINYGLTF_TYPE_VEC3, 3},
		{TINYGLTF_TYPE_VEC4, 4},
		{TINYGLTF_TYPE_MAT2, 4},
		{TINYGLTF_TYPE_MAT3, 9},
		{TINYGLTF_TYPE_MAT4, 16}};

	static std::map<int, int> GLTF_COMPONENT_BYTE_SIZE_LOOKUP = {
		{TINYGLTF_COMPONENT_TYPE_BYTE, 1},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1},
		{TINYGLTF_COMPONENT_TYPE_SHORT, 2},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2},
		{TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, 4},
		{TINYGLTF_COMPONENT_TYPE_FLOAT, 4}};

	static Graphics::TextureWrap GetWrapMode(int mode)
	{
		switch(mode)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			return Graphics::TextureWrap::REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return Graphics::TextureWrap::CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return Graphics::TextureWrap::MIRRORED_REPEAT;
		default:
			return Graphics::TextureWrap::REPEAT;
		}
	}

	static Graphics::TextureFilter GetFilter(int value)
	{
		switch(value)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return Graphics::TextureFilter::NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return Graphics::TextureFilter::LINEAR;
		default:
			return Graphics::TextureFilter::LINEAR;
		}
	}

	std::vector<Ref<Material>> LoadMaterials(tinygltf::Model& gltfModel)
	{
		std::vector<Ref<Graphics::Texture2D>> loadedTextures;
		std::vector<Ref<Material>> loadedMaterials;
		loadedTextures.reserve(gltfModel.textures.size());
		loadedMaterials.reserve(gltfModel.materials.size());

		for(tinygltf::Texture& gltfTexture : gltfModel.textures)
		{
			GLTFTexture imageAndSampler{};

			if(gltfTexture.source != -1)
			{
				imageAndSampler.Image = &gltfModel.images.at(gltfTexture.source);
			}

			if(gltfTexture.sampler != -1)
			{
				imageAndSampler.Sampler = &gltfModel.samplers.at(gltfTexture.sampler);
			}

			if(imageAndSampler.Image)
			{
				Graphics::TextureParameters params;
				if(gltfTexture.sampler != -1)
					params = Graphics::TextureParameters(GetFilter(imageAndSampler.Sampler->minFilter), GetFilter(imageAndSampler.Sampler->magFilter), GetWrapMode(imageAndSampler.Sampler->wrapS));

				Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromSource(imageAndSampler.Image->width, imageAndSampler.Image->height, imageAndSampler.Image->image.data(), params);
				if(texture2D)
					loadedTextures.push_back(Ref<Graphics::Texture2D>(texture2D));
			}
		}

		for(tinygltf::Material& mat : gltfModel.materials)
		{
			Ref<Material> pbrMaterial = CreateRef<Material>();
			PBRMataterialTextures textures;
			Graphics::MaterialProperties properties;

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

			if(baseColorTexture != mat.values.end())
			{
				textures.albedo = loadedTextures[gltfModel.textures[baseColorTexture->second.TextureIndex()].source];
			}

			if(normalTexture != mat.additionalValues.end())
			{
				textures.normal = loadedTextures[gltfModel.textures[normalTexture->second.TextureIndex()].source];
			}

			if(emissiveTexture != mat.additionalValues.end())
			{
				textures.emissive = loadedTextures[gltfModel.textures[emissiveTexture->second.TextureIndex()].source];
			}

			if(metallicRoughnessTexture != mat.values.end())
			{
				textures.metallic = loadedTextures[gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()].source];
				properties.workflow = PBR_WORKFLOW_METALLIC_ROUGHNESS;
			}

			if(occlusionTexture != mat.additionalValues.end())
			{
				textures.ao = loadedTextures[gltfModel.textures[occlusionTexture->second.TextureIndex()].source];
			}

			if(roughnessFactor != mat.values.end())
			{
				properties.roughnessColour = Maths::Vector4(static_cast<float>(roughnessFactor->second.Factor()));
			}

			if(metallicFactor != mat.values.end())
			{
				properties.metallicColour = Maths::Vector4(static_cast<float>(metallicFactor->second.Factor()));
			}

			if(baseColorFactor != mat.values.end())
			{
				properties.albedoColour = Maths::Vector4((float)baseColorFactor->second.ColorFactor()[0], (float)baseColorFactor->second.ColorFactor()[1], (float)baseColorFactor->second.ColorFactor()[2], 1.0f);
			}

			// Extensions
			auto metallicGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrMetallicGlossiness");
			if(metallicGlossinessWorkflow != mat.extensions.end())
			{
				if(metallicGlossinessWorkflow->second.Has("diffuseTexture"))
				{
					int index = metallicGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
					textures.albedo = loadedTextures[gltfModel.textures[index].source];
				}

				if(metallicGlossinessWorkflow->second.Has("metallicGlossinessTexture"))
				{
					int index = metallicGlossinessWorkflow->second.Get("metallicGlossinessTexture").Get("index").Get<int>();
					textures.roughness = loadedTextures[gltfModel.textures[index].source];
				}

				if(metallicGlossinessWorkflow->second.Has("diffuseFactor"))
				{
					auto& factor = metallicGlossinessWorkflow->second.Get("diffuseFactor");
					properties.albedoColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
					properties.albedoColour.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
					properties.albedoColour.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
					properties.albedoColour.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
				}
				if(metallicGlossinessWorkflow->second.Has("metallicFactor"))
				{
					auto& factor = metallicGlossinessWorkflow->second.Get("metallicFactor");
					properties.metallicColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
					properties.metallicColour.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
					properties.metallicColour.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
					properties.metallicColour.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
				}
				if(metallicGlossinessWorkflow->second.Has("glossinessFactor"))
				{
					auto& factor = metallicGlossinessWorkflow->second.Get("glossinessFactor");
					properties.roughnessColour = Maths::Vector4(1.0f - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>()));
				}
			}

			pbrMaterial->SetTextures(textures);
			pbrMaterial->SetMaterialProperites(properties);
			loadedMaterials.push_back(pbrMaterial);
		}

		return loadedMaterials;
	}

	std::vector<Graphics::Mesh*> LoadMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, std::vector<Ref<Material>>& materials, Maths::Transform& parentTransform)
	{
		std::vector<Graphics::Mesh*> meshes;

		for(auto& primitive : mesh.primitives)
		{
			const tinygltf::Accessor& indices = model.accessors[primitive.indices];

			const u32 numVertices = static_cast<u32>(indices.count);
			Graphics::Vertex* tempvertices = lmnew Graphics::Vertex[numVertices];
			u32* indicesArray = lmnew u32[numVertices];

			size_t maxNumVerts = 0;

			Ref<Maths::BoundingBox> boundingBox = CreateRef<Maths::BoundingBox>();

			for(auto& attribute : primitive.attributes)
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

				if(attribute.first == "POSITION")
				{
					size_t positionCount = accessor.count;
					maxNumVerts = Lumos::Maths::Max(maxNumVerts, positionCount);
					Maths::Vector3Simple* positions = reinterpret_cast<Maths::Vector3Simple*>(data.data());
					for(auto p = 0; p < positionCount; ++p)
					{
						tempvertices[p].Position = parentTransform.GetWorldMatrix() * Maths::ToVector(positions[p]);
						boundingBox->Merge(tempvertices[p].Position);
					}
				}

				// -------- Normal attribute -----------

				else if(attribute.first == "NORMAL")
				{
					size_t normalCount = accessor.count;
					maxNumVerts = Lumos::Maths::Max(maxNumVerts, normalCount);
					Maths::Vector3Simple* normals = reinterpret_cast<Maths::Vector3Simple*>(data.data());
					for(auto p = 0; p < normalCount; ++p)
					{
						tempvertices[p].Normal = parentTransform.GetWorldMatrix() * Maths::ToVector(normals[p]);
					}
				}

				// -------- Texcoord attribute -----------

				else if(attribute.first == "TEXCOORD_0")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = Lumos::Maths::Max(maxNumVerts, uvCount);
					Maths::Vector2Simple* uvs = reinterpret_cast<Maths::Vector2Simple*>(data.data());
					for(auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].TexCoords = ToVector(uvs[p]);
					}
				}

				// -------- Colour attribute -----------

				else if(attribute.first == "COLOR_0")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = Lumos::Maths::Max(maxNumVerts, uvCount);
					Maths::Vector4Simple* colours = reinterpret_cast<Maths::Vector4Simple*>(data.data());
					for(auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].Colours = ToVector(colours[p]);
					}
				}

				// -------- Tangent attribute -----------

				else if(attribute.first == "TANGENT")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = Lumos::Maths::Max(maxNumVerts, uvCount);
					Maths::Vector3Simple* uvs = reinterpret_cast<Maths::Vector3Simple*>(data.data());
					for(auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].Tangent = parentTransform.GetWorldMatrix() * ToVector(uvs[p]);
					}
				}
			}

			Ref<Graphics::VertexArray> va;
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
				if(componentTypeByteSize == 2)
				{
					uint16_t* in = reinterpret_cast<uint16_t*>(data.data()); //TODO: Test different models to check size - u32 or 16
					for(auto iCount = 0; iCount < indicesCount; iCount++)
					{
						indicesArray[iCount] = in[iCount];
					}
				}
				else if(componentTypeByteSize == 4)
				{
					auto in = reinterpret_cast<uint32_t*>(data.data());
					for(auto iCount = 0; iCount < indicesCount; iCount++)
					{
						indicesArray[iCount] = in[iCount];
					}
				}
			}

			Ref<Graphics::IndexBuffer> ib;
			ib.reset(Graphics::IndexBuffer::Create(indicesArray, numVertices));

			auto lMesh = lmnew Graphics::Mesh(va, ib, boundingBox);

			delete[] tempvertices;
			delete[] indicesArray;

			meshes.emplace_back(lMesh);
		}

		return meshes;
	}

	void LoadNode(Model* mainModel, int nodeIndex, const Maths::Matrix4& parentTransform, tinygltf::Model& model, std::vector<Ref<Material>>& materials, std::vector<std::vector<Graphics::Mesh*>>& meshes)
	{
		if(nodeIndex < 0)
		{
			return;
		}

		auto& node = model.nodes[nodeIndex];
		auto name = node.name;

		Maths::Transform transform;

		if(!node.scale.empty())
		{
			transform.SetLocalScale(Maths::Vector3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2])));
		}

		if(!node.rotation.empty())
		{
			transform.SetLocalOrientation(Maths::Quaternion(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2])));
		}

		if(!node.translation.empty())
		{
			transform.SetLocalPosition(Maths::Vector3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2])));
		}

		if(!node.matrix.empty())
		{
			auto lTransform = Maths::Matrix4(reinterpret_cast<float*>(node.matrix.data()));
			transform.SetLocalTransform(lTransform.Transpose());
		}

		transform.UpdateMatrices();
		transform.SetWorldMatrix(parentTransform);

		if(node.mesh >= 0)
		{
			int subIndex = 0;

			auto meshes = LoadMesh(model, model.meshes[node.mesh], materials, transform);

			for(auto& mesh : meshes)
			{
				auto subname = node.name;
				auto lMesh = Ref<Graphics::Mesh>(mesh);
				lMesh->SetName(subname);

				int materialIndex = model.meshes[node.mesh].primitives[subIndex].material;
				if(materialIndex >= 0)
					lMesh->SetMaterial(materials[materialIndex]);

				mainModel->AddMesh(lMesh);

				/*if (node.skin >= 0)
                {
                }*/

				subIndex++;
			}
		}

		if(!node.children.empty())
		{
			for(int child : node.children)
			{
				LoadNode(mainModel, child, transform.GetLocalMatrix(), model, materials, meshes);
			}
		}
	}

	void Model::LoadGLTF(const std::string& path)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string ext = StringFormat::GetFilePathExtension(path);

		loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
		loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

		bool ret;

		if(ext == "glb") // assume binary glTF.
		{
			ret = tinygltf::TinyGLTF().LoadBinaryFromFile(&model, &err, &warn, path);
		}
		else // assume ascii glTF.
		{
			ret = tinygltf::TinyGLTF().LoadASCIIFromFile(&model, &err, &warn, path);
		}

		if(!err.empty())
		{
			Debug::Log::Error(err);
		}

		if(!warn.empty())
		{
			Debug::Log::Error(warn);
		}

		if(!ret)
		{
			Debug::Log::Error("Failed to parse glTF");
		}

		auto LoadedMaterials = LoadMaterials(model);

		std::string name = path.substr(path.find_last_of('/') + 1);

		auto meshes = std::vector<std::vector<Graphics::Mesh*>>();
		const tinygltf::Scene& gltfScene = model.scenes[Lumos::Maths::Max(0, model.defaultScene)];
		for(size_t i = 0; i < gltfScene.nodes.size(); i++)
		{
			LoadNode(this, gltfScene.nodes[i], Maths::Matrix4(), model, LoadedMaterials, meshes);
		}
	}
}
