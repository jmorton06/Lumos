#include "Precompiled.h"
#include "RigidBody3DComponent.h"
#include "Scene/Scene.h"
#include "Scene/EntityManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>

namespace Lumos
{
    RigidBody3DInstance::RigidBody3DInstance()
    {
        Body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
    }

    RigidBody3DInstance::RigidBody3DInstance(RigidBody3D* physics)
    {
        Body = physics;
    }

    RigidBody3DInstance::RigidBody3DInstance(const RigidBody3DProperties& params)
    {
        Body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody(params);
    }

    RigidBody3DInstance::~RigidBody3DInstance()
    {
        if(Body)
            Application::Get().GetSystem<LumosPhysicsEngine>()->DestroyBody(Body);
    }

    RigidBody3DComponent::RigidBody3DComponent()
    {
        m_RigidBody = CreateSharedPtr<RigidBody3DInstance>();
    }

    RigidBody3DComponent::RigidBody3DComponent(RigidBody3D* physics)
    {
        m_RigidBody = CreateSharedPtr<RigidBody3DInstance>(physics);
    }

    RigidBody3DComponent::RigidBody3DComponent(const RigidBody3DProperties& properties)
    {
        m_RigidBody = CreateSharedPtr<RigidBody3DInstance>(properties);
    }

    RigidBody3DComponent::RigidBody3DComponent(const RigidBody3DComponent& other)
    {
        m_RigidBody    = other.m_RigidBody;
        m_OwnRigidBody = other.m_OwnRigidBody;
    }

    RigidBody3DComponent::~RigidBody3DComponent()
    {
    }

    void RigidBody3DComponent::Init()
    {
    }

    void RigidBody3DComponent::Update()
    {
    }

    void RigidBody3DComponent::OnImGui()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        auto pos             = m_RigidBody->Body->GetPosition();
        auto torque          = m_RigidBody->Body->GetTorque();
        auto orientation     = m_RigidBody->Body->GetOrientation();
        auto angularVelocity = m_RigidBody->Body->GetAngularVelocity();
        auto friction        = m_RigidBody->Body->GetFriction();
        auto isStatic        = m_RigidBody->Body->GetIsStatic();
        auto isRest          = m_RigidBody->Body->GetIsAtRest();
        auto mass            = 1.0f / m_RigidBody->Body->GetInverseMass();
        auto velocity        = m_RigidBody->Body->GetLinearVelocity();
        auto elasticity      = m_RigidBody->Body->GetElasticity();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", glm::value_ptr(pos)))
            m_RigidBody->Body->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Velocity", glm::value_ptr(velocity)))
            m_RigidBody->Body->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Torque", glm::value_ptr(torque)))
            m_RigidBody->Body->SetTorque(torque);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat4("##Orientation", glm::value_ptr(orientation)))
            m_RigidBody->Body->SetOrientation(orientation);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Angular Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Angular Velocity", glm::value_ptr(angularVelocity)))
            m_RigidBody->Body->SetAngularVelocity(angularVelocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction))
            m_RigidBody->Body->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Mass", &mass))
            m_RigidBody->Body->SetInverseMass(1.0f / mass);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity))
            m_RigidBody->Body->SetElasticity(elasticity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Static", &isStatic))
            m_RigidBody->Body->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##At Rest", &isRest))
            m_RigidBody->Body->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    SpringConstraintComponent::SpringConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = CreateSharedPtr<SpringConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(), pos1, pos2, 0.9f, 0.5f);
    }
    SpringConstraintComponent::SpringConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = CreateSharedPtr<SpringConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(), 0.9f, 0.5f);
    }

    void SpringConstraintComponent::Initialise()
    {
        Scene* scene   = Application::Get().GetCurrentScene();
        Entity entity1 = scene->GetEntityByUUID(m_EntityID);
        Entity entity2 = scene->GetEntityByUUID(m_OtherEntityID);

        if(entity1 && entity2 && entity1.HasComponent<RigidBody3DComponent>() && entity2.HasComponent<RigidBody3DComponent>())
        {
            m_Constraint  = CreateSharedPtr<SpringConstraint>(entity1.GetComponent<RigidBody3DComponent>().GetRigidBody(), entity2.GetComponent<RigidBody3DComponent>().GetRigidBody(), m_Constant, 0.5f);
            m_Initialised = true;
        }
    }

    WeldConstraintComponent::WeldConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = CreateSharedPtr<WeldConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody());
    }

    WeldConstraintComponent::WeldConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = CreateSharedPtr<WeldConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody());
    }

    DistanceConstraintComponent::DistanceConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = CreateSharedPtr<DistanceConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(), pos1, pos2);
    }

    DistanceConstraintComponent::DistanceConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = CreateSharedPtr<DistanceConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(),
                                                           entity.GetComponent<RigidBody3DComponent>().GetRigidBody()->GetPosition(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody()->GetPosition());
    }

    AxisConstraintComponent::AxisConstraintComponent(Entity entity, Axes axes)
    {
        m_EntityID    = entity.GetID();
        m_Axes        = axes;
        m_Constraint  = CreateSharedPtr<AxisConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), axes);
        m_Initialised = true;
    }

    const SharedPtr<AxisConstraint>& AxisConstraintComponent::GetConstraint()
    {
        if(!m_Initialised)
        {
            auto entity = Application::Get().GetCurrentScene()->GetEntityManager()->GetEntityByUUID(m_EntityID);

            if(entity && entity.HasComponent<RigidBody3DComponent>())
                m_Constraint = CreateSharedPtr<AxisConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), m_Axes);
            m_Initialised = true;
        }
        return m_Constraint;
    }
}
