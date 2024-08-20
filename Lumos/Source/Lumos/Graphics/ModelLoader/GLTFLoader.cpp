#if defined(__GNUC__) && defined(_DEBUG) && defined(__OPTIMIZE__)
#warning "Undefing __OPTIMIZE__"
#undef __OPTIMIZE__
#endif

#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Animation/Animation.h"

#include "Graphics/RHI/Texture.h"
#include "Maths/MathsBasicTypes.h"
#include "Maths/MathsUtilities.h"

#include "Maths/Transform.h"
#include "Core/Application.h"
#include "Utilities/StringUtilities.h"
#include "Core/Asset/AssetManager.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Matrix3.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef LUMOS_PRODUCTION
#define TINYGLTF_NOEXCEPTION
#endif
#include <ModelLoaders/tinygltf/tiny_gltf.h>
#include <stb_image_resize2.h>

#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz-animation/src/animation/offline/gltf/gltf2ozz.cc>
#include <ozz/animation/runtime/local_to_model_job.h>

namespace Lumos::Graphics
{
    std::string AlbedoTexName   = "baseColorTexture";
    std::string NormalTexName   = "normalTexture";
    std::string MetallicTexName = "metallicRoughnessTexture";
    std::string GlossTexName    = "metallicRoughnessTexture";
    std::string AOTexName       = "occlusionTexture";
    std::string EmissiveTexName = "emissiveTexture";

    struct GLTFTexture
    {
        tinygltf::Image* Image;
        tinygltf::Sampler* Sampler;
    };

    static HashMap(int, int) GLTF_COMPONENT_LENGTH_LOOKUP;
    static HashMap(int, int) GLTF_COMPONENT_BYTE_SIZE_LOOKUP;
    static bool HashMapsInitialised = false;
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

