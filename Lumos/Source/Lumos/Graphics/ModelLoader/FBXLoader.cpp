#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Core/OS/FileSystem.h"

#include "Graphics/RHI/Texture.h"
#include "Maths/Maths.h"

#include "Maths/Transform.h"
#include "Core/Application.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/AssetManager.h"

#include <ModelLoaders/OpenFBX/ofbx.h>

// #define THREAD_MESH_LOADING //Can't use with opengl
#ifdef THREAD_MESH_LOADING
#include "Core/JobSystem.h"
#endif

const uint32_t MAX_PATH_LENGTH = 260;

namespace Lumos::Graphics
{
    std::string m_FBXModelDirectory;

    enum class Orientation
    {
        Y_UP,
        Z_UP,
        Z_MINUS_UP,
        X_MINUS_UP,
        X_UP
    };

    Orientation orientation = Orientation::Y_UP;
    float fbx_scale         = 1.f;

    static ofbx::Vec3 operator-(const ofbx::Vec3& a, const ofbx::Vec3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    static ofbx::Vec2 operator-(const ofbx::Vec2& a, const ofbx::Vec2& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    glm::vec3 FixOrientation(const glm::vec3& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return glm::vec3(v.x, v.y, v.z);
        case Orientation::Z_UP:
            return glm::vec3(v.x, v.z, -v.y);
        case Orientation::Z_MINUS_UP:
            return glm::vec3(v.x, -v.z, v.y);
        case Orientation::X_MINUS_UP:
            return glm::vec3(v.y, -v.x, v.z);
        case Orientation::X_UP:
            return glm::vec3(-v.y, v.x, v.z);
        }
        return glm::vec3(v.x, v.y, v.z);
    }

    glm::quat FixOrientation(const glm::quat& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return glm::quat(v.x, v.y, v.z, v.w);
        case Orientation::Z_UP:
            return glm::quat(v.x, v.z, -v.y, v.w);
        case Orientation::Z_MINUS_UP:
            return glm::quat(v.x, -v.z, v.y, v.w);
        case Orientation::X_MINUS_UP:
            return glm::quat(v.y, -v.x, v.z, v.w);
        case Orientation::X_UP:
            return glm::quat(-v.y, v.x, v.z, v.w);
        }
        return glm::quat(v.x, v.y, v.z, v.w);
    }

    static void computeTangents(ofbx::Vec3* out, int vertex_count, const ofbx::Vec3* vertices, const ofbx::Vec3* normals, const ofbx::Vec2* uvs)
    {
        for(int i = 0; i < vertex_count; i += 3)
        {
            const ofbx::Vec3 v0  = vertices[i + 0];
            const ofbx::Vec3 v1  = vertices[i + 1];
            const ofbx::Vec3 v2  = vertices[i + 2];
            const ofbx::Vec2 uv0 = uvs[i + 0];
            const ofbx::Vec2 uv1 = uvs[i + 1];
            const ofbx::Vec2 uv2 = uvs[i + 2];

            const ofbx::Vec3 dv10  = v1 - v0;
            const ofbx::Vec3 dv20  = v2 - v0;
            const ofbx::Vec2 duv10 = uv1 - uv0;
            const ofbx::Vec2 duv20 = uv2 - uv0;

            const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
            ofbx::Vec3 tangent;
            tangent.x     = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
            tangent.y     = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
            tangent.z     = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
            const float l = 1 / sqrtf(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
            tangent.x *= l;
            tangent.y *= l;
            tangent.z *= l;
            out[i + 0] = tangent;
            out[i + 1] = tangent;
            out[i + 2] = tangent;
        }
    }

    glm::vec2 ToLumosVector(const ofbx::Vec2& vec)
    {
        return glm::vec2(float(vec.x), float(vec.y));
    }

    glm::vec3 ToLumosVector(const ofbx::Vec3& vec)
    {
        return glm::vec3(float(vec.x), float(vec.y), float(vec.z));
    }

    glm::vec4 ToLumosVector(const ofbx::Vec4& vec)
    {
        return glm::vec4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
    }

    glm::vec4 ToLumosVector(const ofbx::Color& vec)
    {
        return glm::vec4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
    }

    glm::quat ToLumosQuat(const ofbx::Quat& quat)
    {
        return glm::quat(float(quat.x), float(quat.y), float(quat.z), float(quat.w));
    }

    bool IsMeshInvalid(const ofbx::Mesh* aMesh)
    {
        return aMesh->getGeometry()->getVertexCount() == 0;
    }

    Graphics::Texture2D* LoadTexture(const ofbx::Material* material, ofbx::Texture::TextureType type)
    {
        const ofbx::Texture* ofbxTexture = material->getTexture(type);
        Graphics::Texture2D* texture2D   = nullptr;
        if(ofbxTexture)
        {
            std::string stringFilepath;
            ofbx::DataView filename = ofbxTexture->getRelativeFileName();
            if(filename == "")
                filename = ofbxTexture->getFileName();

            char filePath[MAX_PATH_LENGTH];
            filename.toString(filePath);

            stringFilepath = std::string(filePath);
            stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);

            bool fileFound = false;

            fileFound = FileSystem::FileExists(stringFilepath);

            if(!fileFound)
            {
                stringFilepath = StringUtilities::GetFileName(stringFilepath);
                stringFilepath = m_FBXModelDirectory + "/" + stringFilepath;
                fileFound      = FileSystem::FileExists(stringFilepath);
            }

            if(!fileFound)
            {
                stringFilepath = StringUtilities::GetFileName(stringFilepath);
                stringFilepath = m_FBXModelDirectory + "/textures/" + stringFilepath;
                fileFound      = FileSystem::FileExists(stringFilepath);
            }

            if(fileFound)
            {
                texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath);
            }
        }

