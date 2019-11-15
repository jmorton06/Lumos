#pragma once

#include "lmpch.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "MathsCommon.h"
#include "Core/Serialisable.h"

namespace Lumos
{
	namespace Maths
	{

		class Matrix4;

		class LUMOS_EXPORT MEM_ALIGN Quaternion
		{
		public:
			Quaternion();
			Quaternion(const Vector3& vec, float w);
			Quaternion(float lx, float ly, float lz, float lw);
			Quaternion(const Quaternion& v);
            
            ///Construct from Euler (Degrees)
            Quaternion(float pitch, float yaw, float roll);
            ///Construct from Euler (Degrees)
            Quaternion(const Vector3& v);

#ifdef LUMOS_SSEQUAT
			Quaternion(__m128 m);
#endif

#ifdef LUMOS_SSEQUAT
			union
			{
				struct
				{
                    float w;
                    float x;
                    float y;
                    float z;
				};
				__m128 mmvalue;
			} MEM_ALIGN;
#else
			float x;
			float y;
			float z;
			float w;
#endif

			void Normalize();
			Quaternion Normal() const;

			Matrix4 ToMatrix4() const;
			Matrix3 ToMatrix3() const;

			Quaternion Inverse() const;
			Quaternion Conjugate() const;
			float Magnitude() const;
			float LengthSquared() const;

			void GenerateW();	//builds 4th component when loading in shortened, 3 component quaternions
			Vector3 ToEuler() const;
			void FromEulerAngles(float pitch, float yaw, float roll);

			Vector3 GetXAxis() const
			{
				return Vector3(1.0f - 2.0f * (y*y + z * z),
					2.0f * (y*x + z * w),
					2.0f * (x*z - y * w));
			}

			Vector3 GetYAxis() const
			{
				return Vector3(2.0f * (x*y - w * z),
					1.0f - 2.0f * (x*x + z * z),
					2.0f * (y*z + w * x));
			}

			Vector3 GetZAxis() const
			{
				return Vector3(2.0f * (x*z + w * y),
					2.0f * (y*z - w * x),
					1.0f - 2.0f * (x*x + y * y));
			}

			void ToAxes(Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) const
			{
				Matrix3 mat = ToMatrix3();

				xAxis = mat.Column(0);
				yAxis = mat.Column(1);
				zAxis = mat.Column(2);
			}

			static Quaternion EulerAnglesToQuaternion(float pitch, float yaw, float roll);
			static Quaternion AxisAngleToQuaterion(const Vector3& vector, float degrees);

			static void RotatePointByQuaternion(const Quaternion& quat, Vector3& point);

			static Quaternion FromMatrix(const Matrix4& matrix);
			static Quaternion FromVectors(const Vector3 &v1, const Vector3 &v2);

			static Quaternion LookAt(const Vector3& from, const Vector3& to, const Vector3& up = Vector3(0, 1, 0));
			static Quaternion GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up = Vector3(0, 1, 0));
			static Quaternion Rotation(const Vector3& unitVec0, const Vector3& unitVec1);
			static Quaternion Rotation(float radians, const Vector3& unitVec);

			static Quaternion RotationX(float radians)
			{
				float angle = radians * 0.5f;
				return Quaternion(sin(angle), 0.0f, 0.0f, cos(angle));
			}

			static Quaternion RotationY(float radians)
			{
				float angle = radians * 0.5f;
				return Quaternion(0.0f, sin(angle), 0.0f, cos(angle));
			}

			static Quaternion RotationZ(float radians)
			{
				float angle = radians * 0.5f;
				return Quaternion(0.0f, 0.0f, sin(angle), cos(angle));
			}

			Vector3 Transform(const Vector3& point) const;

			float Dot(const Quaternion& q) const;
			static float Dot(const Quaternion &a, const Quaternion &b);
			static Quaternion Lerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
			static Quaternion Slerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
			Quaternion Interpolate(const Quaternion& pStart, const Quaternion& pEnd, float pFactor) const;

			Quaternion operator*(const Quaternion& q) const;
			Quaternion operator*(const Vector3& rhs) const;
			Quaternion operator+(const Quaternion& a) const;
            Quaternion operator*(float rhs) const;
            Quaternion operator-() const;
            Quaternion operator-(const Quaternion& rhs) const;
            
            Quaternion& operator=(const Quaternion& rhs) noexcept;
            Quaternion& operator+=(const Quaternion& rhs);
            Quaternion& operator*=(float rhs);
            bool operator==(const Quaternion& rhs) const;
            bool operator!=(const Quaternion& rhs) const;

			bool Equals(const Quaternion& rhs) const
			{
				return Maths::Equals(x, rhs.x) && Maths::Equals(y, rhs.y) && Maths::Equals(z, rhs.z) && Maths::Equals(w, rhs.w);
			}
            
            nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Quaternion);
				output["x"] = x;
				output["y"] = y;
				output["z"] = z;
				output["w"] = w;

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				x = data["x"];
				y = data["y"];
				z = data["z"];
				w = data["w"];
			};
				
			friend std::ostream& operator<<(std::ostream& o, const Quaternion& q) 
			{
				return o << "Quat(" << q.x << "," << q.y << "," << q.z << "," << q.w << ")" << std::endl;
			}
		};
	}
}
