#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/Transform.h"
#include "Maths/BoundingBox.h"
#include "Graphics/RHI/Texture.h"
#include "Utilities/StringUtilities.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <ModelLoaders/tinyobjloader/tiny_obj_loader.h>

namespace Lumos
{
    std::string m_Directory;
    TDArray<SharedPtr<Graphics::Texture2D>> m_Textures;

    SharedPtr<Graphics::Texture2D> LoadMaterialTextures(const std::string& typeName, TDArray<SharedPtr<Graphics::Texture2D>>& textures_loaded, const std::string& name, const std::string& directory, Graphics::TextureDesc format)
    {
        for(uint32_t j = 0; j < textures_loaded.Size(); j++)
        {
            if(std::strcmp(textures_loaded[j]->GetFilepath().c_str(), (directory + name).c_str()) == 0)
            {
                return textures_loaded[j];
            }
        }

        { // If texture hasn't been loaded already, load it
            Graphics::TextureLoadOptions options(false, true);
            std::string filePath = directory + name;
            filePath             = StringUtilities::BackSlashesToSlashes(filePath);
            auto texture         = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(typeName, filePath, format, options));
            textures_loaded.PushBack(texture); // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.

            return texture;
        }
    }

    void Graphics::Model::LoadOBJ(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string resolvedPath = path;
        tinyobj::attrib_t attrib;
        std::string error;

        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        resolvedPath = StringUtilities::BackSlashesToSlashes(resolvedPath);
        m_Directory  = StringUtilities::GetFileLocation(resolvedPath);

        std::string name = StringUtilities::GetFileName(resolvedPath);

        bool ok = tinyobj::LoadObj(
            &attrib, &shapes, &materials, &error, (resolvedPath).c_str(), (m_Directory).c_str());

        if(!ok)
        {
            LFATAL(error.c_str());
        }

        bool singleMesh = shapes.size() == 1;

        for(const auto& shape : shapes)
        {
            uint32_t vertexCount       = 0;
            const uint32_t numIndices  = static_cast<uint32_t>(shape.mesh.indices.size());
            const uint32_t numVertices = numIndices; // attrib.vertices.size();// numIndices / 3.0f;
            TDArray<Graphics::Vertex> vertices(numVertices);
            TDArray<uint32_t> indices(numIndices);

            SharedPtr<Maths::BoundingBox> boundingBox = CreateSharedPtr<Maths::BoundingBox>();

            for(uint32_t i = 0; i < shape.mesh.indices.size(); i++)
            {
                auto& index = shape.mesh.indices[i];
                Graphics::Vertex vertex;

                if(!attrib.texcoords.empty())
                {
                    vertex.TexCoords = (Vec2(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]));
                }
                else
                {
                    vertex.TexCoords = Vec2(0.0f, 0.0f);
                }
                vertex.Position = (Vec3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]));

                boundingBox->Merge(vertex.Position);

                if(!attrib.normals.empty())
                {
                    vertex.Normal = (Vec3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]));
                }

                Vec4 colour = Vec4(0.0f);

                if(shape.mesh.material_ids[0] >= 0)
                {
                    tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];
                    colour                  = Vec4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f);
                }

                vertex.Colours = colour;

                /*         if(uniqueVertices.count(vertex) == 0)
                         {
                             uniqueVertices[vertex] = static_cast<uint32_t>(vertexCount);*/
                vertices[vertexCount] = vertex;
                //}

                indices[vertexCount] = vertexCount; // uniqueVertices[vertex];

                vertexCount++;
            }

            if(attrib.normals.empty())
                Graphics::Mesh::GenerateNormals(vertices.Data(), vertexCount, indices.Data(), numIndices);

            // TODO : if(isAnimated) Load deferredColourAnimated;
            //  auto shader = Application::Get().GetShaderLibrary()->GetAsset("//CoreShaders/ForwardPBR.shader");
            auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBR")).As<Graphics::Shader>();

            SharedPtr<Material> pbrMaterial = CreateSharedPtr<Material>(shader);

            PBRMataterialTextures textures;

            if(shape.mesh.material_ids[0] >= 0)
            {
                tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

                if(mp->diffuse_texname.length() > 0)
                {
                    SharedPtr<Graphics::Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, Graphics::TextureDesc(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->diffuse_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.albedo = texture;
                }

                if(mp->bump_texname.length() > 0)
                {
                    SharedPtr<Graphics::Texture2D> texture = LoadMaterialTextures("Normal", m_Textures, mp->bump_texname, m_Directory, Graphics::TextureDesc(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->bump_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.normal = texture; // pbrMaterial->SetNormalMap(texture);
                }

                if(mp->roughness_texname.length() > 0)
                {
                    SharedPtr<Graphics::Texture2D> texture = LoadMaterialTextures("Roughness", m_Textures, mp->roughness_texname.c_str(), m_Directory, Graphics::TextureDesc(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->roughness_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.roughness = texture;
                }

                if(mp->metallic_texname.length() > 0)
                {
                    SharedPtr<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->metallic_texname, m_Directory, Graphics::TextureDesc(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->metallic_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.metallic = texture;
                }

                if(mp->specular_highlight_texname.length() > 0)
                {
                    SharedPtr<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->specular_highlight_texname, m_Directory, Graphics::TextureDesc(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->specular_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.metallic = texture;
                }
            }

            pbrMaterial->SetTextures(textures);

            auto mesh = CreateSharedPtr<Graphics::Mesh>(indices, vertices);
            mesh->SetMaterial(pbrMaterial);
            mesh->GenerateTangentsAndBitangents(vertices.Data(), uint32_t(numVertices), indices.Data(), uint32_t(numIndices));

            m_Meshes.PushBack(mesh);

            m_Textures.Clear();
        }
    }

}
