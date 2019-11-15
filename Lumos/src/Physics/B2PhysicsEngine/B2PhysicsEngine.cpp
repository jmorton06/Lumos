#include "lmpch.h"
#include "B2PhysicsEngine.h"
#include "PhysicsObject2D.h"

#include "Utilities/TimeStep.h"
#include "Core/Profiler.h"
#include "ECS/Component/Physics2DComponent.h"

#include "Maths/Transform.h"

#include <Box2D/Box2D.h>
#include <Box2D/Common/b2Math.h>

#include <imgui/imgui.h>

namespace Lumos
{
	B2PhysicsEngine::B2PhysicsEngine()
		: m_UpdateTimestep(1.0f / 60.f)
		, m_UpdateAccum(0.0f)
        , m_B2DWorld(CreateScope<b2World>(b2Vec2(0.0f,-9.81f)))
	{
        m_DebugName = "Box2D Physics Engine";
	}

	B2PhysicsEngine::~B2PhysicsEngine()
	{
	}

	void B2PhysicsEngine::SetDefaults()
	{
		m_UpdateTimestep = 1.0f / 60.f;
		m_UpdateAccum = 0.0f;
	}

	void B2PhysicsEngine::OnUpdate(TimeStep* timeStep, Scene* scene)
	{
		LUMOS_PROFILE_FUNC;
		const int max_updates_per_frame = 5;

		if (!m_Paused)
		{	
			if(m_MultipleUpdates)
			{
				m_UpdateAccum += timeStep->GetMillis();
				for (int i = 0; (m_UpdateAccum >= m_UpdateTimestep) && i < max_updates_per_frame; ++i)
				{
					m_UpdateAccum -= m_UpdateTimestep;
					m_B2DWorld->Step(m_UpdateTimestep, 6, 2);
				}

				if (m_UpdateAccum >= m_UpdateTimestep)
				{
					LUMOS_LOG_CRITICAL("Physics too slow to run in real time!");
					//Drop Time in the hope that it can continue to run in real-time
					m_UpdateAccum = 0.0f;
				}

			}
			else
				m_B2DWorld->Step(m_UpdateTimestep, 6, 2);
            
            auto& registry = scene->GetRegistry();
            
            auto group = registry.group<Physics2DComponent>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto &[phys, trans] = group.get<Physics2DComponent, Maths::Transform>(entity);

               // if (!phys.GetPhysicsObject()->GetB2Body()->IsAwake())
               //     break;

                trans.SetLocalPosition(Maths::Vector3(phys.GetPhysicsObject()->GetPosition(), 1.0f));
                trans.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, phys.GetPhysicsObject()->GetAngle() * Maths::M_DEGTORAD));
                trans.SetWorldMatrix(Maths::Matrix4()); // temp
            };
		}
	}

	void B2PhysicsEngine::OnImGui()
	{
		ImGui::TextUnformatted("2D Physics Engine");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Number Of Collision Pairs");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", m_B2DWorld->GetContactCount());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Number Of Physics Objects");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", m_B2DWorld->GetBodyCount());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Paused");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Checkbox("##Paused", &m_Paused);
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Gravity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		float grav[2] = { m_B2DWorld->GetGravity().x , m_B2DWorld->GetGravity().y };
		if (ImGui::InputFloat2("##Gravity", grav))
			m_B2DWorld->SetGravity({ grav[0], grav[1] });
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	b2Body* B2PhysicsEngine::CreateB2Body(b2BodyDef* bodyDef) const
	{
		return m_B2DWorld->CreateBody(bodyDef); 
	}

	void B2PhysicsEngine::CreateFixture(b2Body* body, const b2FixtureDef* fixtureDef)
	{
		body->CreateFixture(fixtureDef);
	}
}
