#include "JM.h"
#include "Model.h"
#include "Maths/BoundingSphere.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef JM_DIST
#define TINYGLTF_NOEXCEPTION
#endif
#include "external/tinygltf/tiny_gltf.h"

#include "Graphics/API/Textures/Texture2D.h"
#include "Utilities/AssetsManager.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Matrix4.h"

namespace jm
{
	String AlbedoTexName = "baseColorTexture";
	String NormalTexName = "normalTexture";
	String MetallicTexName = "metallicRoughnessTexture";
	String GlossTexName = "metallicRoughnessTexture";

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

	static TextureWrap GetWrapMode(int mode)
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT: return TextureWrap::REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return TextureWrap::CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return TextureWrap::MIRRORED_REPEAT;
		default: return TextureWrap::REPEAT;
		}
	}

	static TextureFilter GetFilter(int value)
	{
		switch (value)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return TextureFilter::NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return TextureFilter::LINEAR;
		default: return TextureFilter::LINEAR;
		}
	}

	maths::Matrix4 GetMatrixFromGLTFNode(const tinygltf::Node & node)
	{

		maths::Matrix4 curMatrix = maths::Matrix4();

		const std::vector<double> &matrix = node.matrix;
		if (matrix.size() > 0)
		{
			// matrix, copy it

			// for (int i = 0; i < 4; i++)
			// {
			// 	for (int j = 0; j < 4; j++)
			// 	{
			// 		curMatrix[i + j] = (float)matrix.at(4 * i + j); // check
			// 	}
			// }

			float values[16];
			for(int i = 0; i < 16; i++)
			{
				values[i] = float(matrix[i]);
			}

			curMatrix = maths::Matrix4(values);
		}
		else
		{
			// no matrix, use rotation, scale, translation
			// if (node.translation.size() > 0)
			// {
			// 	curMatrix[3][0] = node.translation[0];
			// 	curMatrix[3][1] = node.translation[1];
			// 	curMatrix[3][2] = node.translation[2];
			// }

			// if (node.rotation.size() > 0)
			// {
			// 	maths::Matrix4 R;
			// 	maths::Quaternion q;
			// 	q[0] = node.rotation[0];
			// 	q[1] = node.rotation[1];
			// 	q[2] = node.rotation[2];

			// 	R = glm::mat4_cast(q);
			// 	curMatrix = curMatrix * R;
			// }

			// if (node.scale.size() > 0)
			// {
			// 	curMatrix = curMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			// }

			if (node.translation.size() > 0)
			{
				maths::Vector3 translation = maths::Vector3((float)node.translation[0],(float)node.translation[1],(float)node.translation[2]);
				curMatrix = maths::Matrix4::Translation(translation) * curMatrix;
			}

			if (node.rotation.size() > 0)
			{
				//curMatrix = maths::Matrix4::Rotation((float)node.rotation[0],(float)node.rotation[1],(float)node.rotation[2]) * curMatrix;
			}

			if (node.scale.size() > 0)
			{
				maths::Vector3 scale = maths::Vector3((float)node.scale[0],(float)node.scale[1],(float)node.scale[2]);
				curMatrix = maths::Matrix4::Scale(scale) * curMatrix;
			}
		}

		return curMatrix;
	}

	PBRMataterialTextures LoadMaterial(tinygltf::Material gltfmaterial, tinygltf::Model gltfmodel)
	{
		const auto loadTextureFromParameter = [&](const tinygltf::ParameterMap& parameterMap, const String& textureName)
		{
			GLTFTexture texture{};

			const auto& textureIt = parameterMap.find(textureName);
			if (textureIt != std::end(parameterMap))
			{
				const int textureIndex = static_cast<int>(textureIt->second.json_double_value.at("index"));
				const tinygltf::Texture& gltfTexture = gltfmodel.textures.at(textureIndex);
				if (gltfTexture.source != -1)
				{
					texture.Image = &gltfmodel.images.at(gltfTexture.source);
				}

				if (gltfTexture.sampler != -1)
				{
					texture.Sampler = &gltfmodel.samplers.at(gltfTexture.sampler);
				}
			}

			return texture;
		};

		PBRMataterialTextures textures;

		GLTFTexture albedoTex = loadTextureFromParameter(gltfmaterial.values, AlbedoTexName);

		if (albedoTex.Image)
		{
			TextureParameters params = TextureParameters(GetFilter(albedoTex.Sampler->minFilter), GetWrapMode(albedoTex.Sampler->wrapS));

			Texture2D* texture = Texture2D::CreateFromSource(albedoTex.Image->width, albedoTex.Image->height, albedoTex.Image->image.data(), params);
			if (texture)
				textures.albedo = std::shared_ptr<Texture2D>(texture);//material->SetAlbedoMap(texture);
		}
		else
			textures.albedo = nullptr;

		GLTFTexture normalTex = loadTextureFromParameter(gltfmaterial.values, NormalTexName);

		if (normalTex.Image)
		{
			TextureParameters params = TextureParameters(GetFilter(normalTex.Sampler->minFilter), GetWrapMode(normalTex.Sampler->wrapS));

			Texture2D* texture = Texture2D::CreateFromSource(normalTex.Image->width, normalTex.Image->height, normalTex.Image->image.data(), params);
			if (texture)
				textures.normal = std::shared_ptr<Texture2D>(texture);//material->SetNormalMap(texture);
		}

		GLTFTexture specularTex = loadTextureFromParameter(gltfmaterial.values, MetallicTexName);

		if (specularTex.Image)
		{
			TextureParameters params = TextureParameters(GetFilter(specularTex.Sampler->minFilter), GetWrapMode(specularTex.Sampler->wrapS));

			Texture2D* texture = Texture2D::CreateFromSource(specularTex.Image->width, specularTex.Image->height, specularTex.Image->image.data(), params);
			if (texture)
				textures.metallic = std::shared_ptr<Texture2D>(texture);//material->SetSpecularMap(texture);
		}

		GLTFTexture glossTex = loadTextureFromParameter(gltfmaterial.values, GlossTexName);

		if (glossTex.Image)
		{
			TextureParameters params = TextureParameters(GetFilter(glossTex.Sampler->minFilter), GetWrapMode(glossTex.Sampler->wrapS));

			Texture2D* texture = Texture2D::CreateFromSource(glossTex.Image->width, glossTex.Image->height, glossTex.Image->image.data(), params);
			if (texture)
				textures.roughness = std::shared_ptr<Texture2D>(texture);//material->SetGlossMap(texture);
		}

		return textures;
	}

	std::vector<std::shared_ptr<Material>> LoadMaterials(tinygltf::Model &gltfModel)
		{
			std::vector<std::shared_ptr<Texture2D>> loadedTextures;
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
					TextureParameters params = TextureParameters(GetFilter(imageAndSampler.Sampler->minFilter), GetWrapMode(imageAndSampler.Sampler->wrapS));

					Texture2D* texture2D = Texture2D::CreateFromSource(imageAndSampler.Image->width, imageAndSampler.Image->height, imageAndSampler.Image->image.data(), params);
					if (texture2D)
						loadedTextures.push_back(std::shared_ptr<Texture2D>(texture2D));
				}
			}

			for (tinygltf::Material &mat : gltfModel.materials)
			{
				std::shared_ptr<Material> pbrMaterial = std::make_shared<Material>();
				PBRMataterialTextures textures;
				MaterialProperties properties;

				if (mat.values.find("baseColorTexture") != mat.values.end()) {
					textures.albedo = loadedTextures[gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source];
				}
				if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
					textures.metallic = loadedTextures[gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source];
				}
				if (mat.values.find("roughnessFactor") != mat.values.end()) {
				 	properties.glossColour = static_cast<float>(mat.values["roughnessFactor"].Factor());
				}
				if (mat.values.find("metallicFactor") != mat.values.end()) {
					properties.specularColour = maths::Vector4(static_cast<float>(mat.values["metallicFactor"].Factor()));
				}
				if (mat.values.find("baseColorFactor") != mat.values.end()) {
				 	properties.albedoColour = maths::Vector4((float)mat.values["baseColorFactor"].ColorFactor()[0],(float)mat.values["baseColorFactor"].ColorFactor()[1],(float)mat.values["baseColorFactor"].ColorFactor()[2],1.0f);
				}
				if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
					textures.normal = loadedTextures[gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source];
				}
				if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
					//textures.roughness = loadedTextures[gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source];
				}
				// if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				// 	material.occlusionTexture = &loadedTextures[gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source];
				// }
				// if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				// 	tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				// 	if (param.string_value == "BLEND") {
				// 		material.alphaMode = Material::ALPHAMODE_BLEND;
				// 	}
				// 	if (param.string_value == "MASK") {
				// 		material.alphaMode = Material::ALPHAMODE_MASK;
				// 	}
				// }
				// if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				// 	material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
				// }
				// if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				// 	material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
				// 	material.emissiveFactor = glm::vec4(0.0f);
				// }

				// Extensions
				// if (mat.extPBRValues.size() > 0) {
				// 	// KHR_materials_pbrSpecularGlossiness
				// 	if (mat.extPBRValues.find("specularGlossinessTexture") != mat.extPBRValues.end()) {
				// 		material.extension.specularGlossinessTexture = &textures[gltfModel.textures[mat.extPBRValues["specularGlossinessTexture"].TextureIndex()].source];
				// 		material.pbrWorkflows.specularGlossiness = true;
				// 	}
				// 	if (mat.extPBRValues.find("diffuseTexture") != mat.extPBRValues.end()) {
				// 		material.extension.diffuseTexture = &textures[gltfModel.textures[mat.extPBRValues["diffuseTexture"].TextureIndex()].source];
				// 	}
				// 	if (mat.extPBRValues.find("diffuseFactor") != mat.extPBRValues.end()) {
				// 		material.extension.diffuseFactor = glm::make_vec4(mat.extPBRValues["diffuseFactor"].ColorFactor().data());
				// 	}
				// 	if (mat.extPBRValues.find("specularFactor") != mat.extPBRValues.end()) {
				// 		material.extension.specularFactor = glm::vec4(glm::make_vec3(mat.extPBRValues["specularFactor"].ColorFactor().data()), 1.0);
				// 	}
				// }

				pbrMaterial->SetTextures(textures);
				pbrMaterial->SetMaterialProperites(properties);
				loadedMaterials.push_back(pbrMaterial);
			}

			return loadedMaterials;
		}

	void Model::LoadGLTF(const String & path)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string ext = StringFormat::GetFilePathExtension(path);

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
			JM_CORE_ERROR(err);
		}

		if (!warn.empty())
		{
			JM_CORE_ERROR(warn);
		}

		if (!ret)
		{
			JM_CORE_ERROR("Failed to parse glTF");
		}

		auto LoadedMaterials = LoadMaterials(model);

		for (auto& mesh : model.meshes)
		{
			if (mesh.primitives.size() > 1)
			{
				JM_CORE_WARN("UNIMPLEMENTED : glTF model with several primitives ", path, mesh.primitives.size());
			}

			const tinygltf::Primitive &primitive = mesh.primitives[0];
			const tinygltf::Accessor &indices = model.accessors[primitive.indices];
			//const tinygltf::BufferView &indexView = model.bufferViews[indices.bufferView];

			const uint numVertices = static_cast<uint>(indices.count);// attrib.vertices.size();// numIndices / 3.0f;
			Vertex* tempvertices = new Vertex[numVertices];
			uint* indicesArray = new uint[numVertices];

			size_t maxNumVerts = 0;

			std::shared_ptr<maths::BoundingSphere> boundingBox = std::make_shared<maths::BoundingSphere>();

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
				std::vector<byte> data = std::vector<byte>(first, last);

				// -------- Position attribute -----------

				if (attribute.first == "POSITION")
				{
					size_t positionCount = accessor.count;
					maxNumVerts = maths::Max(maxNumVerts, positionCount);
					maths::Vector3Simple* positions = reinterpret_cast<maths::Vector3Simple*>(data.data());
					for (auto p = 0; p < positionCount; ++p)
					{
						//positions[p] = glm::vec3(matrix * glm::vec4(positions[p], 1.0f));
						tempvertices[p].Position = maths::ToVector(positions[p]);

						boundingBox->ExpandToFit(tempvertices[p].Position);
					}
				}

				// -------- Normal attribute -----------

				else if (attribute.first == "NORMAL")
				{
					size_t normalCount = accessor.count;
					maxNumVerts = maths::Max(maxNumVerts, normalCount);
					maths::Vector3Simple* normals = reinterpret_cast<maths::Vector3Simple*>(data.data());
					for (auto p = 0; p < normalCount; ++p)
					{
						tempvertices[p].Normal = maths::ToVector(normals[p]);
					}
				}

				// -------- Texcoord attribute -----------

				else if (attribute.first == "TEXCOORD_0")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = maths::Max(maxNumVerts, uvCount);
					maths::Vector2Simple* uvs = reinterpret_cast<maths::Vector2Simple*>(data.data());
					for (auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].TexCoords = ToVector(uvs[p]);
					}
				}

				// -------- Colour attribute -----------

				else if (attribute.first == "COLOR_0")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = maths::Max(maxNumVerts, uvCount);
					maths::Vector4Simple* uvs = reinterpret_cast<maths::Vector4Simple*>(data.data());
					for (auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].Colours = ToVector(uvs[p]);
					}
				}

				// -------- Tangent attribute -----------

				else if (attribute.first == "TANGENT")
				{
					size_t uvCount = accessor.count;
					maxNumVerts = maths::Max(maxNumVerts, uvCount);
					maths::Vector3Simple* uvs = reinterpret_cast<maths::Vector3Simple*>(data.data());
					for (auto p = 0; p < uvCount; ++p)
					{
						tempvertices[p].Tangent = ToVector(uvs[p]);
					}
				}
			}

			std::shared_ptr<Material> pbrMaterial = LoadedMaterials[primitive.material];//std::make_shared<Material>();

			// if (!model.materials.empty())
			// {
			// 	auto material = model.materials[primitive.material];
			// 	pbrMaterial->SetTextures(LoadMaterial(material, model));
			// }

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(sizeof(Vertex) * numVertices, tempvertices);

			graphics::BufferLayout layout;
			layout.Push<maths::Vector3>("position");
			layout.Push<maths::Vector4>("colour");
			layout.Push<maths::Vector2>("texCoord");
			layout.Push<maths::Vector3>("normal");
			layout.Push<maths::Vector3>("tangent");
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
				std::vector<byte> data = std::vector<byte>(first, last);

				size_t indicesCount = indexAccessor.count;
				if (componentTypeByteSize == 2)
				{
					uint16_t* in = reinterpret_cast<uint16_t*>(data.data()); //TODO: Test different models to check size - uint32 or 16
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

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indicesArray, numVertices));

			m_Meshes.push_back(std::make_shared<Mesh>(va, ib, pbrMaterial, boundingBox));

			delete[] tempvertices;
			delete[] indicesArray;
		}

	}

}
