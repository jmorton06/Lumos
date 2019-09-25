#pragma once

#include "lmpch.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Plane.h"
#include "Quaternion.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace Lumos
{
	namespace Maths
	{
		struct Vector2Simple
		{
			float x, y;
		};

		struct Vector3Simple
		{
			Vector3Simple(float x, float y, float z) : x(x), y(y), z(z) { };
			float x, y, z;
		};

		struct Vector4Simple
		{
			float x, y, z, w;
		};

		inline Vector2 ToVector(const Vector2Simple& vec)
		{
			return Vector2(vec.x, vec.y);
		}

		inline Vector3 ToVector(const Vector3Simple& vec)
		{
			return Vector3(vec.x, vec.y, vec.z);
		}

		inline Vector3Simple ToVector(const Vector3& vec)
		{
			return Vector3Simple(vec.GetX(), vec.GetY(), vec.GetZ());
		}

		inline Vector4 ToVector(const Vector4Simple& vec)
		{
			return Vector4(vec.x, vec.y, vec.z, vec.w);
		}

		inline Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4& mvp, float width, float height, float winPosX = 0.0f, float winPosY = 0.0f)
		{
			Vector4 trans = mvp * Vector4(worldPos);
			trans *= 0.5f / trans.GetW();
			trans += Vector4(0.5f, 0.5f, 0.0f, 0.0f);
			trans.y = 1.f - trans.y;
			trans.x *= width;
			trans.y *= height;
			trans.x += winPosX;
			trans.y += winPosY;
			return Vector2(trans.x, trans.y);
		}
	}
}