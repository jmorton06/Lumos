#pragma once

#include "LM.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "MathsCommon.h"

namespace Lumos
{
	namespace maths
	{

		class Matrix4;
#ifdef LUMOS_SSEQUAT

		class LUMOS_EXPORT MEM_ALIGN Quaternion
		{
		public:
			Quaternion() : mmvalue(_mm_set_ps(1.0f, 0, 0, 0)) {}
			Quaternion(const Vector3& vec, float w) : mmvalue(_mm_set_ps(w, vec.GetX(), vec.GetY(), vec.GetZ())) {}
			Quaternion(float x, float y, float z, float w) : mmvalue(_mm_set_ps(w, z, y, x)) {}
			Quaternion(__m128 m) : mmvalue(m) {}
			Quaternion(const Quaternion& v) : mmvalue(v.mmvalue) {}

			union
			{
				struct
				{
					float x, y, z, w;
				};
				__m128 mmvalue;
			} MEM_ALIGN;

			inline void Normalise() { mmvalue = _mm_mul_ps(mmvalue, _mm_rsqrt_ps(_mm_dp_ps(mmvalue, mmvalue, 0xFF))); }
			inline Quaternion Normal() const { return _mm_mul_ps(mmvalue, _mm_rsqrt_ps(_mm_dp_ps(mmvalue, mmvalue, 0xFF))); }

			Matrix4 ToMatrix4() const;
			Matrix3 ToMatrix3() const;

			Quaternion Inverse() const;
			Quaternion Conjugate() const { return Quaternion(-x, -y, -z, w); }
			float Magnitude() const;

			void GenerateW();	//builds 4th component when loading in shortened, 3 component quaternions

			static const Quaternion EMPTY;
			static const Quaternion IDENTITY;

			static Quaternion EulerAnglesToQuaternion(float pitch, float yaw, float roll);
			static Quaternion AxisAngleToQuaterion(const Vector3& vector, float degrees);

			static void RotatePointByQuaternion(const Quaternion& q, Vector3& point);

			static Quaternion FromMatrix(const Matrix4& m);

			static Quaternion LookAt(const Vector3& from, const Vector3& to, const Vector3& up = Vector3(0, 1, 0));
			static Quaternion GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up = Vector3(0, 1, 0));

			Vector3 Transform(const Vector3& point) const;

			inline float Dot(const Quaternion& q) const { return _mm_cvtss_f32(_mm_dp_ps(mmvalue, q.mmvalue, 0xF1)); }

			static float Dot(const Quaternion &a, const Quaternion &b);

			//http://stackoverflow.com/questions/18542894/how-to-multiply-two-quaternions-with-minimal-instructions
			inline Quaternion operator*(const Quaternion& q) const
			{
				__m128 wzyx = _mm_shuffle_ps(mmvalue, mmvalue, _MM_SHUFFLE(0, 1, 2, 3));
				__m128 baba = _mm_shuffle_ps(q.mmvalue, q.mmvalue, _MM_SHUFFLE(0, 1, 0, 1));
				__m128 dcdc = _mm_shuffle_ps(q.mmvalue, q.mmvalue, _MM_SHUFFLE(2, 3, 2, 3));
				__m128 ZnXWY = _mm_hsub_ps(_mm_mul_ps(mmvalue, baba), _mm_mul_ps(wzyx, dcdc));

				__m128 XZYnW = _mm_hadd_ps(_mm_mul_ps(mmvalue, dcdc), _mm_mul_ps(wzyx, baba));
				__m128 XZWY = _mm_addsub_ps(_mm_shuffle_ps(XZYnW, ZnXWY, _MM_SHUFFLE(3, 2, 1, 0)), _mm_shuffle_ps(ZnXWY, XZYnW, _MM_SHUFFLE(2, 3, 0, 1)));

				return _mm_shuffle_ps(XZWY, XZWY, _MM_SHUFFLE(2, 1, 3, 0));
			}

			inline Quaternion operator*(const Vector3& v) const
			{
				return Quaternion(
					(w * v.GetX()) + (v.GetY() * z) - (v.GetZ() * y),
					(w * v.GetY()) + (v.GetZ() * x) - (v.GetX() * z),
					(w * v.GetZ()) + (v.GetX() * y) - (v.GetY() * x),
					-(x * v.GetX()) - (y * v.GetY()) - (z * v.GetZ())
				);
			}

			inline Quaternion operator+(const Quaternion& a) const
			{
				return _mm_add_ps(mmvalue, a.mmvalue);
			}

			static Quaternion Lerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
			static Quaternion Slerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
			Quaternion Interpolate(const Quaternion& pStart, const Quaternion& pEnd, float pFactor) const;


			MEM_ALIGN_NEW_DELETE

				inline friend std::ostream& operator<<(std::ostream& o, const Quaternion& q) {
				return o << "Quat(" << q.x << "," << q.y << "," << q.z << "," << q.w << ")" << std::endl;
			}
		};
#else
		class LUMOS_EXPORT Quaternion
		{
		public:
			Quaternion(void);
			Quaternion(float x, float y, float z, float w);
			Quaternion(const Vector3& xyz, float w);

			~Quaternion(void);

			float x;
			float y;
			float z;
			float w;

			void Normalise();

			Matrix4 ToMatrix4() const;
			Matrix3 ToMatrix3() const;

			Quaternion Conjugate() const;
			Quaternion Inverse() const;
			float Magnitude() const;

			void GenerateW(); // builds 4th component when loading in shortened, 3 component quaternions - great for network compression ;)

			static Quaternion EulerAnglesToQuaternion(float pitch, float yaw, float roll);
			static Quaternion AxisAngleToQuaterion(const Vector3 &vector, float degrees);

			static void RotatePointByQuaternion(const Quaternion &q, Vector3 &point);

			static Quaternion FromMatrix(const Matrix4 &m);
			static Quaternion FromVectors(const Vector3 &v1, const Vector3 &v2);

			static Quaternion LookAt(const Vector3& from, const Vector3& to, const Vector3& up = Vector3(0, 1, 0));
			static Quaternion GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up = Vector3(0, 1, 0));

			static float Dot(const Quaternion &a, const Quaternion &b);
			static Vector3 Rotate(const Quaternion& quat, const Vector3 & vec);

			Vector3 Transform(const Vector3& point) const;

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

			Quaternion operator*(const Quaternion &a) const;
			Quaternion operator*(const Vector3 &a) const;

			Quaternion operator+(const Quaternion &a) const
			{
				return Quaternion(x + a.x, y + a.y, z + a.z, w + a.w);
			}

			Quaternion Interpolate(const Quaternion &pStart, const Quaternion &pEnd, float pFactor) const;

			static Quaternion Lerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
			static Quaternion Slerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);

			friend std::ostream &operator<<(std::ostream &o, const Quaternion &q);
		};

		std::istream &operator>>(std::istream &stream, Quaternion &q);

#endif
	}
}
