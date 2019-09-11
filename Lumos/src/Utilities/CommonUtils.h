#pragma once
#include "lmpch.h"
#include "Maths/Vector4.h"

namespace Lumos
{
	class PhysicsObject;
	class Scene;
	class Entity;

	namespace CommonUtils
	{
		Maths::Vector4 GenColour(float alpha);

		//Generates a default Sphere object with the parameters specified.
		Entity* BuildSphereObject(
			const std::string& name,
			const Maths::Vector3& pos,
			float radius,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		Entity* BuildCuboidObject(
			const std::string& name,
			const Maths::Vector3& pos,
			const Maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Generates a default Cuboid object with the parameters specified
		Entity* BuildPyramidObject(
			const std::string& name,
			const Maths::Vector3& pos,
			const Maths::Vector3& scale,
			bool physics_enabled = false,
			float inverse_mass = 0.0f,			//requires physics_enabled = true
			bool collidable = true,				//requires physics_enabled = true
			const Maths::Vector4& color = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));


		void AddLightCube(Scene* scene);
		void AddSphere(Scene* scene);
		void AddPyramid(Scene* scene);
	};
}