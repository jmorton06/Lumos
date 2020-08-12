#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"

#include <entt/entity/fwd.hpp>

namespace Lumos
{
	class RigidBody;
	class Scene;

	namespace EntityFactory
	{
		Maths::Vector4 GenColour(float alpha);

		//Generates a default Sphere object with the parameters specified.
		entt::entity BuildSphereObject(
			entt::registry& registry,
			const std::string& name,
			const Maths::Vector3& pos,
			float radius,
			bool physics_enabled = false,
			float inverse_mass = 0.0f, //requires physics_enabled = true
			bool collidable = true, //requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		entt::entity BuildCuboidObject(
			entt::registry& registry,
			const std::string& name,
			const Maths::Vector3& pos,
			const Maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f, //requires physics_enabled = true
			bool collidable = true, //requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		entt::entity BuildPyramidObject(
			entt::registry& registry,
			const std::string& name,
			const Maths::Vector3& pos,
			const Maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f, //requires physics_enabled = true
			bool collidable = true, //requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		void AddLightCube(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir);
		void AddSphere(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir);
		void AddPyramid(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir);
	};
}
