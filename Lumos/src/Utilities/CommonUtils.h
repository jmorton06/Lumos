#pragma once
#include "LM.h"
#include "Maths/Vector4.h"

namespace lumos
{
	class PhysicsObject;
	class Scene;
	class Entity;

	namespace CommonUtils
	{
		maths::Vector4 GenColour(float alpha);

		//Generates a default Sphere object with the parameters specified.
		std::shared_ptr<Entity> BuildSphereObject(
			const std::string& name,
			const maths::Vector3& pos,
			float radius,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const maths::Vector4& color = maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		std::shared_ptr<Entity> BuildCuboidObject(
			const std::string& name,
			const maths::Vector3& pos,
			const maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const maths::Vector4& color = maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		std::shared_ptr<Entity> BuildPyramidObject(
			const std::string& name,
			const maths::Vector3& pos,
			const maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const maths::Vector4& color = maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));


		void AddLightCube(Scene* scene);
		void AddSphere(Scene* scene);
		void AddPyramid(Scene* scene);
	};
}