        return texture2D;
    }

    SharedPtr<Material> LoadMaterial(const ofbx::Material* material, bool animated)
    {
        // auto shader = animated ? Application::Get().GetShaderLibrary()->GetAsset("//CoreShaders/ForwardPBR.shader") : Application::Get().GetShaderLibrary()->GetAsset("//CoreShaders/ForwardPBR.shader");
        auto shader = Application::Get().GetAssetManager()->GetAssetData("ForwardPBR").As<Graphics::Shader>();

        SharedPtr<Material> pbrMaterial = CreateSharedPtr<Material>(shader);

        PBRMataterialTextures textures;
        Graphics::MaterialProperties properties;

        properties.albedoColour = ToLumosVector(material->getDiffuseColor());
        properties.metallic     = material->getSpecularColor().r;

        float roughness      = 1.0f - Maths::Sqrt(float(material->getShininess()) / 100.0f);
        properties.roughness = roughness;
        properties.roughness = roughness;

        textures.albedo = LoadTexture(material, ofbx::Texture::TextureType::DIFFUSE);
        textures.normal = LoadTexture(material, ofbx::Texture::TextureType::NORMAL);
        // textures.metallic = LoadTexture(material, ofbx::Texture::TextureType::REFLECTION);
        textures.metallic  = LoadTexture(material, ofbx::Texture::TextureType::SPECULAR);
        textures.roughness = LoadTexture(material, ofbx::Texture::TextureType::SHININESS);
        textures.emissive  = LoadTexture(material, ofbx::Texture::TextureType::EMISSIVE);
        textures.ao        = LoadTexture(material, ofbx::Texture::TextureType::AMBIENT);

        if(!textures.albedo)
            properties.albedoMapFactor = 0.0f;
        if(!textures.normal)
            properties.normalMapFactor = 0.0f;
        if(!textures.metallic)
            properties.metallicMapFactor = 0.0f;
        if(!textures.roughness)
            properties.roughnessMapFactor = 0.0f;
        if(!textures.emissive)
            properties.emissiveMapFactor = 0.0f;
        if(!textures.ao)
            properties.occlusionMapFactor = 0.0f;

        pbrMaterial->SetTextures(textures);
        pbrMaterial->SetMaterialProperites(properties);

        return pbrMaterial;
    }

    Maths::Transform GetTransform(const ofbx::Object* mesh)
    {
        auto transform = Maths::Transform();

        ofbx::Vec3 p = mesh->getLocalTranslation();

        glm::vec3 pos = (glm::vec3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)));
        transform.SetLocalPosition(FixOrientation(pos));

        ofbx::Vec3 r  = mesh->getLocalRotation();
        glm::vec3 rot = FixOrientation(glm::vec3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
        transform.SetLocalOrientation(glm::quat(glm::vec3(rot.x, rot.y, rot.z)));

        ofbx::Vec3 s  = mesh->getLocalScaling();
        glm::vec3 scl = glm::vec3(static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z));
        transform.SetLocalScale(scl);

        if(mesh->getParent())
        {
            transform.SetWorldMatrix(GetTransform(mesh->getParent()).GetWorldMatrix());
        }
        else
            transform.SetWorldMatrix(glm::mat4(1.0f));

        return transform;
    }

    SharedPtr<Graphics::Mesh> LoadMesh(const ofbx::Mesh* fbxMesh, int32_t triangleStart, int32_t triangleEnd)
    {
        const int32_t firstVertexOffset = triangleStart * 3;
        const int32_t lastVertexOffset  = triangleEnd * 3;
        const int vertexCount           = lastVertexOffset - firstVertexOffset + 3;

        auto geom                  = fbxMesh->getGeometry();
        auto numIndices            = geom->getIndexCount();
        int vertex_count           = geom->getVertexCount();
        const ofbx::Vec3* vertices = geom->getVertices();
        const ofbx::Vec3* normals  = geom->getNormals();
        const ofbx::Vec3* tangents = geom->getTangents();
        // const ofbx::Vec3* bitangents   = geom->getBitangents();
        const ofbx::Vec4* colours = geom->getColors();
        const ofbx::Vec2* uvs     = geom->getUVs();
        const int* materials      = geom->getMaterials();
        std::vector<Graphics::Vertex> tempvertices(vertex_count);
        std::vector<uint32_t> indicesArray(numIndices);
        ofbx::Vec3* generatedTangents = nullptr;

        int indexCount = 0;
        auto indices   = geom->getFaceIndices();

        auto transform = GetTransform(fbxMesh);

        for(int i = 0; i < vertexCount; i++)
        {
            ofbx::Vec3 cp = vertices[i + firstVertexOffset];

            auto& vertex    = tempvertices[i];
            vertex.Position = transform.GetWorldMatrix() * glm::vec4(float(cp.x), float(cp.y), float(cp.z), 1.0f);
            FixOrientation(vertex.Position);

            if(normals)
                vertex.Normal = transform.GetWorldMatrix() * glm::normalize(glm::vec4(float(normals[i + firstVertexOffset].x), float(normals[i + firstVertexOffset].y), float(normals[i + firstVertexOffset].z), 1.0f));
            // vertex.Normal = transform.GetWorldMatrix().ToMatrix3().Inverse().Transpose() * (glm::vec3(float(normals[i].x), float(normals[i].y), float(normals[i].z))).Normalised();
            if(uvs)
                vertex.TexCoords = glm::vec2(float(uvs[i + firstVertexOffset].x), 1.0f - float(uvs[i + firstVertexOffset].y));
            if(colours)
                vertex.Colours = glm::vec4(float(colours[i + firstVertexOffset].x), float(colours[i + firstVertexOffset].y), float(colours[i + firstVertexOffset].z), float(colours[i + firstVertexOffset].w));

            FixOrientation(vertex.Normal);
            FixOrientation(vertex.Tangent);
        }

        for(int i = 0; i < vertexCount; i++)
        {
            indexCount++;

            int index       = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];
            indicesArray[i] = i; // index;
        }

        const ofbx::Material* material = nullptr;
        if(fbxMesh->getMaterialCount() > 0)
        {
            if(geom->getMaterials())
                material = fbxMesh->getMaterial(geom->getMaterials()[triangleStart]);
            else
                material = fbxMesh->getMaterial(0);
        }

        SharedPtr<Material> pbrMaterial;
        if(material)
        {
            pbrMaterial = LoadMaterial(material, false);
        }

        auto mesh = CreateSharedPtr<Graphics::Mesh>(indicesArray, tempvertices);
        mesh->SetName(fbxMesh->name);
        if(material)
            mesh->SetMaterial(pbrMaterial);

        mesh->Graphics::Mesh::GenerateTangentsAndBitangents(tempvertices.data(), uint32_t(vertexCount), indicesArray.data(), uint32_t(indicesArray.size()));

        return mesh;
    }

    glm::mat4 FbxMatrixToLM(const ofbx::Matrix& mat)
    {
        glm::mat4 result;
        for(int32_t i = 0; i < 4; i++)
            for(int32_t j = 0; j < 4; j++)
                result[i][j] = (float)mat.m[i * 4 + j];
        return result;
    }

    glm::mat4 GetOffsetMatrix(const ofbx::Mesh* mesh, const ofbx::Object* node)
    {
        auto* skin = mesh ? mesh->getGeometry()->getSkin() : nullptr;
        if(skin)
        {
            for(int i = 0, c = skin->getClusterCount(); i < c; i++)
            {
                const ofbx::Cluster* cluster = skin->getCluster(i);
                if(cluster->getLink() == node)
                {
                    return FbxMatrixToLM(cluster->getTransformLinkMatrix());
                }
            }
        }
        return FbxMatrixToLM(node->getGlobalTransform());
    }

    void Model::LoadFBX(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string err;
        std::string pathCopy = path;
        pathCopy             = StringUtilities::BackSlashesToSlashes(pathCopy);
        m_FBXModelDirectory  = pathCopy.substr(0, pathCopy.find_last_of('/'));

        std::string name = m_FBXModelDirectory.substr(m_FBXModelDirectory.find_last_of('/') + 1);

        std::string ext = StringUtilities::GetFilePathExtension(path);
        int64_t size    = FileSystem::GetFileSize(path);
        auto data       = FileSystem::ReadFile(path);

        if(data == nullptr)
        {
            LUMOS_LOG_WARN("Failed to load fbx file");
            return;
        }
        const bool ignoreGeometry = false;
        const uint64_t flags      = ignoreGeometry ? (uint64_t)ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t)ofbx::LoadFlags::TRIANGULATE;

        ofbx::IScene* scene                  = ofbx::load(data, uint32_t(size), flags);
        const ofbx::GlobalSettings* settings = scene->getGlobalSettings();

        err = ofbx::getError();

        if(!err.empty() || !scene)
        {
            LUMOS_LOG_CRITICAL(err);
        }

        switch(settings->UpAxis)
        {
        case ofbx::UpVector_AxisX:
            orientation = Orientation::X_UP;
            break;
        case ofbx::UpVector_AxisY:
            orientation = Orientation::Y_UP;
            break;
        case ofbx::UpVector_AxisZ:
            orientation = Orientation::Z_UP;
            break;
        }

        int meshCount = scene->getMeshCount();
        std::vector<const ofbx::Mesh*> meshes;
        // auto skeleton = ImportSkeleton(scene->getMesh());
        // loadAnimation(fileName, scene, sceneBone, outRes, orientation);