    TDArray<SharedPtr<Material>> LoadMaterials(tinygltf::Model& gltfModel)
    {
        LUMOS_PROFILE_FUNCTION();
        TDArray<SharedPtr<Graphics::Texture2D>> loadedTextures;
        TDArray<SharedPtr<Material>> loadedMaterials;
        loadedTextures.Reserve(gltfModel.textures.size());
        loadedMaterials.Reserve(gltfModel.materials.size());
        bool animated = false;
        if(gltfModel.skins.size() >= 0)
        {
            animated = true;
        }

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
                Graphics::TextureDesc params;
                if(gltfTexture.sampler != -1)
                    params = Graphics::TextureDesc(GetFilter(imageAndSampler.Sampler->minFilter), GetFilter(imageAndSampler.Sampler->magFilter), GetWrapMode(imageAndSampler.Sampler->wrapS));
                else
                    LWARN("MISSING SAMPLER");

                uint32_t texWidth  = imageAndSampler.Image->width;
                uint32_t texHeight = imageAndSampler.Image->height;
                uint8_t* pixels    = imageAndSampler.Image->image.data();

                uint32_t maxWidth, maxHeight;
                GetMaxImageDimensions(maxWidth, maxHeight);
                bool freeData = false;

                if(maxWidth > 0 && maxHeight > 0 && (texWidth > maxWidth || texHeight > maxHeight))
                {
                    uint32_t texWidthOld  = imageAndSampler.Image->width;
                    uint32_t texHeightOld = imageAndSampler.Image->height;

                    float aspectRatio = static_cast<float>(texWidth) / static_cast<float>(texHeight);
                    if(texWidth > maxWidth)
                    {
                        texWidth  = maxWidth;
                        texHeight = static_cast<uint32_t>(maxWidth / aspectRatio);
                    }
                    if(texHeight > maxHeight)
                    {
                        texHeight = maxHeight;
                        texWidth  = static_cast<uint32_t>(maxHeight * aspectRatio);
                    }

                    // Resize the image using stbir (a simple image resizing library)
                    int resizedChannels    = 4; // RGBA format
                    stbi_uc* resizedPixels = (stbi_uc*)malloc(texWidth * texHeight * resizedChannels);
                    stbir_resize_uint8_linear(pixels, texWidthOld, texHeightOld, 0, resizedPixels, texWidth, texHeight, 0, STBIR_RGBA);

                    // free(pixels);  // Free the original image
                    pixels   = resizedPixels;
                    freeData = true;
                }

                Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromSource(texWidth, texHeight, pixels, params);
                loadedTextures.PushBack(SharedPtr<Graphics::Texture2D>(texture2D ? texture2D : nullptr));
                if(freeData)
                    free(pixels);

                imageAndSampler.Image->image.clear();
                imageAndSampler.Image->image.shrink_to_fit();
            }
        }

        auto TextureName = [&](int index)
        {
            if(index >= 0)
            {
                const tinygltf::Texture& tex = gltfModel.textures[index];
                if(tex.source >= 0 && tex.source < loadedTextures.Size())
                {
                    return loadedTextures[tex.source];
                }
            }
            return SharedPtr<Graphics::Texture2D>();
        };

        for(tinygltf::Material& mat : gltfModel.materials)
        {
            // TODO : if(isAnimated) Load deferredColourAnimated;
            // auto shader = Application::Get().GetShaderLibrary()->GetAsset("//CoreShaders/ForwardPBR.shader");
            auto shader = Application::Get().GetAssetManager()->GetAssetData(animated ? "ForwardPBRAnim" : "ForwardPBR").As<Graphics::Shader>();

            SharedPtr<Material> pbrMaterial = CreateSharedPtr<Material>(shader);
            PBRMataterialTextures textures;
            Graphics::MaterialProperties properties;

            const tinygltf::PbrMetallicRoughness& pbr = mat.pbrMetallicRoughness;
            textures.albedo                           = TextureName(pbr.baseColorTexture.index);
            textures.normal                           = TextureName(mat.normalTexture.index);
            textures.ao                               = TextureName(mat.occlusionTexture.index);
            textures.emissive                         = TextureName(mat.emissiveTexture.index);
            textures.metallic                         = TextureName(pbr.metallicRoughnessTexture.index);

            // TODO: correct way of handling this
            if(textures.metallic)
                properties.workflow = PBR_WORKFLOW_METALLIC_ROUGHNESS;
            else
                properties.workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;

            // metallic-roughness workflow:
            auto baseColourFactor = mat.values.find("baseColorFactor");
            auto roughnessFactor  = mat.values.find("roughnessFactor");
            auto metallicFactor   = mat.values.find("metallicFactor");

            if(roughnessFactor != mat.values.end())
            {
                properties.roughness = static_cast<float>(roughnessFactor->second.Factor());
            }

            if(metallicFactor != mat.values.end())
            {
                properties.metallic = static_cast<float>(metallicFactor->second.Factor());
            }

            if(baseColourFactor != mat.values.end())
            {
                properties.albedoColour = Vec4((float)baseColourFactor->second.ColorFactor()[0], (float)baseColourFactor->second.ColorFactor()[1], (float)baseColourFactor->second.ColorFactor()[2], 1.0f);
                if(baseColourFactor->second.ColorFactor().size() > 3)
                    properties.albedoColour.w = (float)baseColourFactor->second.ColorFactor()[3];
            }

            // Extensions
            auto metallicGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
            if(metallicGlossinessWorkflow != mat.extensions.end())
            {
                if(metallicGlossinessWorkflow->second.Has("diffuseTexture"))
                {
                    int index       = metallicGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
                    textures.albedo = loadedTextures[gltfModel.textures[index].source];
                }

                if(metallicGlossinessWorkflow->second.Has("metallicGlossinessTexture"))
                {
                    int index           = metallicGlossinessWorkflow->second.Get("metallicGlossinessTexture").Get("index").Get<int>();
                    textures.roughness  = loadedTextures[gltfModel.textures[index].source];
                    properties.workflow = PBR_WORKFLOW_SPECULAR_GLOSINESS;
                }

                if(metallicGlossinessWorkflow->second.Has("diffuseFactor"))
                {
                    auto& factor              = metallicGlossinessWorkflow->second.Get("diffuseFactor");
                    properties.albedoColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                    properties.albedoColour.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
                    properties.albedoColour.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
                    properties.albedoColour.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
                }
                if(metallicGlossinessWorkflow->second.Has("metallicFactor"))
                {
                    auto& factor        = metallicGlossinessWorkflow->second.Get("metallicFactor");
                    properties.metallic = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                }
                if(metallicGlossinessWorkflow->second.Has("glossinessFactor"))
                {
                    auto& factor         = metallicGlossinessWorkflow->second.Get("glossinessFactor");
                    properties.roughness = 1.0f - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
                }
            }

            auto ext_ior = mat.extensions.find("KHR_materials_ior");
            if(ext_ior != mat.extensions.end())
            {
                // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_ior

                if(ext_ior->second.Has("ior"))
                {
                    auto& factor = ext_ior->second.Get("ior");
                    float ior    = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());

                    properties.reflectance = Maths::Sqrt(Maths::Pow((ior - 1.0f) / (ior + 1.0f), 2.0f) / 16.0f);
                }
            }

            pbrMaterial->SetTextures(textures);
            pbrMaterial->SetMaterialProperites(properties);
            pbrMaterial->SetName(mat.name);

            if(mat.doubleSided)
                pbrMaterial->SetFlag(Graphics::Material::RenderFlags::TWOSIDED);

            if(mat.alphaMode != "OPAQUE")
                pbrMaterial->SetFlag(Graphics::Material::RenderFlags::ALPHABLEND);

            loadedMaterials.PushBack(pbrMaterial);
        }

        return loadedMaterials;
    }

    TDArray<Graphics::Mesh*> LoadMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, TDArray<SharedPtr<Material>>& materials, Maths::Transform& parentTransform)
    {
        TDArray<Graphics::Mesh*> meshes;

        for(auto& primitive : mesh.primitives)
        {
            TDArray<Graphics::Vertex> vertices;
            TDArray<Graphics::AnimVertex> animVertices;

            uint32_t vertexCount = (uint32_t)(primitive.attributes.empty() ? 0 : model.accessors.at(primitive.attributes["POSITION"]).count);

            bool hasNormals    = false;
            bool hasTangents   = false;
            bool hasBitangents = false;
            bool hasWeights    = false;
            bool hasJoints     = false;

            if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
                hasNormals = true;
            if(primitive.attributes.find("TANGENT") != primitive.attributes.end())
                hasTangents = true;
            if(primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                hasJoints = true;
            if(primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
                hasWeights = true;
            if(primitive.attributes.find("BITANGENT") != primitive.attributes.end())
                hasBitangents = true;

            if(hasJoints || hasWeights)
                animVertices.Resize(vertexCount);

            vertices.Resize(vertexCount);

#define VERTEX(i, member) (hasWeights ? vertices[i].member : animVertices[i].member)

            for(auto& attribute : primitive.attributes)
            {
                // Get accessor info
                auto& accessor            = model.accessors.at(attribute.second);
                auto& bufferView          = model.bufferViews.at(accessor.bufferView);
                auto& buffer              = model.buffers.at(bufferView.buffer);
                //int componentLength       = GLTF_COMPONENT_LENGTH_LOOKUP.at(accessor.type);
                //int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(accessor.componentType);

                int componentLength = 0;// GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
                int componentTypeByteSize = 0;// GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);

                HashMapFind(&GLTF_COMPONENT_LENGTH_LOOKUP, accessor.type, &componentLength);
                HashMapFind(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, accessor.componentType, &componentTypeByteSize);

                int stride = accessor.ByteStride(bufferView);

                // Extra vertex data from buffer
                size_t bufferOffset = bufferView.byteOffset + accessor.byteOffset;
                int bufferLength    = static_cast<int>(accessor.count) * componentLength * componentTypeByteSize;
                // auto first                = buffer.data.begin() + bufferOffset;
                // auto last                 = buffer.data.begin() + bufferOffset + bufferLength;
                // TDArray<uint8_t> data = TDArray<uint8_t>(first, last);
                TDArray<uint8_t> data = TDArray<uint8_t>();
                data.Resize(bufferLength);
                uint8_t*& arrayData = data.Data();
                MemoryCopy(arrayData, buffer.data.data() + bufferOffset, bufferLength);

                // -------- Position attribute -----------

                if(attribute.first == "POSITION")
                {
                    size_t positionCount            = accessor.count;
                    Maths::Vector3Simple* positions = reinterpret_cast<Maths::Vector3Simple*>(data.Data());
                    for(auto p = 0; p < positionCount; ++p)
                    {
                        vertices[p].Position = parentTransform.GetWorldMatrix() * Maths::ToVector4(positions[p]);
                        ASSERT(!Maths::IsInf(vertices[p].Position.x) && !Maths::IsInf(vertices[p].Position.y) && !Maths::IsInf(vertices[p].Position.z) && !Maths::IsNaN(vertices[p].Position.x) && !Maths::IsNaN(vertices[p].Position.y) && !Maths::IsNaN(vertices[p].Position.z));
                    }
                }

                // -------- Normal attribute -----------

                else if(attribute.first == "NORMAL")
                {
                    size_t normalCount            = accessor.count;
                    Maths::Vector3Simple* normals = reinterpret_cast<Maths::Vector3Simple*>(data.Data());
                    for(auto p = 0; p < normalCount; ++p)
                    {
                        // vertices[p].Normal = (parentTransform.GetWorldMatrix() * Maths::ToVector4(normals[p]));
                        vertices[p].Normal = Maths::Transpose(Mat3::Inverse(Mat3(parentTransform.GetWorldMatrix()))) * (Vec3(Maths::ToVector4(normals[p])));
                        vertices[p].Normal = vertices[p].Normal.Normalised();
                        ASSERT(!Maths::IsInf(vertices[p].Normal.x) && !Maths::IsInf(vertices[p].Normal.y) && !Maths::IsInf(vertices[p].Normal.z) && !Maths::IsNaN(vertices[p].Normal.x) && !Maths::IsNaN(vertices[p].Normal.y) && !Maths::IsNaN(vertices[p].Normal.z));
                    }
                }

                // -------- Texcoord attribute -----------

                else if(attribute.first == "TEXCOORD_0")
                {
                    size_t uvCount            = accessor.count;
                    Maths::Vector2Simple* uvs = reinterpret_cast<Maths::Vector2Simple*>(data.Data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].TexCoords = ToVector(uvs[p]);
                    }
                }

                // -------- Colour attribute -----------

                else if(attribute.first == "COLOR_0")
                {
                    size_t uvCount                = accessor.count;
                    Maths::Vector4Simple* colours = reinterpret_cast<Maths::Vector4Simple*>(data.Data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].Colours = ToVector(colours[p]);
                    }
                }

                // -------- Tangent attribute -----------

                else if(attribute.first == "TANGENT")
                {
                    hasTangents               = true;
                    size_t uvCount            = accessor.count;
                    Maths::Vector3Simple* uvs = reinterpret_cast<Maths::Vector3Simple*>(data.Data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].Tangent = Maths::Transpose(Mat3::Inverse(Mat3(parentTransform.GetWorldMatrix()))) * (Vec3(Maths::ToVector4(uvs[p])));
                        vertices[p].Tangent = vertices[p].Tangent.Normalised();
                        ASSERT(!Maths::IsInf(vertices[p].Tangent.x) && !Maths::IsInf(vertices[p].Tangent.y) && !Maths::IsInf(vertices[p].Tangent.z) && !Maths::IsNaN(vertices[p].Tangent.x) && !Maths::IsNaN(vertices[p].Tangent.y) && !Maths::IsNaN(vertices[p].Tangent.z));
                    }
                }

                else if(attribute.first == "BINORMAL")
                {
                    hasBitangents             = true;
                    size_t uvCount            = accessor.count;
                    Maths::Vector3Simple* uvs = reinterpret_cast<Maths::Vector3Simple*>(data.Data());
                    for(auto p = 0; p < uvCount; ++p)
                    {
                        vertices[p].Bitangent = Maths::Transpose(Mat3::Inverse(Mat3(parentTransform.GetWorldMatrix()))) * (Vec3(Maths::ToVector4(uvs[p])));
                        vertices[p].Bitangent = vertices[p].Bitangent.Normalised();
                        ASSERT(!Maths::IsInf(vertices[p].Bitangent.x) && !Maths::IsInf(vertices[p].Bitangent.y) && !Maths::IsInf(vertices[p].Bitangent.z) && !Maths::IsNaN(vertices[p].Bitangent.x) && !Maths::IsNaN(vertices[p].Bitangent.y) && !Maths::IsNaN(vertices[p].Bitangent.z));
                    }
                }

                // -------- Weights attribute -----------

                else if(attribute.first == "WEIGHTS_0")
                {
                    hasWeights = true;

                    size_t weightCount = accessor.count;

                    if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                    {
                        Maths::Vector4Simple* weights = reinterpret_cast<Maths::Vector4Simple*>(data.Data());
                        for(auto p = 0; p < weightCount; ++p)
                        {
                            animVertices[p].Weights[0] = weights[p].x;
                            animVertices[p].Weights[1] = weights[p].y;
                            animVertices[p].Weights[2] = weights[p].z;
                            animVertices[p].Weights[3] = weights[p].w;
                        }
                    }
                    else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    {
                        for(auto i = 0; i < weightCount; ++i)
                        {
                            const uint8_t& x = *(uint8_t*)((size_t)data.Data() + i * stride + 0 * sizeof(uint16_t));
                            const uint8_t& y = *(uint8_t*)((size_t)data.Data() + i * stride + 1 * sizeof(uint16_t));
                            const uint8_t& z = *(uint8_t*)((size_t)data.Data() + i * stride + 2 * sizeof(uint16_t));
                            const uint8_t& w = *(uint8_t*)((size_t)data.Data() + i * stride + 3 * sizeof(uint16_t));

                            animVertices[i].Weights[0] = (float)x / 65535.0f;
                            animVertices[i].Weights[1] = (float)y / 65535.0f;
                            animVertices[i].Weights[2] = (float)z / 65535.0f;
                            animVertices[i].Weights[3] = (float)w / 65535.0f;
                        }
                    }
                    else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        for(auto i = 0; i < weightCount; ++i)
                        {
                            const uint8_t& x = *(uint8_t*)((size_t)data.Data() + i * stride + 0);
                            const uint8_t& y = *(uint8_t*)((size_t)data.Data() + i * stride + 1);
                            const uint8_t& z = *(uint8_t*)((size_t)data.Data() + i * stride + 2);
                            const uint8_t& w = *(uint8_t*)((size_t)data.Data() + i * stride + 3);

                            animVertices[i].Weights[0] = (float)x / 255.0f;
                            animVertices[i].Weights[1] = (float)y / 255.0f;
                            animVertices[i].Weights[2] = (float)z / 255.0f;
                            animVertices[i].Weights[3] = (float)w / 255.0f;
                        }
                    }
                    else
                    {
                        LWARN("Unsupported weight data type");
                    }
                }

                // -------- Joints attribute -----------

                else if(attribute.first == "JOINTS_0")
                {
                    hasJoints = true;

                    size_t jointCount = accessor.count;
                    if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    {
                        struct JointTmp
                        {
                            uint16_t ind[4];
                        };

                        uint16_t* joints = reinterpret_cast<uint16_t*>(data.Data());
                        for(auto p = 0; p < jointCount; ++p)
                        {
                            const JointTmp& joint = *(const JointTmp*)(data.Data() + p * stride);

                            animVertices[p].BoneInfoIndices[0] = (uint32_t)joint.ind[0];
                            animVertices[p].BoneInfoIndices[1] = (uint32_t)joint.ind[1];
                            animVertices[p].BoneInfoIndices[2] = (uint32_t)joint.ind[2];
                            animVertices[p].BoneInfoIndices[3] = (uint32_t)joint.ind[3];
                        }
                    }
                    else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        uint8_t* joints = reinterpret_cast<uint8_t*>(data.Data());
                        for(auto p = 0; p < jointCount; ++p)
                        {
                            animVertices[p].BoneInfoIndices[0] = (uint32_t)joints[p * 4];
                            animVertices[p].BoneInfoIndices[1] = (uint32_t)joints[p * 4 + 1];
                            animVertices[p].BoneInfoIndices[2] = (uint32_t)joints[p * 4 + 2];
                            animVertices[p].BoneInfoIndices[3] = (uint32_t)joints[p * 4 + 3];
                        }
                    }
                    else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    {
                        struct JointTmp
                        {
                            uint32_t ind[4];
                        };

                        for(size_t i = 0; i < jointCount; ++i)
                        {
                            const JointTmp& joint = *(const JointTmp*)(data.Data() + i * stride);

                            animVertices[i].BoneInfoIndices[0] = joint.ind[0];
                            animVertices[i].BoneInfoIndices[1] = joint.ind[1];
                            animVertices[i].BoneInfoIndices[2] = joint.ind[2];
                            animVertices[i].BoneInfoIndices[3] = joint.ind[3];
                        }
                    }
                    else
                    {
                        LWARN("Unsupported joint data type");
                    }
                }
            }

            // -------- Indices ----------
            TDArray<uint32_t> indices;
            if(primitive.indices >= 0)
            {
                const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
                indices.Resize(indicesAccessor.count);
                {
                    // Get accessor info
                    auto indexAccessor   = model.accessors.at(primitive.indices);
                    auto indexBufferView = model.bufferViews.at(indexAccessor.bufferView);
                    auto indexBuffer     = model.buffers.at(indexBufferView.buffer);

                    int componentLength = 0;// GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
                    int componentTypeByteSize = 0;// GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);

                    HashMapFind(&GLTF_COMPONENT_LENGTH_LOOKUP, indexAccessor.type, &componentLength);
                    HashMapFind(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, indexAccessor.componentType, &componentTypeByteSize);

                    // Extra index data
                    size_t bufferOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
                    int bufferLength    = static_cast<int>(indexAccessor.count) * componentLength * componentTypeByteSize;
                    /*       auto first                = indexBuffer.data.begin() + bufferOffset;
                           auto last                 = indexBuffer.data.begin() + bufferOffset + bufferLength;
                           TDArray<uint8_t> data = TDArray<uint8_t>(first, last);*/
                    TDArray<uint8_t> data = TDArray<uint8_t>();
                    data.Resize(bufferLength);
                    uint8_t*& arrayData = data.Data();
                    MemoryCopy(arrayData, indexBuffer.data.data() + bufferOffset, bufferLength);

                    size_t indicesCount = indexAccessor.count;
                    if(componentTypeByteSize == 1)
                    {
                        uint8_t* in = reinterpret_cast<uint8_t*>(data.Data());
                        for(auto iCount = 0; iCount < indicesCount; iCount++)
                        {
                            indices[iCount] = (uint32_t)in[iCount];
                        }
                    }
                    else if(componentTypeByteSize == 2)
                    {
                        uint16_t* in = reinterpret_cast<uint16_t*>(data.Data());
                        for(auto iCount = 0; iCount < indicesCount; iCount++)
                        {
                            indices[iCount] = (uint32_t)in[iCount];
                        }
                    }
                    else if(componentTypeByteSize == 4)
                    {
                        auto in = reinterpret_cast<uint32_t*>(data.Data());
                        for(auto iCount = 0; iCount < indicesCount; iCount++)
                        {
                            indices[iCount] = in[iCount];
                        }
                    }
                    else
                    {
                        LWARN("Unsupported indices data type - %i", componentTypeByteSize);
                    }
                }
            }
            else
            {
                LWARN("Missing Indices - Generating new");

                const auto& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
                indices.Reserve(accessor.count);
                //                for (auto i = 0; i < accessor.count; i++)
                //                       indices.PushBack(i);

                for(size_t vi = 0; vi < accessor.count; vi += 3)
                {
                    indices.PushBack(uint32_t(vi + 0));
                    indices.PushBack(uint32_t(vi + 1));
                    indices.PushBack(uint32_t(vi + 2));
                }
            }

            if(!hasNormals)
                Graphics::Mesh::GenerateNormals(vertices.Data(), uint32_t(vertices.Size()), indices.Data(), uint32_t(indices.Size()));
            if(!hasTangents || !hasBitangents)
                Graphics::Mesh::GenerateTangentsAndBitangents(vertices.Data(), uint32_t(vertices.Size()), indices.Data(), uint32_t(indices.Size()));

            // Add mesh
            Graphics::Mesh* lMesh;

            if(hasJoints || hasWeights)
            {
                for(size_t i = 0; i < vertices.Size(); i++)
                {
                    animVertices[i].NormalizeWeights();
                    animVertices[i].Position  = vertices[i].Position;
                    animVertices[i].Normal    = vertices[i].Normal;
                    animVertices[i].Colours   = vertices[i].Colours;
                    animVertices[i].Tangent   = vertices[i].Tangent;
                    animVertices[i].Bitangent = vertices[i].Bitangent;
                    animVertices[i].TexCoords = vertices[i].TexCoords;
                }
                lMesh = new Graphics::Mesh(indices, animVertices);
            }
            else
                lMesh = new Graphics::Mesh(indices, vertices);


            //Moved from mesh
            //Move this lbmesh
            /*
            // int lod = 2;
            // float threshold = powf(0.7f, float(lod));
            
            if(optimise)
            {
                size_t indexCount         = indices.Size();
                size_t vertexCount        = vertices.Size();

                size_t target_index_count = size_t(indices.Size() * optimiseThreshold);

                float target_error = 1e-3f;
                float* resultError = nullptr;

                auto newIndexCount = meshopt_simplify(indices.Data(), indices.Data(), indices.Size(), (const float*)(&vertices[0]), vertices.Size(), sizeof(Graphics::Vertex), target_index_count, target_error, resultError);

                auto newVertexCount = meshopt_optimizeVertexFetch( // return vertices (not vertex attribute values)
                    (vertices.Data()),
                    (unsigned int*)(indices.Data()),
                    newIndexCount, // total new indices (not faces)
                    (vertices.Data()),
                    (size_t)vertices.Size(), // total vertices (not vertex attribute values)
                    sizeof(Graphics::Vertex)   // vertex stride
                );
            }

            */
            meshes.EmplaceBack(lMesh);
        }

        return meshes;
    }

    void LoadNode(Model* mainModel, int nodeIndex, const Mat4& parentTransform, tinygltf::Model& model, TDArray<SharedPtr<Material>>& materials, TDArray<TDArray<Graphics::Mesh*>>& meshes)
    {
        LUMOS_PROFILE_FUNCTION();
        if(nodeIndex < 0)
        {
            return;
        }

        auto& node = model.nodes[nodeIndex];
        auto name  = node.name;

#ifdef DEBUG_PRINT_GLTF_LOADING
        LINFO("asset.copyright : %s", model.asset.copyright);
        LINFO("asset.generator : %s", model.asset.generator);
        LINFO("asset.version : %s", model.asset.version);
        LINFO("asset.minVersion : %s", model.asset.minVersion);
#endif

        Maths::Transform transform;
        Mat4 matrix;
        Mat4 position = Mat4(1.0f);
        Mat4 rotation = Mat4(1.0f);
        Mat4 scale    = Mat4(1.0f);

        if(!node.scale.empty())
        {
            scale = Mat4::Scale(Vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2])));
            // transform.SetLocalScale(Vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2])));
        }

        if(!node.rotation.empty())
        {
            rotation = Maths::ToMat4(Quat(static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]), static_cast<float>(node.rotation[3])));

            // transform.SetLocalOrientation(Quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2])));
        }

        if(!node.translation.empty())
        {
            // transform.SetLocalPosition(Vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2])));
            position = Mat4::Translation(Vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2])));
        }

        if(!node.matrix.empty())
        {
            float matrixData[16];
            for(int i = 0; i < 16; i++)
                matrixData[i] = float(node.matrix.data()[i]);
            matrix = Mat4(matrixData);
            transform.SetLocalTransform(matrix);
        }
        else
        {
            matrix = position * rotation * scale;
            transform.SetLocalTransform(matrix);
        }

        transform.SetWorldMatrix(parentTransform);

        if(node.mesh >= 0)
        {
            int subIndex = 0;

            auto meshes = LoadMesh(model, model.meshes[node.mesh], materials, transform);

            for(auto& mesh : meshes)
            {
                auto subname = model.meshes[node.mesh].name;
                auto lMesh   = SharedPtr<Graphics::Mesh>(mesh);
                lMesh->SetName(subname);

                int materialIndex = model.meshes[node.mesh].primitives[subIndex].material;
                if(materialIndex >= 0)
                    lMesh->SetMaterial(materials[materialIndex]);

                mainModel->AddMesh(lMesh);

                // if (node.skin >= 0)
                // {
                // }

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

    Mat4 ConvertToGLM2(const ozz::math::Float4x4& ozzMat)
    {
        Mat4 glmMat;

        // Assuming ozz::math::Float4x4 is column-major
        float matrix[16];
        for(int r = 0; r < 4; r++)
        {
            float result[4];
            MemoryCopy(result, ozzMat.cols + r, sizeof(ozzMat.cols[r]));
            float dresult[4];
            for(int j = 0; j < 4; j++)
            {
                dresult[j] = result[j];
            }
            //_mm_store_ps(result,p.cols[r]);
            MemoryCopy(matrix + r * 4, dresult, sizeof(dresult));
        }

        glmMat = Mat4(matrix);
        return glmMat;
    }

    void Model::LoadGLTF(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        if (!HashMapsInitialised)
        {
            HashMapInit(&GLTF_COMPONENT_LENGTH_LOOKUP);
            HashMapInit(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP);

            int key = (int)TINYGLTF_TYPE_SCALAR;
            int value = 1;
            {
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);
                key = (int)TINYGLTF_TYPE_VEC2;
                value = 2;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);

                key = (int)TINYGLTF_TYPE_VEC3;
                value = 3;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);

                key = (int)TINYGLTF_TYPE_VEC4;
                value = 4;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);

                key = (int)TINYGLTF_TYPE_MAT2;
                value = 4;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);

                key = (int)TINYGLTF_TYPE_MAT3;
                value = 9;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);

                key = (int)TINYGLTF_TYPE_MAT4;
                value = 16;
                HashMapInsert(&GLTF_COMPONENT_LENGTH_LOOKUP, key, value);
            }

            {
                key = (int)TINYGLTF_COMPONENT_TYPE_BYTE;
                value = 1;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);

                key = (int)TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
                value = 1;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);

                key = (int)TINYGLTF_COMPONENT_TYPE_SHORT;
                value = 2;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);

                key = (int)TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                value = 2;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);

                key = (int)TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
                value = 4;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);

                key = (int)TINYGLTF_COMPONENT_TYPE_FLOAT;
                value = 4;
                HashMapInsert(&GLTF_COMPONENT_BYTE_SIZE_LOOKUP, key, value);
            }

            HashMapsInitialised = true;
        }

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        std::string ext = StringUtilities::GetFilePathExtension(path);

        // loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
        // loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

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
            LERROR(err.c_str());
        }

        if(!warn.empty())
        {
            LERROR(warn.c_str());
        }

        if(!ret || model.defaultScene < 0 || model.scenes.empty())
        {
            LERROR("Failed to parse glTF");
        }
        {
            LUMOS_PROFILE_SCOPE("Parse GLTF Model");

            auto LoadedMaterials = LoadMaterials(model);

            std::string name = path.substr(path.find_last_of('/') + 1);

            auto meshes                      = TDArray<TDArray<Graphics::Mesh*>>();
            const tinygltf::Scene& gltfScene = model.scenes[Lumos::Maths::Max(0, model.defaultScene)];
            for(size_t i = 0; i < gltfScene.nodes.size(); i++)
            {
                LoadNode(this, gltfScene.nodes[i], Mat4(1.0f), model, LoadedMaterials, meshes);
            }

            auto skins = model.skins;
            if(!skins.empty() || !model.animations.empty())
            {
                using namespace ozz::animation::offline;
                GltfImporter impl;
                ozz::animation::offline::OzzImporter& importer = impl;
                OzzImporter::NodeType types                    = {};

                importer.Load(path.c_str());
                RawSkeleton* rawSkeleton = new RawSkeleton();
                importer.Import(rawSkeleton, types);

                ozz::animation::offline::SkeletonBuilder skeletonBuilder;

                m_Skeleton = CreateSharedPtr<Skeleton>(skeletonBuilder(*rawSkeleton).release());
                if(m_Skeleton->Valid())
                {
                    ozz::vector<ozz::math::Float4x4> bindposes_ozz;
                    m_BindPoses.Reserve(m_Skeleton->GetSkeleton().joint_names().size());
                    bindposes_ozz.resize(m_Skeleton->GetSkeleton().joint_names().size());

                    // convert from local space to model space
                    ozz::animation::LocalToModelJob job;
                    job.skeleton = &m_Skeleton->GetSkeleton();
                    job.input    = ozz::span(m_Skeleton->GetSkeleton().joint_rest_poses());
                    job.output   = ozz::make_span(bindposes_ozz); //, m_Skeleton->GetSkeleton().joint_names().size_bytes());

                    if(!job.Run())
                    {
                        LERROR("Failed to run ozz LocalToModelJob");
                    }

                    for(auto& pose : bindposes_ozz)
                    {
                        m_BindPoses.PushBack(Mat4::Inverse(ConvertToGLM2(pose)));
                    }

                    ozz::animation::offline::AnimationBuilder animBuilder;
                    auto animationNames = importer.GetAnimationNames();

                    for(auto& animName : animationNames)
                    {
                        RawAnimation* rawAnimation = new RawAnimation();
                        importer.Import(animName.c_str(), m_Skeleton->GetSkeleton(), 30.0f, rawAnimation);

                        m_Animation.PushBack(CreateSharedPtr<Animation>(std::string(animName.c_str()), animBuilder(*rawAnimation).release(), m_Skeleton));

                        delete rawAnimation;
                    }
                }

                delete rawSkeleton;
            }
        }
    }
}
