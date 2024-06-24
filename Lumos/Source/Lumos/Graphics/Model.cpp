#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "Utilities/StringUtilities.h"
#include "Core/OS/FileSystem.h"
#include "Animation/Skeleton.h"
#include "Animation/Animation.h"
#include "Animation/AnimationController.h"
#include "Animation/SamplingContext.h"
#include "AI/AStar.h"

namespace Lumos::Graphics
{
    Model::Model()
        : m_FilePath()
        , m_PrimitiveType(PrimitiveType::None)
    {
    }

    Model::Model(const std::string& filePath)
        : m_FilePath(filePath)
        , m_PrimitiveType(PrimitiveType::File)
    {
        LoadModel(m_FilePath);
    }

    Model::Model(const SharedPtr<Mesh>& mesh, PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.push_back(mesh);
    }

    Model::Model(PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.push_back(SharedPtr<Mesh>(CreatePrimative(type)));
    }

    Model::~Model()
    {
    }

    Model::Model(const Model&)            = default;
    Model& Model::operator=(const Model&) = default;
    Model::Model(Model&&)                 = default;
    Model& Model::operator=(Model&&)      = default;

    void Model::LoadModel(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(path, physicalPath))
        {
            LUMOS_LOG_INFO("Failed to load Model - {0}", path);
            return;
        }

        std::string resolvedPath = physicalPath;

        const std::string fileExtension = StringUtilities::GetFilePathExtension(path);

        if(fileExtension == "obj")
            LoadOBJ(resolvedPath);
        else if(fileExtension == "gltf" || fileExtension == "glb")
            LoadGLTF(resolvedPath);
        else if(fileExtension == "fbx" || fileExtension == "FBX")
            LoadFBX(resolvedPath);
        else
            LUMOS_LOG_ERROR("Unsupported File Type : {0}", fileExtension);

        LUMOS_LOG_INFO("Loaded Model - {0}", path);
    }

    void Model::UpdateAnimation(const TimeStep& dt)
    {
        if(m_Animation.empty())
            return;

        if(!m_SamplingContext)
        {
            m_SamplingContext = CreateSharedPtr<SamplingContext>();
        }

        if(!m_AnimationController)
        {
            m_AnimationController = CreateSharedPtr<AnimationController>();
            m_AnimationController->SetSkeleton(m_Skeleton);
            for(auto anim : m_Animation)
            {
                m_AnimationController->AddState(anim->GetName(), anim);
            }
            m_AnimationController->SetBindPoses(m_BindPoses);
        }

        static float time = 0.0f;
        time += (float)dt.GetSeconds();
        m_AnimationController->SetCurrentState(m_CurrentAnimation);
        m_AnimationController->Update(time, *m_SamplingContext.get());
    }

    void Model::UpdateAnimation(const TimeStep& dt, float overrideTime)
    {
        if(m_Animation.empty())
            return;

        if(!m_SamplingContext)
        {
            m_SamplingContext = CreateSharedPtr<SamplingContext>();
        }

        if(!m_AnimationController)
        {
            m_AnimationController = CreateSharedPtr<AnimationController>();
            m_AnimationController->SetSkeleton(m_Skeleton);
            for(auto anim : m_Animation)
            {
                m_AnimationController->AddState(anim->GetName(), anim);
            }
            m_AnimationController->SetBindPoses(m_BindPoses);
        }

        m_AnimationController->SetCurrentState(m_CurrentAnimation);
        m_AnimationController->Update(overrideTime, *m_SamplingContext.get());
    }

    std::vector<glm::mat4> Model::GetJointMatrices()
    {
        if(m_Animation.empty())
            return {};

        auto matrices = m_AnimationController->GetJointMatrices();

        for(int i = 0; i < matrices.size(); i++)
        {
            // matrices[i] = m_BindPoses[i] * matrices[i];
        }

        return matrices;
    }

    std::vector<SharedPtr<Mesh>>& Model::GetMeshesRef()
    {
        return m_Meshes;
    }
    const std::vector<SharedPtr<Mesh>>& Model::GetMeshes() const
    {
        return m_Meshes;
    }
    void Model::AddMesh(SharedPtr<Mesh> mesh)
    {
        m_Meshes.push_back(mesh);
    }
    SharedPtr<Skeleton> Model::GetSkeleton() const
    {
        return m_Skeleton;
    }

    const std::vector<SharedPtr<Animation>>& Model::GetAnimations() const
    {
        return m_Animation;
    }
    SharedPtr<SamplingContext> Model::GetSamplingContext() const
    {
        return m_SamplingContext;
    }

    SharedPtr<AnimationController> Model::GetAnimationController() const
    {
        return m_AnimationController;
    }
}