#ifdef THREAD_MESH_LOADING
        System::JobSystem::Context ctx;
        System::JobSystem::Dispatch(ctx, static_cast<uint32_t>(meshCount), 1, [&](JobDispatchArgs args)
#else
        for(int i = 0; i < meshCount; ++i)
#endif
                                    {
#ifdef THREAD_MESH_LOADING
                int i = args.jobIndex;
#endif

            const ofbx::Mesh* fbxMesh = (const ofbx::Mesh*)scene->getMesh(i);

            meshes.push_back(fbxMesh);

            const auto geometry       = fbxMesh->getGeometry();
            const auto trianglesCount = geometry->getVertexCount() / 3;

            if(IsMeshInvalid(fbxMesh))
                continue;

            if(fbxMesh->getMaterialCount() < 2 || !geometry->getMaterials())
            {
                m_Meshes.push_back(LoadMesh(fbxMesh, 0, trianglesCount - 1));
            }
            else
            {
                // Create mesh for each material

                const auto materials       = geometry->getMaterials();
                int32_t rangeStart         = 0;
                int32_t rangeStartMaterial = materials[rangeStart];
                for(int32_t triangleIndex = 1; triangleIndex < trianglesCount; triangleIndex++)
                {
                    if(rangeStartMaterial != materials[triangleIndex])
                    {
                        m_Meshes.push_back(LoadMesh(fbxMesh, rangeStart, triangleIndex - 1));

                        // Start a new range
                        rangeStart         = triangleIndex;
                        rangeStartMaterial = materials[triangleIndex];
                    }
                }
                m_Meshes.push_back(LoadMesh(fbxMesh, rangeStart, trianglesCount - 1));
            } }
#ifdef THREAD_MESH_LOADING
        );
        System::JobSystem::Wait(ctx);
#endif
    }

}
