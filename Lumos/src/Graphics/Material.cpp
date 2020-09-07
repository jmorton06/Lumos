#include "Precompiled.h"
#include "Material.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Core/OS/FileSystem.h"
#include "Core/VFS.h"

#include <imgui/imgui.h>

namespace Lumos::Graphics
{

	Ref<Graphics::Texture2D> Material::s_DefaultTexture = nullptr;

	Material::Material(Ref<Graphics::Shader>& shader, const MaterialProperties& properties, const PBRMataterialTextures& textures)
		: m_PBRMaterialTextures(textures)
		, m_Shader(shader)
	{
		m_RenderFlags = 0;
		SetRenderFlag(RenderFlags::DEFERREDRENDER);
		m_DescriptorSet = nullptr;
		m_MaterialProperties = new MaterialProperties();
		SetMaterialProperites(properties);
		m_MaterialPropertiesBuffer = nullptr;
		m_MaterialBufferSize = sizeof(MaterialProperties);
		m_MaterialBufferData = new u8[m_MaterialBufferSize];
		m_Pipeline = nullptr;
	}

	Material::Material()
		: m_Shader(nullptr)
	{
		m_RenderFlags = 0;
		SetRenderFlag(RenderFlags::DEFERREDRENDER);
		m_DescriptorSet = nullptr;
		m_MaterialPropertiesBuffer = nullptr;
		m_MaterialProperties = new MaterialProperties();
		m_PBRMaterialTextures.albedo = nullptr;

		m_MaterialBufferSize = sizeof(MaterialProperties);
		m_MaterialBufferData = new u8[m_MaterialBufferSize];

		m_Pipeline = nullptr;
	}

	Material::~Material()
	{
		delete m_DescriptorSet;
		delete m_MaterialProperties;
		delete m_MaterialPropertiesBuffer;
		delete[] m_MaterialBufferData;
	}

	void Material::SetTextures(const PBRMataterialTextures& textures)
	{
		m_PBRMaterialTextures.albedo = textures.albedo;
		m_PBRMaterialTextures.normal = textures.normal;
		m_PBRMaterialTextures.roughness = textures.roughness;
		m_PBRMaterialTextures.metallic = textures.metallic;
		m_PBRMaterialTextures.ao = textures.ao;
		m_PBRMaterialTextures.emissive = textures.emissive;
	}

	bool FileExists(const std::string& path)
	{
		std::string physicalPath;

		VFS::Get()->ResolvePhysicalPath(path, physicalPath);
		return FileSystem::FileExists(physicalPath);
	}

	void Material::LoadPBRMaterial(const std::string& name, const std::string& path, const std::string& extension)
	{
		m_Name = name;
		m_PBRMaterialTextures = PBRMataterialTextures();
		auto params = Graphics::TextureParameters(Graphics::TextureFormat::RGBA, Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR, Graphics::TextureWrap::CLAMP_TO_EDGE);

		auto filePath = path + "/" + name + "/albedo" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.albedo = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/albedo" + extension, params));

		filePath = path + "/" + name + "/normal" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.normal = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/normal" + extension, params));

		filePath = path + "/" + name + "/roughness" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.roughness = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/roughness" + extension, params));

		filePath = path + "/" + name + "/metallic" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.metallic = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/metallic" + extension, params));

		filePath = path + "/" + name + "/ao" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.ao = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/ao" + extension, params));

		filePath = path + "/" + name + "/emissive" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.emissive = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/emissive" + extension, params));
	}

	void Material::LoadMaterial(const std::string& name, const std::string& path)
	{
		m_Name = name;
		m_PBRMaterialTextures = PBRMataterialTextures();
		m_PBRMaterialTextures.albedo = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
		m_PBRMaterialTextures.normal = nullptr;
		m_PBRMaterialTextures.roughness = nullptr;
		m_PBRMaterialTextures.metallic = nullptr;
		m_PBRMaterialTextures.ao = nullptr;
		m_PBRMaterialTextures.emissive = nullptr;
	}

	void Material::UpdateMaterialPropertiesData()
	{
		memcpy(m_MaterialBufferData, m_MaterialProperties, sizeof(MaterialProperties));
	}

	void Material::SetMaterialProperites(const MaterialProperties& properties)
	{
		m_MaterialProperties->albedoColour = properties.albedoColour;
		m_MaterialProperties->metallicColour = properties.metallicColour;
		m_MaterialProperties->roughnessColour = properties.roughnessColour;
		m_MaterialProperties->usingAlbedoMap = properties.usingAlbedoMap;
		m_MaterialProperties->usingNormalMap = properties.usingNormalMap;
		m_MaterialProperties->usingMetallicMap = properties.usingMetallicMap;
		m_MaterialProperties->usingRoughnessMap = properties.usingRoughnessMap;
		m_MaterialProperties->usingAOMap = properties.usingAOMap;
		m_MaterialProperties->usingEmissiveMap = properties.usingEmissiveMap;
		m_MaterialProperties->workflow = properties.workflow;
		m_MaterialProperties->emissiveColour = properties.emissiveColour;

		UpdateMaterialPropertiesData();

		if(m_MaterialPropertiesBuffer)
			m_MaterialPropertiesBuffer->SetData(m_MaterialBufferSize, *&m_MaterialBufferData);
	}

