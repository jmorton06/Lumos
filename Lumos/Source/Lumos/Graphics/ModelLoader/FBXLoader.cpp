#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Core/OS/FileSystem.h"

#include "Graphics/RHI/Texture.h"
#include "Maths/Maths.h"

#include "Maths/Transform.h"
#include "Core/Application.h"
#include "Core/StringUtilities.h"

#include <OpenFBX/ofbx.h>

//#define THREAD_MESH_LOADING //Can't use with opengl
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
    float fbx_scale = 1.f;

    static ofbx::Vec3 operator-(const ofbx::Vec3& a, const ofbx::Vec3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    static ofbx::Vec2 operator-(const ofbx::Vec2& a, const ofbx::Vec2& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    Maths::Vector3 FixOrientation(const Maths::Vector3& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return Maths::Vector3(v.x, v.y, v.z);
        case Orientation::Z_UP:
            return Maths::Vector3(v.x, v.z, -v.y);
        case Orientation::Z_MINUS_UP:
            return Maths::Vector3(v.x, -v.z, v.y);
        case Orientation::X_MINUS_UP:
            return Maths::Vector3(v.y, -v.x, v.z);
        case Orientation::X_UP:
            return Maths::Vector3(-v.y, v.x, v.z);
        }
        return Maths::Vector3(v.x, v.y, v.z);
    }

    Maths::Quaternion FixOrientation(const Maths::Quaternion& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return Maths::Quaternion(v.x, v.y, v.z, v.w);
        case Orientation::Z_UP:
            return Maths::Quaternion(v.x, v.z, -v.y, v.w);
        case Orientation::Z_MINUS_UP:
            return Maths::Quaternion(v.x, -v.z, v.y, v.w);
        case Orientation::X_MINUS_UP:
            return Maths::Quaternion(v.y, -v.x, v.z, v.w);
        case Orientation::X_UP:
            return Maths::Quaternion(-v.y, v.x, v.z, v.w);
        }
        return Maths::Quaternion(v.x, v.y, v.z, v.w);
    }

    static void computeTangents(ofbx::Vec3* out, int vertex_count, const ofbx::Vec3* vertices, const ofbx::Vec3* normals, const ofbx::Vec2* uvs)
    {
        for(int i = 0; i < vertex_count; i += 3)
        {
            const ofbx::Vec3 v0 = vertices[i + 0];
            const ofbx::Vec3 v1 = vertices[i + 1];
            const ofbx::Vec3 v2 = vertices[i + 2];
            const ofbx::Vec2 uv0 = uvs[i + 0];
            const ofbx::Vec2 uv1 = uvs[i + 1];
            const ofbx::Vec2 uv2 = uvs[i + 2];

            const ofbx::Vec3 dv10 = v1 - v0;
            const ofbx::Vec3 dv20 = v2 - v0;
            const ofbx::Vec2 duv10 = uv1 - uv0;
            const ofbx::Vec2 duv20 = uv2 - uv0;

            const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
            ofbx::Vec3 tangent;
            tangent.x = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
            tangent.y = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
            tangent.z = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
            const float l = 1 / sqrtf(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
            tangent.x *= l;
            tangent.y *= l;
            tangent.z *= l;
            out[i + 0] = tangent;
            out[i + 1] = tangent;
            out[i + 2] = tangent;
        }
    }

    Maths::Vector2 ToLumosVector(const ofbx::Vec2& vec)
    {
        return Maths::Vector2(float(vec.x), float(vec.y));
    }

    Maths::Vector3 ToLumosVector(const ofbx::Vec3& vec)
    {
        return Maths::Vector3(float(vec.x), float(vec.y), float(vec.z));
    }

    Maths::Vector4 ToLumosVector(const ofbx::Vec4& vec)
    {
        return Maths::Vector4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
    }

    Maths::Vector4 ToLumosVector(const ofbx::Color& vec)
    {
        return Maths::Vector4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
    }

    Maths::Quaternion ToLumosQuat(const ofbx::Quat& quat)
    {
        return Maths::Quaternion(float(quat.x), float(quat.y), float(quat.z), float(quat.w));
    }

    Graphics::Texture2D* LoadTexture(const ofbx::Material* material, ofbx::Texture::TextureType type)
    {
        const ofbx::Texture* ofbxTexture = material->getTexture(type);
        Graphics::Texture2D* texture2D = nullptr;
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
                fileFound = FileSystem::FileExists(stringFilepath);
            }

            if(!fileFound)
            {
                stringFilepath = StringUtilities::GetFileName(stringFilepath);
                stringFilepath = m_FBXModelDirectory + "/textures/" + stringFilepath;
                fileFound = FileSystem::FileExists(stringFilepath);
            }

            if(fileFound)
            {
                texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath);
            }
        }

        return texture2D;
    }

    SharedRef<Material> LoadMaterial(const ofbx::Material* material, bool animated)
    {
        auto shader = animated ? Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader") : Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");

        SharedRef<Material> pbrMaterial = CreateSharedRef<Material>(shader);

        PBRMataterialTextures textures;
        Graphics::MaterialProperties properties;

        properties.albedoColour = ToLumosVector(material->getDiffuseColor());
        properties.metallicColour = ToLumosVector(material->getSpecularColor());

        float roughness = 1.0f - Maths::Sqrt(float(material->getShininess()) / 100.0f);
        properties.roughnessColour = Maths::Vector3(roughness);

        textures.albedo = LoadTexture(material, ofbx::Texture::TextureType::DIFFUSE);
        textures.normal = LoadTexture(material, ofbx::Texture::TextureType::NORMAL);
        //textures.metallic = LoadTexture(material, ofbx::Texture::TextureType::REFLECTION);
        textures.metallic = LoadTexture(material, ofbx::Texture::TextureType::SPECULAR);
        textures.roughness = LoadTexture(material, ofbx::Texture::TextureType::SHININESS);
        textures.emissive = LoadTexture(material, ofbx::Texture::TextureType::EMISSIVE);
        textures.ao = LoadTexture(material, ofbx::Texture::TextureType::AMBIENT);

        if(!textures.albedo)
            properties.usingAlbedoMap = 0.0f;
        if(!textures.normal)
            properties.usingNormalMap = 0.0f;
        if(!textures.metallic)
            properties.usingMetallicMap = 0.0f;
        if(!textures.roughness)
            properties.usingRoughnessMap = 0.0f;
        if(!textures.emissive)
            properties.usingEmissiveMap = 0.0f;
        if(!textures.ao)
            properties.usingAOMap = 0.0f;

        pbrMaterial->SetTextures(textures);
        pbrMaterial->SetMaterialProperites(properties);

        return pbrMaterial;
    }

    Maths::Transform GetTransform(const ofbx::Object* mesh)
    {
        auto transform = Maths::Transform();

        ofbx::Vec3 p = mesh->getLocalTranslation();

        Maths::Vector3 pos = (Maths::Vector3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)));
        transform.SetLocalPosition(FixOrientation(pos));

        ofbx::Vec3 r = mesh->getLocalRotation();
        Maths::Vector3 rot = FixOrientation(Maths::Vector3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
        transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(rot.x, rot.y, rot.z));

        ofbx::Vec3 s = mesh->getLocalScaling();
        Maths::Vector3 scl = Maths::Vector3(static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z));
        transform.SetLocalScale(scl);

        if(mesh->getParent())
        {
            transform.SetWorldMatrix(GetTransform(mesh->getParent()).GetWorldMatrix());
        }
        else
            transform.SetWorldMatrix(Maths::Matrix4());

        return transform;
    }

    void Model::LoadFBX(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string err;
        std::string pathCopy = path;
        pathCopy = StringUtilities::BackSlashesToSlashes(pathCopy);
        m_FBXModelDirectory = pathCopy.substr(0, pathCopy.find_last_of('/'));

        std::string name = m_FBXModelDirectory.substr(m_FBXModelDirectory.find_last_of('/') + 1);

        std::string ext = StringUtilities::GetFilePathExtension(path);
        int64_t size = FileSystem::GetFileSize(path);
        auto data = FileSystem::ReadFile(path);

        if(data == nullptr)
        {
            LUMOS_LOG_WARN("Failed to load fbx file");
            return;
        }
        const bool ignoreGeometry = false;
        const uint64_t flags = ignoreGeometry ? (uint64_t)ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t)ofbx::LoadFlags::TRIANGULATE;

        ofbx::IScene* scene = ofbx::load(data, uint32_t(size), flags);

        err = ofbx::getError();

        if(!err.empty() || !scene)
        {
            LUMOS_LOG_CRITICAL(err);
        }

        const ofbx::GlobalSettings* settings = scene->getGlobalSettings();
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

                const ofbx::Mesh* fbx_mesh = (const ofbx::Mesh*)scene->getMesh(i);
                auto geom = fbx_mesh->getGeometry();
                auto numIndices = geom->getIndexCount();
                int vertex_count = geom->getVertexCount();
                const ofbx::Vec3* vertices = geom->getVertices();
                const ofbx::Vec3* normals = geom->getNormals();
                const ofbx::Vec3* tangents = geom->getTangents();
                const ofbx::Vec4* colours = geom->getColors();
                const ofbx::Vec2* uvs = geom->getUVs();
                Graphics::Vertex* tempvertices = new Graphics::Vertex[vertex_count];
                uint32_t* indicesArray = new uint32_t[numIndices];

                SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();

                auto indices = geom->getFaceIndices();

                ofbx::Vec3* generatedTangents = nullptr;
                if(!tangents && normals && uvs)
                {
                    generatedTangents = new ofbx::Vec3[vertex_count];
                    computeTangents(generatedTangents, vertex_count, vertices, normals, uvs);
                    tangents = generatedTangents;
                }

                auto transform = GetTransform(fbx_mesh);

                for(int i = 0; i < vertex_count; ++i)
                {
                    ofbx::Vec3 cp = vertices[i];

                    auto& vertex = tempvertices[i];
                    vertex.Position = transform.GetWorldMatrix() * Maths::Vector3(float(cp.x), float(cp.y), float(cp.z));
                    FixOrientation(vertex.Position);
                    boundingBox->Merge(vertex.Position);

                    if(normals)
                        vertex.Normal = transform.GetWorldMatrix().ToMatrix3().Inverse().Transpose() * (Maths::Vector3(float(normals[i].x), float(normals[i].y), float(normals[i].z))).Normalised();
                    if(uvs)
                        vertex.TexCoords = Maths::Vector2(float(uvs[i].x), 1.0f - float(uvs[i].y));
                    if(colours)
                        vertex.Colours = Maths::Vector4(float(colours[i].x), float(colours[i].y), float(colours[i].z), float(colours[i].w));
                    if(tangents)
                        vertex.Tangent = transform.GetWorldMatrix() * Maths::Vector3(float(tangents[i].x), float(tangents[i].y), float(tangents[i].z));

                    FixOrientation(vertex.Normal);
                    FixOrientation(vertex.Tangent);
                }

                for(int i = 0; i < numIndices; i++)
                {
                    int index = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];

                    indicesArray[i] = index;
                }

                SharedRef<Graphics::VertexBuffer> vb = SharedRef<Graphics::VertexBuffer>(Graphics::VertexBuffer::Create());
                vb->SetData(sizeof(Graphics::Vertex) * vertex_count, tempvertices);

                SharedRef<Graphics::IndexBuffer> ib;
                ib.reset(Graphics::IndexBuffer::Create(indicesArray, numIndices));

                const ofbx::Material* material = fbx_mesh->getMaterialCount() > 0 ? fbx_mesh->getMaterial(0) : nullptr;
                SharedRef<Material> pbrMaterial;
                if(material)
                {
                    pbrMaterial = LoadMaterial(material, false);
                }

                auto mesh = CreateSharedRef<Graphics::Mesh>(vb, ib, boundingBox);
                mesh->SetName(fbx_mesh->name);
                if(material)
                    mesh->SetMaterial(pbrMaterial);

                m_Meshes.push_back(mesh);

                if(generatedTangents)
                    delete[] generatedTangents;
                delete[] tempvertices;
                delete[] indicesArray;
            }
#ifdef THREAD_MESH_LOADING
        );
        System::JobSystem::Wait(ctx);
#endif
    }

}
