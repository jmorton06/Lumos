#include "LM.h"
#include "Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/ShaderResource.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Shader.h"

#include "System/VFS.h"

namespace lumos
{

	Material::Material(std::shared_ptr<graphics::Shader>& shader, const MaterialProperties& properties, const PBRMataterialTextures& textures) : m_PBRMaterialTextures(textures), m_Shader(shader)
	{
		m_RenderFlags = 0;
		SetRenderFlag(RenderFlags::DEFERREDRENDER);
		m_DescriptorSet = nullptr;
		m_MaterialProperties = new MaterialProperties();
        SetMaterialProperites(properties);
		m_MaterialPropertiesBuffer = nullptr;
        m_MaterialBufferSize = sizeof(MaterialProperties);
        m_MaterialBufferData = new byte[m_MaterialBufferSize];
		m_Pipeline = nullptr;
	}

	Material::Material() : m_Shader(nullptr)
	{
		m_RenderFlags = 0;
		SetRenderFlag(RenderFlags::DEFERREDRENDER);
		m_DescriptorSet = nullptr;
		m_MaterialPropertiesBuffer = nullptr;
		m_MaterialProperties = new MaterialProperties();
		m_PBRMaterialTextures.albedo = nullptr;

        m_MaterialBufferSize = sizeof(MaterialProperties);
        m_MaterialBufferData = new byte[m_MaterialBufferSize];

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
		m_PBRMaterialTextures.albedo    = textures.albedo;
		m_PBRMaterialTextures.normal    = textures.normal;
		m_PBRMaterialTextures.roughness = textures.roughness;
		m_PBRMaterialTextures.specular = textures.specular;
		m_PBRMaterialTextures.ao		= textures.ao;
		m_PBRMaterialTextures.emissive  = textures.emissive;
	}

	bool FileExists(const String& path)
	{
		std::string physicalPath;
		if (!VFS::Get()->ResolvePhysicalPath(path, physicalPath))
			return false;

		return true;
	}

	void Material::LoadPBRMaterial(const String& name, const String& path, const String& extension)
	{
		m_Name = name;
		m_PBRMaterialTextures = PBRMataterialTextures();
        auto params = graphics::TextureParameters(graphics::TextureFormat::RGBA, graphics::TextureFilter::LINEAR, graphics::TextureWrap::CLAMP_TO_EDGE);

		auto filePath = path + "/" + name + "/albedo" + extension;

		if(FileExists(filePath))
			m_PBRMaterialTextures.albedo    = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/albedo" + extension,params));

		filePath = path + "/" + name + "/normal" + extension;

		if (FileExists(filePath))
		m_PBRMaterialTextures.normal    = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/normal" + extension,params));

		filePath = path + "/" + name + "/roughness" + extension;

		if (FileExists(filePath))
		m_PBRMaterialTextures.roughness = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/roughness" + extension,params));

		filePath = path + "/" + name + "/metallic" + extension;

		if (FileExists(filePath))
		m_PBRMaterialTextures.specular = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/metallic" + extension,params));

		filePath = path + "/" + name + "/ao" + extension;

		if (FileExists(filePath))
		m_PBRMaterialTextures.ao		= std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/ao" + extension, params));

		filePath = path + "/" + name + "/emissive" + extension;

		if (FileExists(filePath))
			m_PBRMaterialTextures.emissive = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/emissive" + extension, params));

	}

