#include "LM.h"
#include "Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/ShaderResource.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{

	Material::Material(std::shared_ptr<Shader>& shader, const MaterialProperties& properties, const PBRMataterialTextures& textures) : m_PBRMaterialTextures(textures), m_Shader(shader)
	{
		m_RenderFlags = 0;
		SetRenderFlag(RenderFlags::DEFERREDRENDER);
		m_DescriptorSet = nullptr;
		m_MaterialProperties = new MaterialProperties();
        SetMaterialProperites(properties);
		m_MaterialPropertiesBuffer = nullptr;
        m_MaterialBufferSize = sizeof(MaterialProperties);
        m_MaterialBufferData = new byte[m_MaterialBufferSize];
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
		m_PBRMaterialTextures.metallic  = textures.metallic;
	}

	void Material::LoadPBRMaterial(const String& name, const String& path, const String& extension)
	{
		m_PBRMaterialTextures = PBRMataterialTextures();
        auto params = TextureParameters(TextureFormat::RGBA, TextureFilter::LINEAR, TextureWrap::CLAMP_TO_EDGE);
        
        m_PBRMaterialTextures.albedo    = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(name, path + "/" + name + "/albedo" + extension,params));
		m_PBRMaterialTextures.normal    = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(name, path + "/" + name + "/normal" + extension,params));
		m_PBRMaterialTextures.roughness = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(name, path + "/" + name + "/roughness" + extension,params));
		m_PBRMaterialTextures.metallic  = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(name, path + "/" + name + "/metallic" + extension,params));
	}

	void Material::LoadMaterial(const String& name, const String& path)
	{
		m_PBRMaterialTextures = PBRMataterialTextures();
		m_PBRMaterialTextures.albedo    = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(name, path));
		m_PBRMaterialTextures.normal    = nullptr;
		m_PBRMaterialTextures.roughness = nullptr;
		m_PBRMaterialTextures.metallic  = nullptr;
	}

    void Material::UpdateMaterialPropertiesData()
    {
		memcpy(m_MaterialBufferData, m_MaterialProperties, sizeof(MaterialProperties));
    }

    void Material::SetMaterialProperites(const MaterialProperties &properties)
    {
        m_MaterialProperties->albedoColour      = properties.albedoColour;
        m_MaterialProperties->specularColour    = properties.specularColour;
        m_MaterialProperties->glossColour       = properties.glossColour;
        m_MaterialProperties->usingAlbedoMap    = properties.usingAlbedoMap;
        m_MaterialProperties->usingNormalMap    = properties.usingNormalMap;
        m_MaterialProperties->usingSpecularMap  = properties.usingSpecularMap;
        m_MaterialProperties->usingGlossMap     = properties.usingGlossMap;

        UpdateMaterialPropertiesData();
    }

	void Material::CreateDescriptorSet(graphics::api::Pipeline* pipeline, int layoutID)
	{
        if(m_DescriptorSet)
            delete m_DescriptorSet;

		m_Pipeline = pipeline;

		graphics::api::DescriptorInfo info;
		info.pipeline = pipeline;
		info.layoutIndex = layoutID;
		info.shader = pipeline->GetShader();

		if(m_MaterialPropertiesBuffer == nullptr)
		{
			m_MaterialPropertiesBuffer = graphics::api::UniformBuffer::Create();

			m_MaterialBufferSize = static_cast<uint32_t>(sizeof(MaterialProperties));
			m_MaterialPropertiesBuffer->Init(m_MaterialBufferSize, nullptr);
		}

		m_DescriptorSet = graphics::api::DescriptorSet::Create(info);

		std::vector<graphics::api::ImageInfo> imageInfos;
		std::vector<graphics::api::BufferInfo> bufferInfos;

		if(m_PBRMaterialTextures.albedo.get() != nullptr)
		{
			graphics::api::ImageInfo imageInfo1 = {};
			imageInfo1.texture = m_PBRMaterialTextures.albedo.get();
			imageInfo1.binding = 0;
			imageInfo1.name = "u_AlbedoMap";
			imageInfos.push_back(imageInfo1);
		}
		else
			m_MaterialProperties->usingAlbedoMap = 0.0f;

		if(m_PBRMaterialTextures.metallic.get() != nullptr)
		{
			graphics::api::ImageInfo imageInfo2 = {};
			imageInfo2.texture = m_PBRMaterialTextures.metallic.get();
			imageInfo2.binding = 1;
			imageInfo2.name = "u_SpecularMap";
			imageInfos.push_back(imageInfo2);
		}
		else
			m_MaterialProperties->usingSpecularMap = 0.0f;

		if(m_PBRMaterialTextures.roughness.get() != nullptr)
		{
			graphics::api::ImageInfo imageInfo3 = {};
			imageInfo3.texture = m_PBRMaterialTextures.roughness.get();
			imageInfo3.binding = 2;
			imageInfo3.name = "u_GlossMap";
			imageInfos.push_back(imageInfo3);
		}
		else
			m_MaterialProperties->usingGlossMap = 0.0f;

		if(m_PBRMaterialTextures.normal.get() != nullptr)
		{
			graphics::api::ImageInfo imageInfo4 = {};
			imageInfo4.texture = m_PBRMaterialTextures.normal.get();
			imageInfo4.binding = 3;
			imageInfo4.name = "u_NormalMap";
			imageInfos.push_back(imageInfo4);
		}
		else
			m_MaterialProperties->usingNormalMap = 0.0f;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.buffer = m_MaterialPropertiesBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(MaterialProperties);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 4;
		bufferInfo.shaderType = ShaderType::FRAGMENT;
		bufferInfo.name = "UniformMaterialData";
		bufferInfo.systemUniforms = false;

		bufferInfos.push_back(bufferInfo);

        UpdateMaterialPropertiesData();
		m_MaterialPropertiesBuffer->SetData(m_MaterialBufferSize, *&m_MaterialBufferData);

		m_DescriptorSet->Update(imageInfos, bufferInfos);
	}
}
