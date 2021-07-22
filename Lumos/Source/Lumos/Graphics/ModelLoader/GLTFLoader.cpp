#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"

#include "Graphics/RHI/Texture.h"
#include "Maths/Maths.h"

#include "Maths/Transform.h"
#include "Core/Application.h"
#include "Core/StringUtilities.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef LUMOS_DIST
#define TINYGLTF_NOEXCEPTION
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

    static std::map<int32_t, size_t> ComponentSize {
        { TINYGLTF_COMPONENT_TYPE_BYTE, sizeof(int8_t) },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, sizeof(uint8_t) },
        { TINYGLTF_COMPONENT_TYPE_SHORT, sizeof(int16_t) },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, sizeof(uint16_t) },
        { TINYGLTF_COMPONENT_TYPE_INT, sizeof(int32_t) },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, sizeof(uint32_t) },
        { TINYGLTF_COMPONENT_TYPE_FLOAT, sizeof(float) },
        { TINYGLTF_COMPONENT_TYPE_DOUBLE, sizeof(double) }
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
        { TINYGLTF_COMPONENT_TYPE_BYTE, 1 },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1 },
        { TINYGLTF_COMPONENT_TYPE_SHORT, 2 },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2 },
        { TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, 4 },
        { TINYGLTF_COMPONENT_TYPE_FLOAT, 4 }
    };

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

    std::vector<SharedRef<Material>> LoadMaterials(tinygltf::Model& gltfModel)
    {
        LUMOS_PROFILE_FUNCTION();
        std::vector<SharedRef<Graphics::Texture2D>> loadedTextures;
        std::vector<SharedRef<Material>> loadedMaterials;
        loadedTextures.reserve(gltfModel.textures.size());
        loadedMaterials.reserve(gltfModel.materials.size());

        for(tinygltf::Texture& gltfTexture : gltfModel.textures)
        {
            GLTFTexture imageAndSampler {};

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
                loadedTextures.push_back(SharedRef<Graphics::Texture2D>(texture2D ? texture2D : nullptr));
            }
        }

        auto TextureName = [&](int index)
        {
            if(index >= 0)
            {
                const tinygltf::Texture& tex = gltfModel.textures[index];
                if(tex.source >= 0 && tex.source < loadedTextures.size())
                {
                    return loadedTextures[tex.source];
                }
            }
            return SharedRef<Graphics::Texture2D>();
        };

        for(tinygltf::Material& mat : gltfModel.materials)
        {
            //TODO : if(isAnimated) Load deferredColourAnimated;
            auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");

            SharedRef<Material> pbrMaterial = CreateSharedRef<Material>(shader);
            PBRMataterialTextures textures;
            Graphics::MaterialProperties properties;

            const tinygltf::PbrMetallicRoughness& pbr = mat.pbrMetallicRoughness;
            textures.albedo = TextureName(pbr.baseColorTexture.index);
            textures.normal = TextureName(mat.normalTexture.index);
            textures.ao = TextureName(mat.occlusionTexture.index);
            textures.emissive = TextureName(mat.emissiveTexture.index);
            textures.metallic = TextureName(pbr.metallicRoughnessTexture.index);

            if(textures.metallic)
                properties.workflow = PBR_WORKFLOW_METALLIC_ROUGHNESS;
            else
                properties.workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;

            // metallic-roughness workflow:
            auto baseColourFactor = mat.values.find("baseColorFactor");
            auto roughnessFactor = mat.values.find("roughnessFactor");
            auto metallicFactor = mat.values.find("metallicFactor");

            if(roughnessFactor != mat.values.end())
            {
                properties.roughnessColour = Maths::Vector4(static_cast<float>(roughnessFactor->second.Factor()));
            }

            if(metallicFactor != mat.values.end())
            {
                properties.metallicColour = Maths::Vector4(static_cast<float>(metallicFactor->second.Factor()));
            }

            if(baseColourFactor != mat.values.end())
            {
                properties.albedoColour = Maths::Vector4((float)baseColourFactor->second.ColorFactor()[0], (float)baseColourFactor->second.ColorFactor()[1], (float)baseColourFactor->second.ColorFactor()[2], 1.0f);
            }

            // Extensions
            auto metallicGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
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
                    properties.workflow = PBR_WORKFLOW_SPECULAR_GLOSINESS;
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

            if(mat.doubleSided)
                pbrMaterial->SetFlag(Graphics::Material::RenderFlags::TWOSIDED);

            loadedMaterials.push_back(pbrMaterial);
        }

        return loadedMaterials;
    }

    std::vector<Graphics::Mesh*> LoadMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, std::vector<SharedRef<Material>>& materials, Maths::Transform& parentTransform)
    {
        std::vector<Graphics::Mesh*> meshes;

        for(auto& primitive : mesh.primitives)
        {
            const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];

            std::vector<uint32_t> indices;
            std::vector<Graphics::Vertex> vertices;

            indices.resize(indicesAccessor.count);
            vertices.resize(indicesAccessor.count);

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
                std::vector<uint8_t> data = std::vector<uint8_t>(first, last);

                // -------- Position attribute -----------

                if(attribute.first == "POSITION")
                {
                    size_t positionCount = accessor.count;
                    Maths::Vector3Simple* positions = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for(auto p = 0; p < positionCount; ++p)
                    {
                        vertices[p].Position = parentTransform.GetWorldMatrix() * Maths::ToVector(positions[p]);
                    }
                }

                // -------- Normal attribute -----------

                else if(attribute.first == "NORMAL")
                {
                    size_t normalCount = accessor.count;
                    Maths::Vector3Simple* normals = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for(auto p = 0; p < normalCount; ++p)
                    {
                        vertices[p].Normal = (parentTransform.GetWorldMatrix().ToMatrix3().Inverse().Transpose() * Maths::ToVector(normals[p])).Normalised();
                    }
                }

                // -------- Texcoord attribute -----------

                else if(attribute.first == "TEXCOORD_0")
                {
                    size_t uvCount = accessor.count;
                    Maths::Vector2Simple* uvs = reinterpret_cast<Maths::Vector2Simple*>(data.data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].TexCoords = ToVector(uvs[p]);
                    }
                }

                // -------- Colour attribute -----------

                else if(attribute.first == "COLOR_0")
                {
                    size_t uvCount = accessor.count;
                    Maths::Vector4Simple* colours = reinterpret_cast<Maths::Vector4Simple*>(data.data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].Colours = ToVector(colours[p]);
                    }
                }

                // -------- Tangent attribute -----------

                else if(attribute.first == "TANGENT")
                {
                    size_t uvCount = accessor.count;
                    Maths::Vector3Simple* uvs = reinterpret_cast<Maths::Vector3Simple*>(data.data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].Tangent = parentTransform.GetWorldMatrix() * ToVector(uvs[p]);
                    }
                }
            }

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
                std::vector<uint8_t> data = std::vector<uint8_t>(first, last);

                size_t indicesCount = indexAccessor.count;
                if(componentTypeByteSize == 2)
                {
                    uint16_t* in = reinterpret_cast<uint16_t*>(data.data());
                    for(auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indices[iCount] = (uint32_t)in[iCount];
                    }
                }
                else if(componentTypeByteSize == 4)
                {
                    auto in = reinterpret_cast<uint32_t*>(data.data());
                    for(auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indices[iCount] = in[iCount];
                    }
                }
            }

            auto lMesh = new Graphics::Mesh(indices, vertices);
            meshes.emplace_back(lMesh);
        }

        return meshes;
    }

    void LoadNode(Model* mainModel, int nodeIndex, const Maths::Matrix4& parentTransform, tinygltf::Model& model, std::vector<SharedRef<Material>>& materials, std::vector<std::vector<Graphics::Mesh*>>& meshes)
    {
        LUMOS_PROFILE_FUNCTION();
        if(nodeIndex < 0)
        {
            return;
        }

        auto& node = model.nodes[nodeIndex];
        auto name = node.name;

#ifdef DEBUG_PRINT_GLTF_LOADING
        LUMOS_LOG_INFO("asset.copyright : {0}", model.asset.copyright);
        LUMOS_LOG_INFO("asset.generator : {0}", model.asset.generator);
        LUMOS_LOG_INFO("asset.version : {0}", model.asset.version);
        LUMOS_LOG_INFO("asset.minVersion : {0}", model.asset.minVersion);
#endif

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
                auto lMesh = SharedRef<Graphics::Mesh>(mesh);
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
        LUMOS_PROFILE_FUNCTION();
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        std::string ext = StringUtilities::GetFilePathExtension(path);

        //loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
        //loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

        bool ret;

        if(ext == "glb") // assume binary glTF.
        {
            LUMOS_PROFILE_SCOPE(".glb binary loading");
            ret = tinygltf::TinyGLTF().LoadBinaryFromFile(&model, &err, &warn, path);
        }
        else // assume ascii glTF.
        {
            LUMOS_PROFILE_SCOPE(".gltf loading");
            ret = tinygltf::TinyGLTF().LoadASCIIFromFile(&model, &err, &warn, path);
        }

        if(!err.empty())
        {
            LUMOS_LOG_ERROR(err);
        }

        if(!warn.empty())
        {
            LUMOS_LOG_ERROR(warn);
        }

        if(!ret)
        {
            LUMOS_LOG_ERROR("Failed to parse glTF");
        }
        {
            LUMOS_PROFILE_SCOPE("Parse GLTF Model");

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
}