	void Material::LoadMaterial(const String& name, const String& path)
	{
		m_Name = name;
		m_PBRMaterialTextures = PBRMataterialTextures();
		m_PBRMaterialTextures.albedo    = std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile(name, path));
		m_PBRMaterialTextures.normal    = nullptr;
		m_PBRMaterialTextures.roughness = nullptr;
		m_PBRMaterialTextures.specular  = nullptr;
		m_PBRMaterialTextures.ao		= nullptr;
		m_PBRMaterialTextures.emissive  = nullptr;
	}

    void Material::UpdateMaterialPropertiesData()
    {
		memcpy(m_MaterialBufferData, m_MaterialProperties, sizeof(MaterialProperties));
    }

    void Material::SetMaterialProperites(const MaterialProperties &properties)
    {
        m_MaterialProperties->albedoColour      = properties.albedoColour;
        m_MaterialProperties->specularColour    = properties.specularColour;
        m_MaterialProperties->roughnessColour   = properties.roughnessColour;
        m_MaterialProperties->usingAlbedoMap    = properties.usingAlbedoMap;
        m_MaterialProperties->usingNormalMap    = properties.usingNormalMap;
        m_MaterialProperties->usingSpecularMap  = properties.usingSpecularMap;
        m_MaterialProperties->usingRoughnessMap = properties.usingRoughnessMap;
		m_MaterialProperties->usingAOMap		= properties.usingAOMap;
		m_MaterialProperties->usingEmissiveMap  = properties.usingEmissiveMap;
		m_MaterialProperties->workflow			= properties.workflow;

        UpdateMaterialPropertiesData();

		if(m_MaterialPropertiesBuffer)
			m_MaterialPropertiesBuffer->SetData(m_MaterialBufferSize, *&m_MaterialBufferData);
    }

	void Material::CreateDescriptorSet(graphics::Pipeline* pipeline, int layoutID, bool pbr)
	{
        if(m_DescriptorSet)
            delete m_DescriptorSet;

		m_Pipeline = pipeline;

		graphics::DescriptorInfo info;
		info.pipeline = pipeline;
		info.layoutIndex = layoutID;
		info.shader = pipeline->GetShader();

		if(m_MaterialPropertiesBuffer == nullptr && pbr)
		{
			m_MaterialPropertiesBuffer = graphics::UniformBuffer::Create();

			m_MaterialBufferSize = static_cast<uint32_t>(sizeof(MaterialProperties));
			m_MaterialPropertiesBuffer->Init(m_MaterialBufferSize, nullptr);
		}

		m_DescriptorSet = graphics::DescriptorSet::Create(info);

		std::vector<graphics::ImageInfo> imageInfos;
		std::vector<graphics::BufferInfo> bufferInfos;

		if(m_PBRMaterialTextures.albedo != nullptr)
		{
			graphics::ImageInfo imageInfo1 = {};
			imageInfo1.texture = { m_PBRMaterialTextures.albedo.get() };
			imageInfo1.binding = 0;
			imageInfo1.name = "u_AlbedoMap";
			imageInfos.push_back(imageInfo1);
		}
		else
			m_MaterialProperties->usingAlbedoMap = 0.0f;

		if(m_PBRMaterialTextures.specular != nullptr)
		{
			graphics::ImageInfo imageInfo2 = {};
			imageInfo2.texture ={ m_PBRMaterialTextures.specular.get() };
			imageInfo2.binding = 1;
			imageInfo2.name = "u_SpecularMap";
			imageInfos.push_back(imageInfo2);
		}
		else
			m_MaterialProperties->usingSpecularMap = 0.0f;

		if(m_PBRMaterialTextures.roughness != nullptr)
		{
			graphics::ImageInfo imageInfo3 = {};
            imageInfo3.texture = { m_PBRMaterialTextures.roughness.get() };
			imageInfo3.binding = 2;
			imageInfo3.name = "u_RoughnessMap";
			imageInfos.push_back(imageInfo3);
		}
		else
			m_MaterialProperties->usingRoughnessMap = 0.0f;

		if(m_PBRMaterialTextures.normal != nullptr)
		{
			graphics::ImageInfo imageInfo4 = {};
			imageInfo4.texture = { m_PBRMaterialTextures.normal.get() };
			imageInfo4.binding = 3;
			imageInfo4.name = "u_NormalMap";
			imageInfos.push_back(imageInfo4);
		}
		else
			m_MaterialProperties->usingNormalMap = 0.0f;

		if (m_PBRMaterialTextures.ao != nullptr)
		{
			graphics::ImageInfo imageInfo5 = {};
			imageInfo5.texture = { m_PBRMaterialTextures.ao.get() };
			imageInfo5.binding = 4;
			imageInfo5.name = "u_AOMap";
			imageInfos.push_back(imageInfo5);
		}
		else
			m_MaterialProperties->usingAOMap = 0.0f;

		if (m_PBRMaterialTextures.emissive != nullptr)
		{
			graphics::ImageInfo imageInfo6 = {};
			imageInfo6.texture = { m_PBRMaterialTextures.emissive.get() };
			imageInfo6.binding = 5;
			imageInfo6.name = "u_EmissiveMap";
			imageInfos.push_back(imageInfo6);
		}
		else
			m_MaterialProperties->usingEmissiveMap = 0.0f;

		if (pbr)
		{
			graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_MaterialPropertiesBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(MaterialProperties);
			bufferInfo.type = graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 6;
			bufferInfo.shaderType = graphics::ShaderType::FRAGMENT;
			bufferInfo.name = "UniformMaterialData";
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			UpdateMaterialPropertiesData();
			m_MaterialPropertiesBuffer->SetData(m_MaterialBufferSize, *&m_MaterialBufferData);
		}

		m_DescriptorSet->Update(imageInfos, bufferInfos);
	}
}
