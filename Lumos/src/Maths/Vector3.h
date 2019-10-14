#pragma once
#include "lmpch.h"
#include "Vector2.h"
#include "MathsCommon.h"
#include "MathsUtilities.h"

#include "Core/Serialisable.h"

namespace Lumos 
{
	namespace Maths 
	{
		class LUMOS_EXPORT MEM_ALIGN Vector3
		{
		public:

			Vector3()
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_set1_ps(0.0f);
#else
				x = y = z = 0.0f;
#endif
			}

			Vector3(const float xVal, const float yVal, const float zVal)
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_set_ps(0, zVal, yVal, xVal);
#else
				x = xVal; y = yVal; z = zVal;
#endif
			}

			explicit Vector3(const float value) 
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_set_ps(0, value, value, value);
#else
				x = y = z = value;
#endif
			}

			Vector3(const Vector2 &vec2, float zVal)
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_set_ps(0, zVal, vec2.GetY(), vec2.GetX());
#else
				x = vec2.GetX(); y = vec2.GetY(); z = zVal;
#endif
			}

#ifdef LUMOS_SSEVEC3
			Vector3(const __m128 m) : m_Value(m) {}
#endif

			~Vector3() {
			}

			static Vector3 Zero() { return Vector3(); };

			static Vector3 ZAxis() { return Vector3(0.0f, 0.0f, 1.0f); };

#ifdef LUMOS_SSEVEC3
			union 
			{
				struct
				{
					float x;
					float y;
					float z;
				};
				__m128 m_Value;
			} MEM_ALIGN;
#else
			float x;
			float y;
			float z;
#endif

			float GetX() const { return x; }
			float GetY() const { return y; }
			float GetZ() const { return z; }

#ifdef LUMOS_SSEVEC3
			void SetX(const float X) { reinterpret_cast<float *>(&m_Value)[0] = X; }
			void SetY(const float Y) { reinterpret_cast<float *>(&m_Value)[1] = Y; }
			void SetZ(const float Z) { reinterpret_cast<float *>(&m_Value)[2] = Z; }
#else
			void SetX(const float X) { x = X; }
			void SetY(const float Y) { y = Y; }
			void SetZ(const float Z) { z = Z; }
#endif

			void ToZero() 
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_setzero_ps();
#else
				x = y = z = 0.0f;
#endif
			}

			static inline float Sqrt(float x)
			{
#ifdef LUMOS_SSEVEC3
				__m128 v = _mm_set_ss(x);
				v = _mm_sqrt_ss(v);
				return _mm_cvtss_f32(v);
#else
				return sqrt(x);
#endif
			}

			float Length() const 
			{
				return Sqrt(Dot(*this, *this));
			}

			float LengthSquared() const 
			{
#ifdef LUMOS_SSEVEC3
				return _mm_cvtss_f32(_mm_dp_ps(m_Value, m_Value, 0x71));
#else
				return (x*x + y * y + z * z);
#endif
			}

			Vector3 Inverse() const
			{
				return Vector3(-GetX(), -GetY(), -GetZ());
			}

			void Invert() 
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(-1.0f));
#else
				x = -x; y = -y; z = -z;
#endif
			}

			void Normalise()
			{
#ifdef LUMOS_SSEVEC3
				m_Value = _mm_mul_ps(m_Value, _mm_rsqrt_ps(_mm_dp_ps(m_Value, m_Value, 0x77)));
#else
				float length = Length();
				if (length != 0.0f)
				{
					length = 1.0f / length;
					x *= length;
					y *= length;
					z *= length;
				}
#endif
			}

			inline Vector3 Normal() 
			{
				Normalise();
				return Vector3(*this);
			}

			inline const Vector3 Normal() const
			{
				Vector3 normal(*this);
				normal.Normalise();
				return normal;
			}

			static float Dot(const Vector3 &a, const Vector3 &b)
			{
#ifdef LUMOS_SSEVEC3
				return _mm_cvtss_f32(_mm_dp_ps(a.m_Value, b.m_Value, 0x7f));
#else
				return (a.x*b.x) + (a.y*b.y) + (a.z*b.z);
#endif
			}

			float Dot(const Vector3 &other) const
			{
				return Dot(*this, other);
			}

			static Vector3 Cross(const Vector3 &a, const Vector3 &b)
			{
#ifdef LUMOS_SSEVEC3
				return _mm_sub_ps(
					_mm_mul_ps(_mm_shuffle_ps(a.m_Value, a.m_Value, _MM_SHUFFLE(3, 0, 2, 1)),
						_mm_shuffle_ps(b.m_Value, b.m_Value, _MM_SHUFFLE(3, 1, 0, 2))),
					_mm_mul_ps(_mm_shuffle_ps(a.m_Value, a.m_Value, _MM_SHUFFLE(3, 1, 0, 2)),
						_mm_shuffle_ps(b.m_Value, b.m_Value, _MM_SHUFFLE(3, 0, 2, 1))));
#else
				return Vector3((a.y*b.z) - (a.z*b.y), (a.z*b.x) - (a.x*b.z), (a.x*b.y) - (a.y*b.x));
#endif
			}

			Vector3 Cross(const Vector3 &other) const
			{
				return Cross(*this, other);
			}

			inline friend std::ostream &operator<<(std::ostream &o, const Vector3 &v)
			{
				o << "Vector3(" << v.GetX() << "," << v.GetY() << "," << v.GetZ() << ")" << std::endl;
				return o;
			}

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Vector3);
				output["x"] = x;
				output["y"] = y;
				output["z"] = z;

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				x = data["x"];
				y = data["y"];
				z = data["z"];
			};
            
            bool Equals(const Vector3& rhs) const
            {
                return Maths::Equals(x, rhs.x) && Maths::Equals(y, rhs.y) && Maths::Equals(z, rhs.z);
            }

