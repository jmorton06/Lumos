#include "LM.h"
#include "Material.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/ShaderResource.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Core/FileSystem.h"
#include "Core/VFS.h"

#include <imgui/imgui.h>

namespace Lumos
{

    Material::Material(Ref<Graphics::Shader>& shader, const MaterialProperties& properties, const PBRMataterialTextures& textures) : m_PBRMaterialTextures(textures), m_Shader(shader)
    {
        m_RenderFlags = 0;
        SetRenderFlag(RenderFlags::DEFERREDRENDER);
        m_DescriptorSet = nullptr;
        m_MaterialProperties = lmnew MaterialProperties();
        SetMaterialProperites(properties);
        m_MaterialPropertiesBuffer = nullptr;
        m_MaterialBufferSize = sizeof(MaterialProperties);
        m_MaterialBufferData = lmnew u8[m_MaterialBufferSize];
        m_Pipeline = nullptr;
    }

    Material::Material() : m_Shader(nullptr)
    {
        m_RenderFlags = 0;
        SetRenderFlag(RenderFlags::DEFERREDRENDER);
        m_DescriptorSet = nullptr;
        m_MaterialPropertiesBuffer = nullptr;
        m_MaterialProperties = lmnew MaterialProperties();
        m_PBRMaterialTextures.albedo = nullptr;

        m_MaterialBufferSize = sizeof(MaterialProperties);
        m_MaterialBufferData = lmnew u8[m_MaterialBufferSize];

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
        m_PBRMaterialTextures.specular  = textures.specular;
        m_PBRMaterialTextures.ao        = textures.ao;
        m_PBRMaterialTextures.emissive  = textures.emissive;
    }

    bool FileExists(const String& path)
    {
        String physicalPath;
        
        VFS::Get()->ResolvePhysicalPath(path,physicalPath);
        return FileSystem::FileExists(physicalPath);
    }

    void Material::LoadPBRMaterial(const String& name, const String& path, const String& extension)
    {
        m_Name = name;
        m_PBRMaterialTextures = PBRMataterialTextures();
        auto params = Graphics::TextureParameters(Graphics::TextureFormat::RGBA, Graphics::TextureFilter::LINEAR, Graphics::TextureWrap::CLAMP_TO_EDGE);

        auto filePath = path + "/" + name + "/albedo" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.albedo    = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/albedo" + extension,params));

        filePath = path + "/" + name + "/normal" + extension;

        if (FileExists(filePath))
        m_PBRMaterialTextures.normal    = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/normal" + extension,params));

        filePath = path + "/" + name + "/roughness" + extension;

        if (FileExists(filePath))
        m_PBRMaterialTextures.roughness = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/roughness" + extension,params));

        filePath = path + "/" + name + "/metallic" + extension;

        if (FileExists(filePath))
        m_PBRMaterialTextures.specular = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/metallic" + extension,params));

        filePath = path + "/" + name + "/ao" + extension;

        if (FileExists(filePath))
        m_PBRMaterialTextures.ao        = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/ao" + extension, params));

        filePath = path + "/" + name + "/emissive" + extension;

        if (FileExists(filePath))
            m_PBRMaterialTextures.emissive = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/emissive" + extension, params));

    }

    void Material::LoadMaterial(const String& name, const String& path)
    {
        m_Name = name;
        m_PBRMaterialTextures = PBRMataterialTextures();
        m_PBRMaterialTextures.albedo    = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
        m_PBRMaterialTextures.normal    = nullptr;
        m_PBRMaterialTextures.roughness = nullptr;
        m_PBRMaterialTextures.specular  = nullptr;
        m_PBRMaterialTextures.ao        = nullptr;
        m_PBRMaterialTextures.emissive  = nullptr;
    }

    void Material::UpdateMaterialPropertiesData()
    {
        memcpy(m_MaterialBufferData, m_MaterialProperties, sizeof(MaterialProperties));
    }

