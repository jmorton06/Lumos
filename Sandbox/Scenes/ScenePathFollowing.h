#pragma once

#include <JMEngine.h>

using namespace jm;
using namespace maths;

const float time_between_nodes = 0.5f;
const int num_control_points = 10;
const float total_circuit_time = float(num_control_points) * time_between_nodes;

class ScenePathFollowing : public Scene
{
protected:
	std::shared_ptr<Entity> player;
	std::vector<std::shared_ptr<Entity>> path_nodes;
	std::vector<maths::Quaternion> path_rotations;
	int current_node;
	float elapsed_time;

	float cubic_tangent_weighting;

	int interpolation_type_pos;
	int interpolation_type_rot;

public:

	void GetNodeIdxAndFactor(float elapsed, int& out_idx, float& out_factor) const
	{
		elapsed = fmod(elapsed, total_circuit_time);
		float node_time = elapsed / time_between_nodes;
		double node_time_floored;

		out_factor = static_cast<float>(modf(node_time, &node_time_floored));
		out_idx = static_cast<int>(node_time_floored);
	}

	void OnUpdate(TimeStep* timeStep) override
	{
		UpdatePathForwardVectors();

		const size_t num_nodes = path_nodes.size();
		elapsed_time += timeStep->GetSeconds();
		elapsed_time = fmod(elapsed_time, total_circuit_time);

		int current_node;
		float factor;
		GetNodeIdxAndFactor(elapsed_time, current_node, factor);

		maths::Vector3 player_pos;
		switch (interpolation_type_pos)
		{
		default:
		case 0:
			player_pos = InterpolatePositionLinear(
				current_node,
				static_cast<int>((current_node + 1) % num_nodes),
				factor);

			break;
		case 1:
			player_pos = InterpolatePositionCubic(
					static_cast<int>((current_node + num_nodes - 1) % num_nodes),
				current_node,
				static_cast<int>((current_node + 1) % num_nodes),
					static_cast<int>((current_node + 2) % num_nodes),
				factor);
			break;
		}

		maths::Quaternion player_rot;
		switch (interpolation_type_rot)
		{
		default:
		case 0:
		{
			player_rot = InterpolateRotationLinear(
				current_node,
				static_cast<int>((current_node + 1) % num_nodes),
				factor, false);
		}
		break;
		case 1:
		{
			player_rot = InterpolateRotationLinear(
				current_node,
				static_cast<int>((current_node + 1) % num_nodes),
				factor, true);
		}
		break;
		case 2:
		{
			GetNodeIdxAndFactor(elapsed_time + timeStep->GetSeconds(), current_node, factor);
			maths::Vector3 ahead;
			if (interpolation_type_pos == 0)
			{
				player_pos = InterpolatePositionLinear(
					current_node,
					(current_node + 1) % (int)num_nodes,
					factor);
			}
			else
			{
				ahead = InterpolatePositionCubic(
					(current_node + num_nodes - 1) % (int)num_nodes,
					current_node,
					(current_node + 1) % (int)num_nodes,
					(current_node + 2) % (int)num_nodes,
					factor);
			}
			player_rot = maths::Quaternion::LookAt(player_pos, ahead);
		}
		break;
		}

		player->GetComponent<TransformComponent>()->m_WorldSpaceTransform =  Matrix4::Translation(player_pos) * player_rot.ToMatrix4();
		DebugDrawPath();

		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Interpolation Method:-");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "       [H] Rotation: %s",
			(interpolation_type_rot == 0) ? "Direct-Linear (and why you should never lerp a quaternion!)" : (interpolation_type_rot == 1) ? "Spherical-Linear" : "Look Ahead");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "       [G] Position: %s", (interpolation_type_pos == 0) ? "Linear" : "Cubic/Hermite");

		if (interpolation_type_pos == 1)
		{
			DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "       Tangent Factor: %5.2f [1/2]", cubic_tangent_weighting);

			if (Input::GetInput().GetKeyPressed(JM_KEY_1))
			{
				cubic_tangent_weighting -= timeStep->GetSeconds() * 0.25f;
			}
			if (Input::GetInput().GetKeyPressed(JM_KEY_2))
			{
				cubic_tangent_weighting += timeStep->GetSeconds() * 0.25f;
			}
		}

		if (Input::GetInput().GetKeyPressed(JM_KEY_G))
		{
			interpolation_type_pos = (interpolation_type_pos + 1) % 2;
		}

		if (Input::GetInput().GetKeyPressed(JM_KEY_H))
		{
			interpolation_type_rot = (interpolation_type_rot + 1) % 3;
		}

		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Move the control points around to change the path");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "----------------");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "A little demo to explain and compare linear");
		DebugRenderer::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "and cubic interpolation methods.");

		Scene::OnUpdate(timeStep);
	}

	maths::Vector3 InterpolatePositionLinear(int node_a, int node_b, float factor)
	{
		maths::Vector3 posA = path_nodes[node_a]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
		maths::Vector3 posB = path_nodes[node_b]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();

		return posA * (1.0f - factor) + posB * factor;
	}

	maths::Quaternion InterpolateRotationLinear(int node_a, int node_b, float factor, bool use_proper_slerp)
	{
		maths::Quaternion rotA = path_rotations[node_a];
		maths::Quaternion rotB = path_rotations[node_b];

		if (!use_proper_slerp)
		{
			return maths::Quaternion::Lerp(rotA, rotB, factor);
		}
		else
		{
			return maths::Quaternion::Slerp(rotA, rotB, factor);
		}
	}

	maths::Vector3 InterpolatePositionCubic(int node_a, int node_b, int node_c, int node_d, float f)
	{
		maths::Vector3 posA = path_nodes[node_a]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
		maths::Vector3 posB = path_nodes[node_b]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
		maths::Vector3 posC = path_nodes[node_c]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
		maths::Vector3 posD = path_nodes[node_d]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();

		maths::Vector3 tanB = (posC - posA) * cubic_tangent_weighting;
		maths::Vector3 tanC = (posD - posB) * cubic_tangent_weighting;

		float f2 = f * f;
		float f3 = f2 * f;

		return posB * (f3 * 2.0f - 3.0f * f2 + 1.0f)
			+ tanB * (f3 - 2.0f * f2 + f)
			+ posC * (-2.0f * f3 + 3.0f * f2)
			+ tanC * (f3 - f2);
	}

	void UpdatePathForwardVectors()
	{
		const size_t size = path_nodes.size();

		maths::Vector3 centre = maths::Vector3(0, 0, 0);
		for (uint i = 0; i < size; ++i)
		{
			centre = centre + path_nodes[i]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
		}
		centre = centre / static_cast<float>(size);

		path_rotations.resize(size);
		for (uint i = 0; i < size; ++i)
		{
			maths::Vector3 cur_node_pos = path_nodes[i % size]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();
			maths::Vector3 nxt_node_pos = path_nodes[(i + 1) % size]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();

			path_rotations[i] = maths::Quaternion::LookAt(cur_node_pos, nxt_node_pos, maths::Vector3(0, 1, 0));
		}
	}


	explicit ScenePathFollowing(const std::string& friendly_name)
		: Scene(friendly_name)
		, current_node(0)
		, elapsed_time(0.0f)
		, cubic_tangent_weighting(0.5f)
		, interpolation_type_pos(1)
		, interpolation_type_rot(1)
	{
	}

	~ScenePathFollowing()
	{
	}


	virtual void OnInit() override
	{
		JMPhysicsEngine::Instance()->SetDampingFactor(0.998f);
		JMPhysicsEngine::Instance()->SetIntegrationType(INTEGRATION_RUNGE_KUTTA_4);
		JMPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 5, std::make_shared<SortAndSweepBroadphase>()));
		//JMPhysicsEngine::Instance()->SetBroadphase( new BruteForceBroadphase());
		SetDebugDrawFlags(DEBUGDRAW_FLAGS_COLLISIONVOLUMES
			| DEBUGDRAW_FLAGS_AABB
			// | DEBUGDRAW_FLAGS_BOUNDING_RADIUS 
			| DEBUGDRAW_FLAGS_COLLISIONNORMALS
			| DEBUGDRAW_FLAGS_LINEARVELOCITY 
			// | DEBUGDRAW_FLAGS_BROADPHASE
			| DEBUGDRAW_FLAGS_CONSTRAINT
			// | DEBUGDRAW_FLAGS_BROADPHASE_PAIRS
		);

		Light* sun = new Light();
		sun->SetDirection(maths::Vector3(26.0f, 22.0f, 48.5f));
		sun->SetPosition(maths::Vector3(26.0f, 22.0f, 48.5f) * 10000.0f);
		m_LightSetup->SetDirectionalLight(sun);


		elapsed_time = 0.0f;
		interpolation_type_pos = 1;
		interpolation_type_rot = 1;
		cubic_tangent_weighting = 0.5f;

		//There is always too much ground to cover...
		this->AddEntity(CommonUtils::BuildCuboidObject(
			"Ground",
			maths::Vector3(0.0f, -1.0f, 0.0f),
			maths::Vector3(20.0f, 1.0f, 20.0f),
			false,
			0.0f,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f)));


		//Create some balls to drag around and form our path
		const Vector4 ball_col = Vector4(1.0f, 0.f, 0.f, 1.0f);
		for (int i = 0; i < num_control_points; ++i)
		{
			float angle = (i / static_cast<float>(num_control_points)) * 2.0f * maths::PI;
			float x = cosf(angle) * 6.0f;
			float z = sinf(angle) * 6.0f;

			const std::shared_ptr<Entity> obj = CommonUtils::BuildSphereObject(
				"",
				maths::Vector3(x, (i % 3 == 0) ? 4.0f : 1.0f, z),
				0.3f,
				false,
				0.0f,
				false,
				ball_col);
			path_nodes.push_back(obj);
			this->AddEntity(obj);
		}


		player = CommonUtils::BuildCuboidObject(
			"",
			maths::Vector3(0.0f, 0.5f, 0.0f),
			maths::Vector3(0.2f, 0.2f, 0.5f),
			false,
			0.0f,
			false,
			Vector4(0.5f, 1.0f, 0.8f, 1.0f));
		this->AddEntity(player);

		m_pCamera = new ThirdPersonCamera(-20.0f, -40.0f, maths::Vector3(-3.0f, 10.0f, 15.0f), 45.0f, 0.1f, 1000.0f, m_Application->GetWindow()->GetWidth() / m_Application->GetWindow()->GetHeight());

	}

	virtual void OnCleanupScene() override
	{
		Scene::OnCleanupScene();
		
		if(m_CurrentScene)
			delete m_pCamera;
	
		player.reset();
		path_nodes.clear();
	}

	void DebugDrawPath()
	{
		const size_t num_nodes = path_nodes.size();
		const uint num_samples = 50;
		const Vector4 line_col = Vector4(1.0f, 0.0f, 1.0f, 0.5f);
		for (uint nde = 0; nde < num_nodes; ++nde)
		{
			maths::Vector3 end, start = path_nodes[nde]->GetComponent<TransformComponent>()->m_LocalTransform.GetPositionVector();

			for (uint i = 1; i <= num_samples; ++i)
			{
				float factor = (i / static_cast<float>(num_samples));
				switch (interpolation_type_pos)
				{
				default:
				case 0: //Linear
				{
					end = InterpolatePositionLinear(
						nde,
						(nde + 1) % static_cast<int>(num_nodes),
						factor);
				}
				break;
				case 1: //Cubic
				{
					end = InterpolatePositionCubic(
						(nde + num_nodes - 1) % static_cast<int>(num_nodes),
						nde,
						(nde + 1) % static_cast<int>(num_nodes),
						(nde + 2) % static_cast<int>(num_nodes),
						factor);
				}
				break;
				}

				jm::DebugRenderer::DrawThickLineNDT(start, end, 0.05f, line_col);
				start = end;
			}
		}
	}

	void OnIMGUI() override
	{
		ImGui::Begin(m_SceneName.c_str());
		if(ImGui::Button("<- Back"))
		{
			GetSceneManager()->JumpToScene("SceneSelect");
			ImGui::End();
			return;
		}

		ImGui::End();
	}
};