#ifdef LUMOS_SSEVEC3
			inline Vector3 operator+(float v) const { return _mm_add_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector3 operator-(float v) const { return _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector3 operator*(float v) const { return _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector3 operator/(float v) const { return _mm_div_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator+=(float v) { m_Value = _mm_add_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator-=(float v) { m_Value = _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator*=(float v) { m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator/=(float v) { m_Value = _mm_div_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector3 operator-() const { return _mm_set_ps(0, -GetZ(), -GetY(), -GetX()); }


			inline Vector3 operator+(const Vector3 &v) const { return _mm_add_ps(m_Value, v.m_Value); }
			inline Vector3 operator-(const Vector3 &v) const { return _mm_sub_ps(m_Value, v.m_Value); }
			inline Vector3 operator*(const Vector3 &v) const { return _mm_mul_ps(m_Value, v.m_Value); }
			inline Vector3 operator/(const Vector3 &v) const { return _mm_div_ps(m_Value, v.m_Value); }
			inline void operator+=(const Vector3 &v) { m_Value = _mm_add_ps(m_Value, v.m_Value); }
			inline void operator-=(const Vector3 &v) { m_Value = _mm_sub_ps(m_Value, v.m_Value); }
			inline void operator*=(const Vector3 &v) { m_Value = _mm_mul_ps(m_Value, v.m_Value); }
			inline void operator/=(const Vector3 &v) { m_Value = _mm_div_ps(m_Value, v.m_Value); }
#else
			inline Vector3 operator+(const float v) const { return Vector3(x + v, y + v, z + v); }
			inline Vector3 operator-(const float v) const { return Vector3(x - v, y - v, z - v); }
			inline Vector3 operator*(const float v) const { return Vector3(x * v, y * v, z * v); }
			inline Vector3 operator/(const float v) const { return Vector3(x / v, y / v, z / v); };
			inline void operator+=(const float v) { x += v; y += v; z += v; }
			inline void operator-=(const float v) { x -= v; y -= v; z -= v; }
			inline void operator*=(const float v) { x *= v; y *= v; z *= v; }
			inline void operator/=(const float v) { x /= v; y /= v; z /= v; }
			inline Vector3  operator-() const { return Vector3(-x, -y, -z); }

			inline Vector3 operator+(const Vector3  &v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
			inline Vector3 operator-(const Vector3  &v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
			inline Vector3 operator*(const Vector3  &v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
			inline Vector3 operator/(const Vector3  &v) const { return Vector3(x / v.x, y / v.y, z / v.z); };
			inline void operator+=(const Vector3  &v) { x += v.x; y += v.y; z += v.z; }
			inline void operator-=(const Vector3  &v) { x -= v.x; y -= v.y; z -= v.z; }
			inline void operator*=(const Vector3  &v) { x *= v.x; y *= v.y; z *= v.z; }
			inline void operator/=(const Vector3  &v) { x /= v.x; y /= v.y; z /= v.z; }
#endif

			inline bool operator<(const Vector3 &other)	 const { return GetX() < other.GetX() && GetY() < other.GetY() && GetZ() < other.GetZ(); }
			inline bool operator<=(const Vector3 &other) const { return GetX() <= other.GetX() && GetY() <= other.GetY() && GetZ() <= other.GetZ(); }
			inline bool operator>(const Vector3 &other)  const { return GetX() > other.GetX() && GetY() > other.GetY() && GetZ() > other.GetZ(); }
			inline bool operator>=(const Vector3 &other) const { return GetX() >= other.GetX() && GetY() >= other.GetY() && GetZ() >= other.GetZ(); }


			inline bool operator==(const Vector3 &v) const
			{
#ifdef LUMOS_SSEVEC3
				return (_mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) & 0x7) == 0;
#else
				return (v.x == x && v.y == y && v.z == z) ? true : false;
#endif
			}

			inline bool operator!=(const Vector3 &v) const
			{
#ifdef LUMOS_SSEVEC3
				return (_mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) & 0x7) != 0;
#else
				return (v.x == x && v.y == y && v.z == z) ? false : true;
#endif
			}

			inline float operator[](int i) const
			{
				switch (i)
				{
					case 0:
						return GetX();
					case 1:
						return GetY();
					case 2:
						return GetZ();
					default:
						return 0.0f;
				}
			}

			static inline Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t)
			{
				return a * (1.0f - t) + b * t;
			}
		};
    inline Vector3 operator *(float lhs, const Vector3& rhs) { return rhs * lhs; }
	}
}

namespace std 
{
	template<>
	struct hash<Lumos::Maths::Vector3>
	{
		size_t operator()(const Lumos::Maths::Vector3& x) const
		{
			return hash<float>()(x.GetX()) ^ (hash<float>()(x.GetY()) * 997u) ^ (hash<float>()(x.GetZ()) * 999983u);

		}
	};
}
