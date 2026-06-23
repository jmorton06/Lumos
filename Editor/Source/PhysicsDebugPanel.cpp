#include "PhysicsDebugPanel.h"
#include "Editor.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>

#include <imgui/imgui.h>

namespace Lumos
{
    PhysicsDebugPanel::PhysicsDebugPanel()
    {
        m_Name       = "Physics Debug###PhysicsDebug";
        m_SimpleName = "Physics Debug";
    }

    static void FlagCheckbox(const char* label, uint32_t& flags, uint32_t bit)
    {
        bool on = (flags & bit) != 0;
        if(ImGui::Checkbox(label, &on))
            flags = on ? (flags | bit) : (flags & ~bit);
    }

    void PhysicsDebugPanel::OnImGui()
    {
        ImGuiUtilities::BeginPanel(m_Name.c_str(), &m_Active, ImGuiWindowFlags_NoCollapse);

        if(!m_Editor)
        {
            ImGui::Text("Editor not bound.");
            ImGui::End();
            return;
        }

        auto& settings = m_Editor->GetSettings();
        uint32_t& f3   = settings.m_Physics3DDebugFlags;
        uint32_t& f2   = settings.m_Physics2DDebugFlags;

        if(ImGui::CollapsingHeader("3D Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            FlagCheckbox("Collision Volumes", f3, PhysicsDebugFlags::COLLISIONVOLUMES);
            FlagCheckbox("Collision Normals", f3, PhysicsDebugFlags::COLLISIONNORMALS);
            FlagCheckbox("AABB", f3, PhysicsDebugFlags::AABB);
            FlagCheckbox("Bounding Radius", f3, PhysicsDebugFlags::BOUNDING_RADIUS);
            FlagCheckbox("Linear Velocity", f3, PhysicsDebugFlags::LINEARVELOCITY);
            FlagCheckbox("Linear Force", f3, PhysicsDebugFlags::LINEARFORCE);
            FlagCheckbox("Constraints", f3, PhysicsDebugFlags::CONSTRAINT);
            FlagCheckbox("Manifolds", f3, PhysicsDebugFlags::MANIFOLD);
            FlagCheckbox("Broadphase", f3, PhysicsDebugFlags::BROADPHASE);
            FlagCheckbox("Broadphase Pairs", f3, PhysicsDebugFlags::BROADPHASE_PAIRS);

            if(ImGui::Button("All 3D"))
                f3 = 0xFFFFFFFF;
            ImGui::SameLine();
            if(ImGui::Button("None 3D"))
                f3 = 0;

            if(auto* engine = Application::Get().GetSystem<LumosPhysicsEngine>())
            {
                engine->SetDebugDrawFlags(f3);
                const auto& s = engine->GetStats();
                ImGui::Separator();
                ImGui::Text("Bodies: %u  Static: %u  Resting: %u",
                            s.RigidBodyCount, s.StaticCount, s.RestCount);
                ImGui::Text("Collisions: %u  Constraints: %u",
                            s.CollisionCount, s.ConstraintCount);
            }
        }

        ImGui::Separator();

        if(ImGui::CollapsingHeader("2D Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            FlagCheckbox("Collision Volumes##2D", f2, PhysicsDebugFlags2D::COLLISIONVOLUMES2D);
            FlagCheckbox("Collision Normals##2D", f2, PhysicsDebugFlags2D::COLLISIONNORMALS2D);
            FlagCheckbox("AABB##2D", f2, PhysicsDebugFlags2D::AABB2D);
            FlagCheckbox("Linear Velocity##2D", f2, PhysicsDebugFlags2D::LINEARVELOCITY2D);
            FlagCheckbox("Linear Force##2D", f2, PhysicsDebugFlags2D::LINEARFORCE2D);
            FlagCheckbox("Constraints##2D", f2, PhysicsDebugFlags2D::CONSTRAINT2D);
            FlagCheckbox("Manifolds##2D", f2, PhysicsDebugFlags2D::MANIFOLD2D);

            if(ImGui::Button("All 2D"))
                f2 = 0xFFFFFFFF;
            ImGui::SameLine();
            if(ImGui::Button("None 2D"))
                f2 = 0;

            if(auto* engine = Application::Get().GetSystem<B2PhysicsEngine>())
                engine->SetDebugDrawFlags(f2);
        }

        ImGui::End();
    }
}