	void Material::OnImGui()
	{
		bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

		MaterialProperties* prop = GetProperties();

		if (ImGui::TreeNode("Albedo"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().albedo;

			if (tex)
			{
				ImGui::Image(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}


			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use Albedo Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseAlbedoMap", &prop->usingAlbedoMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Albedo");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::ColorEdit4("##Albedo", &prop->albedoColour.x);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Normal"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().normal;

			if (tex)
			{
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use Normal Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseNormalMap", &prop->usingNormalMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Specular"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().specular;

			if (tex)
			{
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use Specular Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseSpecularMap", &prop->usingSpecularMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Specular");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat3("##Specular", &prop->specularColour.x, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Roughness"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().roughness;
			if (tex)
			{
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use Roughness Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseRoughnessMap", &prop->usingRoughnessMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Roughness");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat3("##Roughness", &prop->roughnessColour.x, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Ao"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().ao;
			if (tex)
			{
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use AO Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseAOMap", &prop->usingAOMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Emissive"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = GetTextures().emissive;
			if (tex)
			{
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::Button("Empty", ImVec2(64, 64));
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Use Emissive Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseEmissiveMap", &prop->usingEmissiveMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		SetMaterialProperites(*prop);
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
        m_MaterialProperties->usingAOMap        = properties.usingAOMap;
        m_MaterialProperties->usingEmissiveMap  = properties.usingEmissiveMap;
        m_MaterialProperties->workflow          = properties.workflow;

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
            imageInfo1.texture = { m_PBRMaterialTextures.albedo.get() };
            imageInfo1.binding = 0;
            imageInfo1.name = "u_AlbedoMap";
            imageInfos.push_back(imageInfo1);
        }
        else
            m_MaterialProperties->usingAlbedoMap = 0.0f;

        if(m_PBRMaterialTextures.specular != nullptr)
        {
            Graphics::ImageInfo imageInfo2 = {};
            imageInfo2.texture ={ m_PBRMaterialTextures.specular.get() };
            imageInfo2.binding = 1;
            imageInfo2.name = "u_SpecularMap";
            imageInfos.push_back(imageInfo2);
        }
        else
            m_MaterialProperties->usingSpecularMap = 0.0f;

        if(m_PBRMaterialTextures.roughness != nullptr)
        {
            Graphics::ImageInfo imageInfo3 = {};
            imageInfo3.texture = { m_PBRMaterialTextures.roughness.get() };
            imageInfo3.binding = 2;
            imageInfo3.name = "u_RoughnessMap";
            imageInfos.push_back(imageInfo3);
        }
        else
            m_MaterialProperties->usingRoughnessMap = 0.0f;

        if(m_PBRMaterialTextures.normal != nullptr)
        {
            Graphics::ImageInfo imageInfo4 = {};
            imageInfo4.texture = { m_PBRMaterialTextures.normal.get() };
            imageInfo4.binding = 3;
            imageInfo4.name = "u_NormalMap";
            imageInfos.push_back(imageInfo4);
        }
        else
            m_MaterialProperties->usingNormalMap = 0.0f;

        if (m_PBRMaterialTextures.ao != nullptr)
        {
            Graphics::ImageInfo imageInfo5 = {};
            imageInfo5.texture = { m_PBRMaterialTextures.ao.get() };
            imageInfo5.binding = 4;
            imageInfo5.name = "u_AOMap";
            imageInfos.push_back(imageInfo5);
        }
        else
            m_MaterialProperties->usingAOMap = 0.0f;

        if (m_PBRMaterialTextures.emissive != nullptr)
        {
            Graphics::ImageInfo imageInfo6 = {};
            imageInfo6.texture = { m_PBRMaterialTextures.emissive.get() };
            imageInfo6.binding = 5;
            imageInfo6.name = "u_EmissiveMap";
            imageInfos.push_back(imageInfo6);
        }
        else
            m_MaterialProperties->usingEmissiveMap = 0.0f;

        if (pbr)
        {
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
}