	void Material::CreateDescriptorSet(Graphics::Pipeline* pipeline, int layoutID, bool pbr)
	{
		if(m_DescriptorSet)
			delete m_DescriptorSet;

		m_Pipeline = pipeline;

		Graphics::DescriptorInfo info;
		info.pipeline = pipeline;
		info.layoutIndex = layoutID;
		info.shader = pipeline->GetShader();

		if(m_MaterialPropertiesBuffer == nullptr && pbr)
		{
			m_MaterialPropertiesBuffer = Graphics::UniformBuffer::Create();

			m_MaterialBufferSize = static_cast<uint32_t>(sizeof(MaterialProperties));
			m_MaterialPropertiesBuffer->Init(m_MaterialBufferSize, nullptr);
		}

		m_DescriptorSet = Graphics::DescriptorSet::Create(info);

		std::vector<Graphics::ImageInfo> imageInfos;
		std::vector<Graphics::BufferInfo> bufferInfos;

		if(m_PBRMaterialTextures.albedo != nullptr)
		{
			Graphics::ImageInfo imageInfo1 = {};
			imageInfo1.texture = {m_PBRMaterialTextures.albedo.get()};
			imageInfo1.binding = 0;
			imageInfo1.name = "u_AlbedoMap";
			imageInfos.push_back(imageInfo1);
		}
		else
		{
			Graphics::ImageInfo imageInfo1 = {};
			imageInfo1.texture = {s_DefaultTexture.get()};
			imageInfo1.binding = 0;
			imageInfo1.name = "u_AlbedoMap";
			imageInfos.push_back(imageInfo1);
			m_MaterialProperties->usingAlbedoMap = 0.0f;
		}

		if(pbr)
		{
			if(m_PBRMaterialTextures.metallic != nullptr)
			{
				Graphics::ImageInfo imageInfo2 = {};
				imageInfo2.texture = {m_PBRMaterialTextures.metallic.get()};
				imageInfo2.binding = 1;
				imageInfo2.name = "u_MetallicMap";
				imageInfos.push_back(imageInfo2);
			}
			else
			{
				Graphics::ImageInfo imageInfo2 = {};
				imageInfo2.texture = {s_DefaultTexture.get()};
				imageInfo2.binding = 1;
				imageInfo2.name = "u_MetallicMap";
				imageInfos.push_back(imageInfo2);
				m_MaterialProperties->usingMetallicMap = 0.0f;
			}

			if(m_PBRMaterialTextures.roughness != nullptr)
			{
				Graphics::ImageInfo imageInfo3 = {};
				imageInfo3.texture = {m_PBRMaterialTextures.roughness.get()};
				imageInfo3.binding = 2;
				imageInfo3.name = "u_RoughnessMap";
				imageInfos.push_back(imageInfo3);
			}
			else
			{
				Graphics::ImageInfo imageInfo3 = {};
				imageInfo3.texture = {s_DefaultTexture.get()};
				imageInfo3.binding = 2;
				imageInfo3.name = "u_RoughnessMap";
				imageInfos.push_back(imageInfo3);
				m_MaterialProperties->usingRoughnessMap = 0.0f;
			}

			if(m_PBRMaterialTextures.normal != nullptr)
			{
				Graphics::ImageInfo imageInfo4 = {};
				imageInfo4.texture = {m_PBRMaterialTextures.normal.get()};
				imageInfo4.binding = 3;
				imageInfo4.name = "u_NormalMap";
				imageInfos.push_back(imageInfo4);
			}
			else
			{
				Graphics::ImageInfo imageInfo4 = {};
				imageInfo4.texture = {s_DefaultTexture.get()};
				imageInfo4.binding = 3;
				imageInfo4.name = "u_NormalMap";
				imageInfos.push_back(imageInfo4);
				m_MaterialProperties->usingNormalMap = 0.0f;
			}

			if(m_PBRMaterialTextures.ao != nullptr)
			{
				Graphics::ImageInfo imageInfo5 = {};
				imageInfo5.texture = {m_PBRMaterialTextures.ao.get()};
				imageInfo5.binding = 4;
				imageInfo5.name = "u_AOMap";
				imageInfos.push_back(imageInfo5);
			}
			else
			{
				Graphics::ImageInfo imageInfo5 = {};
				imageInfo5.texture = {s_DefaultTexture.get()};
				imageInfo5.binding = 5;
				imageInfo5.name = "u_AOMap";
				imageInfos.push_back(imageInfo5);
				m_MaterialProperties->usingAOMap = 0.0f;
			}

			if(m_PBRMaterialTextures.emissive != nullptr)
			{
				Graphics::ImageInfo imageInfo6 = {};
				imageInfo6.texture = {m_PBRMaterialTextures.emissive.get()};
				imageInfo6.binding = 5;
				imageInfo6.name = "u_EmissiveMap";
				imageInfos.push_back(imageInfo6);
			}
			else
			{
				Graphics::ImageInfo imageInfo6 = {};
				imageInfo6.texture = {s_DefaultTexture.get()};
				imageInfo6.binding = 5;
				imageInfo6.name = "u_EmissiveMap";
				imageInfos.push_back(imageInfo6);
				m_MaterialProperties->usingEmissiveMap = 0.0f;
			}

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_MaterialPropertiesBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(MaterialProperties);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 6;
			bufferInfo.shaderType = Graphics::ShaderType::FRAGMENT;
			bufferInfo.name = "UniformMaterialData";
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			UpdateMaterialPropertiesData();
			m_MaterialPropertiesBuffer->SetData(m_MaterialBufferSize, *&m_MaterialBufferData);
		}

		m_DescriptorSet->Update(imageInfos, bufferInfos);
	}

	void Material::InitDefaultTexture()
	{
		uint32_t whiteTextureData = 0xffffffff;
		s_DefaultTexture = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromSource(1, 1, &whiteTextureData));
	}

	void Material::ReleaseDefaultTexture()
	{
		s_DefaultTexture.reset();
	}

    void Material::SetAlbedoTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.albedo = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetNormalTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.normal = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetRoughnessTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.roughness = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetMetallicTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.metallic = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetAOTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.ao = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetEmissiveTexture(const std::string& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.emissive = tex;
            m_TexturesUpdated = true;
        }
    }
}
