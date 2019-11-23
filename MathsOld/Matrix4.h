#pragma once
#include "lmpch.h"
#include "Vector4.h"
#include "MathsCommon.h"
#include "Core/Serialisable.h"

#define CLIP_CONTROL_ZO_BIT		(1 << 0) // ZERO_TO_ONE
#define CLIP_CONTROL_NO_BIT		(1 << 1) // NEGATIVE_ONE_TO_ONE
#define CLIP_CONTROL_LH_BIT		(1 << 2) // LEFT_HANDED, For DirectX, Metal, Vulkan
#define CLIP_CONTROL_RH_BIT		(1 << 3) // RIGHT_HANDED, For OpenGL, default

#define CLIP_CONTROL_LH_ZO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_LH_NO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_NO_BIT)
#define CLIP_CONTROL_RH_ZO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_RH_NO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_NO_BIT)

namespace Lumos
{
	namespace LMMaths
	{
		class Vector3;
		class Matrix3;
        class Quaternion;

		class LUMOS_EXPORT MEM_ALIGN Matrix4
		{
		public:

            static int CONFIG_CLIP_CONTROL;
            static void SetUpCoordSystem(bool LeftHanded, bool forceZeroToOne);

            Matrix4()
			{
				ToIdentity();
			}

			inline Matrix4(const float elements[16])
			{
				memcpy(values, elements, 16 * sizeof(float));
			}

			Matrix4(const Matrix3 &mat);
			Matrix4(const Matrix4 &mat);

#ifdef LUMOS_SSEMAT4
			struct
            {
				union
                {
					float values[16];
					__m128 mmvalues[4];
				};
			} MEM_ALIGN;
#else
            float values[16];
#endif
            void ToZero();
            void ToIdentity();

			inline float Get(unsigned int row, unsigned int column) const
			{
				return values[row + column * 4];
			}

			inline Vector4 GetRow(unsigned int row) const
			{
				return Vector4(values[row], values[row + 4], values[row + 8], values[row + 12]);
			}

			inline Vector4 Column(unsigned int column) const
			{
#if defined(LUMOS_SSEVEC4) && defined(LUMOS_SSEMAT4)
				return Vector4(mmvalues[column]);
#else
				return Vector4();// mmvalues[column]);
#endif
			}

			inline void SetRow(unsigned int row, const Vector4 &val)
			{
				values[row] = val.x;
				values[row + 4] = val.y;
				values[row + 8] = val.z;
				values[row + 12] = val.w;
			}

			inline void SetCol(unsigned int column, const Vector4 &val)
            {
#if defined(LUMOS_SSEVEC4) && defined(LUMOS_SSEMAT4)
				mmvalues[column] = val.m_Value;
#endif
			}

			inline Vector3 Translation() const
			{
				return Vector3(values[12], values[13], values[14]);
			}

#if defined(LUMOS_SSEVEC3) && defined(LUMOS_SSEMAT4)
			inline Vector3 GetTranslationMemAligned() const
			{
				return Vector3(mmvalues[3]);
			}
#endif

			inline void SetPositionVector(const Vector3 &v)
			{
				values[12] = v.x;
				values[13] = v.y;
				values[14] = v.z;
			}

			inline Vector3 GetScaling() const 
			{ 
				//return Vector3(values[0], values[5], values[10]);
				return Vector3(
					sqrtf(values[0] * values[0] + values[4] * values[4] + values[8] * values[8]),
					sqrtf(values[1] * values[1] + values[5] * values[5] + values[9] * values[9]),
					sqrtf(values[2] * values[2] + values[6] * values[6] + values[10] * values[10])
				);
			}

			inline void SetScaling(const Vector3 &in)
			{
				values[0] = in.x;
				values[5] = in.y;
				values[10] = in.z;
			}

			Matrix4 operator*(const Matrix4 &m) const;
			Vector3 operator*(const Vector3 &v) const;
			Vector4 operator*(const Vector4 &v) const;
            Matrix4 operator*(float rhs) const;

			bool operator==(const Matrix4 &m) const;

			void Transpose();
            Matrix4 GetRotation() const;
            
            Quaternion ToQuaternion() const;

			static Vector3 GetEulerAngles(const Matrix4 &mat);
			static Matrix4 RotationX(float degrees);
			static Matrix4 RotationY(float degrees);
			static Matrix4 RotationZ(float degrees);
			static Matrix4 Rotation(float degrees, const Vector3 &axis);
			static Matrix4 Rotation(float degreesX, float degreesY, float degreesZ);
			static Matrix4 Scale(const Vector3 &scale);
			static Matrix4 Translation(const Vector3 &translation);
            static Matrix4 Inverse(const Matrix4 &inM);
			static Matrix4 Perspective(float znear, float zfar, float aspect, float fov);
			static Matrix4 Orthographic(float left, float right, float bottom, float top, float znear, float zfar);
			static Matrix4 BuildViewMatrix(const Vector3 &from, const Vector3 &lookingAt,
										   const Vector3 &up = Vector3(0.0f, 1.0f, 0.0f));

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Matrix4);

				nlohmann::json data = nlohmann::json::array_t();

				for (int i = 0; i < 16; i++)
				{
					data.push_back(values[i]);
				}

				output["values"] = data;

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				nlohmann::json::array_t dataArray = data["values"];

				for (int i = 0; i < 16; i++)
				{
					values[i] = dataArray[i];
				}
			};

			friend std::ostream &operator<<(std::ostream &o, const Matrix4 &m)
			{
				return o << "Mat4(" << "/n" <<
					"\t" << m.values[0] << ", " << m.values[4] << ", " << m.values[8] << ", " << m.values[12] << ", "
					<< "/n" <<
					"\t" << m.values[1] << ", " << m.values[5] << ", " << m.values[9] << ", " << m.values[13] << ", "
					<< "/n" <<
					"\t" << m.values[2] << ", " << m.values[6] << ", " << m.values[10] << ", " << m.values[14] << ", "
					<< "/n" <<
					"\t" << m.values[3] << ", " << m.values[7] << ", " << m.values[11] << ", " << m.values[15] << "/n"
					<<
					" )";
			}

			inline float operator[](const int index) const
            {
				return values[index];
			}

			inline float &operator[](const int index)
            {
				return values[index];
			}
		};
        
        inline Matrix4 operator *(float lhs, const Matrix4& rhs) { return rhs * lhs; }
	}
}
