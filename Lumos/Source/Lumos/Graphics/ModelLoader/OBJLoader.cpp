#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/Transform.h"
#include "Graphics/RHI/Texture.h"
#include "Maths/Maths.h"

#include "Core/Application.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Lumos
{
    std::string m_Directory;
    std::vector<SharedRef<Graphics::Texture2D>> m_Textures;

    SharedRef<Graphics::Texture2D> LoadMaterialTextures(const std::string& typeName, std::vector<SharedRef<Graphics::Texture2D>>& textures_loaded, const std::string& name, const std::string& directory, Graphics::TextureParameters format)
    {
        for(uint32_t j = 0; j < textures_loaded.size(); j++)
        {
            if(std::strcmp(textures_loaded[j]->GetFilepath().c_str(), (directory + "/" + name).c_str()) == 0)
            {
                return textures_loaded[j];
            }
        }

        { // If texture hasn't been loaded already, load it
            Graphics::TextureLoadOptions options(false, true);
            auto texture = SharedRef<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(typeName, directory + "/" + name, format, options));
            textures_loaded.push_back(texture); // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.

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

        m_Directory = resolvedPath.substr(0, resolvedPath.find_last_of('/'));

        std::string name = m_Directory.substr(m_Directory.find_last_of('/') + 1);

        bool ok = tinyobj::LoadObj(
            &attrib, &shapes, &materials, &error, (resolvedPath).c_str(), (m_Directory + "/").c_str());

        if(!ok)
        {
            LUMOS_LOG_CRITICAL(error);
        }

        bool singleMesh = shapes.size() == 1;

        for(const auto& shape : shapes)
        {
            uint32_t vertexCount = 0;
            const uint32_t numIndices = static_cast<uint32_t>(shape.mesh.indices.size());
            const uint32_t numVertices = numIndices; // attrib.vertices.size();// numIndices / 3.0f;
            Graphics::Vertex* vertices = new Graphics::Vertex[numVertices];
            uint32_t* indices = new uint32_t[numIndices];

            std::unordered_map<Graphics::Vertex, uint32_t> uniqueVertices;

            SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();

            for(uint32_t i = 0; i < shape.mesh.indices.size(); i++)
            {
                auto& index = shape.mesh.indices[i];
                Graphics::Vertex vertex;

                if(!attrib.texcoords.empty())
                {
                    vertex.TexCoords = (Maths::Vector2(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]));
                }
                else
                {
                    vertex.TexCoords = Maths::Vector2(0.0f, 0.0f);
                }
                vertex.Position = (Maths::Vector3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]));

                boundingBox->Merge(vertex.Position);

                if(!attrib.normals.empty())
                {
                    vertex.Normal = (Maths::Vector3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]));
                }

                Maths::Vector4 colour = Maths::Vector4(0.0f);

                if(shape.mesh.material_ids[0] >= 0)
                {
                    tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];
                    colour = Maths::Vector4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f);
                }

                vertex.Colours = colour;

                if(uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertexCount);
                    vertices[vertexCount] = vertex;
                }

                indices[vertexCount] = uniqueVertices[vertex];

                vertexCount++;
            }

            if(attrib.normals.empty())
                Graphics::Mesh::GenerateNormals(vertices, vertexCount, indices, numIndices);

            Graphics::Mesh::GenerateTangents(vertices, vertexCount, indices, numIndices);

            //TODO : if(isAnimated) Load deferredColourAnimated;
            auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");

            SharedRef<Material> pbrMaterial = CreateSharedRef<Material>(shader);

            PBRMataterialTextures textures;

            if(shape.mesh.material_ids[0] >= 0)
            {
                tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

                if(mp->diffuse_texname.length() > 0)
                {
                    SharedRef<Graphics::Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->diffuse_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.albedo = texture;
                }

                if(mp->bump_texname.length() > 0)
                {
                    SharedRef<Graphics::Texture2D> texture = LoadMaterialTextures("Normal", m_Textures, mp->bump_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->bump_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.normal = texture; //pbrMaterial->SetNormalMap(texture);
                }

                if(mp->roughness_texname.length() > 0)
                {
                    SharedRef<Graphics::Texture2D> texture = LoadMaterialTextures("Roughness", m_Textures, mp->roughness_texname.c_str(), m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->roughness_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.roughness = texture;
                }

                if(mp->metallic_texname.length() > 0)
                {
                    SharedRef<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->metallic_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->metallic_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.metallic = texture;
                }

                if(mp->specular_highlight_texname.length() > 0)
                {
                    SharedRef<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->specular_highlight_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->specular_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
                    if(texture)
                        textures.metallic = texture;
                }
            }

            pbrMaterial->SetTextures(textures);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(sizeof(Graphics::Vertex) * numVertices, vertices);

            SharedRef<Graphics::IndexBuffer> ib;
            ib.reset(Graphics::IndexBuffer::Create(indices, numIndices));

            auto mesh = CreateSharedRef<Graphics::Mesh>(vb, ib, boundingBox);
            mesh->SetMaterial(pbrMaterial);
            m_Meshes.push_back(mesh);

            m_Textures.clear();

            delete[] vertices;
            delete[] indices;
        }
    }

}
