#pragma once
#include "lmpch.h"
#include "Vector3.h" 
#include "MathsCommon.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT MEM_ALIGN Vector4
		{
		public:
			Vector4()
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_set1_ps(1.0f);
#else
				x = y = z = w = 1.0f;
#endif
			}

			Vector4(float xVal, float yVal, float zVal, float wVal)
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_set_ps(wVal, zVal, yVal, xVal);
#else
                x = xVal; y = yVal; z = zVal; w = wVal;
#endif
			}

			Vector4(float xVal)
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_set_ps(xVal, xVal, xVal, xVal);
#else
				x = y = z = w = xVal;
#endif
			}

#ifdef LUMOS_SSEVEC4
			Vector4(__m128 m) : m_Value(m) { }
#endif

			Vector4(const Vector4 &v)
			{
#ifdef LUMOS_SSEVEC4
				m_Value = v.m_Value;
#else
				x = v.GetX(); y = v.GetY(); z = v.GetZ(); w = v.GetW();
#endif
			}

			Vector4(const Vector3 &v, float wVal = 1.0f)
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_set_ps(wVal, v.GetZ(), v.GetY(), v.GetX());
#else
				x = v.GetX(); y = v.GetY(); z = v.GetZ(); w = wVal;
#endif
			}

#ifdef LUMOS_SSEVEC4
			union
			{
				struct 
				{
					float x;
					float y;
					float z;
					float w;
				};
				__m128 m_Value;
			} MEM_ALIGN;
#else
			float x;
			float y;
			float z;
			float w;
#endif

			float GetX() const { return x; }
			float GetY() const { return y; }
			float GetZ() const { return z; }
			float GetW() const { return w; }

            float* GetPointer() { return &x; }
#ifdef LUMOS_SSEVEC4
			void SetX(const float X) { reinterpret_cast<float *>(&m_Value)[0] = X; }
			void SetY(const float Y) { reinterpret_cast<float *>(&m_Value)[1] = Y; }
			void SetZ(const float Z) { reinterpret_cast<float *>(&m_Value)[2] = Z; }
			void SetW(const float W) { reinterpret_cast<float *>(&m_Value)[3] = W; }
#else
			void SetX(const float X) { x = X; }
			void SetY(const float Y) { y = Y; }
			void SetZ(const float Z) { z = Z; }
			void SetW(const float W) { w = W; }
#endif

			Vector3 ToVector3() const { return Vector3(x, y, z); }

			inline void ToZero()
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_setzero_ps(); 
#else
				x = y = z = w = 0.0f;
#endif
			}

			inline float Length() const
			{
#ifdef LUMOS_SSEVEC4
				return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m_Value, m_Value, 0xF1))); 
#else
				return sqrt((x*x) + (y*y) + (z*z) + (w*w));
#endif
			}

			inline float LengthR() const 
			{ 
#ifdef LUMOS_SSEVEC4
				return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(m_Value, m_Value, 0xF1)));
#else
				return sqrt((x*x) + (y*y) + (z*z) + (w*w));
#endif
			}

			inline float LengthSq() const 
			{ 
#ifdef LUMOS_SSEVEC4
				return _mm_cvtss_f32(_mm_dp_ps(m_Value, m_Value, 0xF1));
#else
				return (x*x) + (y*y) + (z*z) + (w*w);
#endif
			}

			inline void Normalise() 
			{
#ifdef LUMOS_SSEVEC4
				m_Value = _mm_mul_ps(m_Value, _mm_rsqrt_ps(_mm_dp_ps(m_Value, m_Value, 0xFF)));
#else
				float length = Length();

				if (length != 0.0f)
				{
					length = 1.0f / length;
					x = x * length;
					y = y * length;
					z = z * length;
					w = w * length;
				}
#endif
			}

			inline Vector4 Normal()
			{
#ifdef LUMOS_SSEVEC4
				return _mm_mul_ps(m_Value, _mm_rsqrt_ps(_mm_dp_ps(m_Value, m_Value, 0xFF)));
#else
				float length = Length();

				if (length != 0.0f)
				{
					length = 1.0f / length;
					x = x * length;
					y = y * length;
					z = z * length;
					w = w * length;
				}

				return *this;
#endif
			}

			inline float Dot(const Vector4 &v)
			{
#ifdef LUMOS_SSEVEC4
				return _mm_cvtss_f32(_mm_dp_ps(m_Value, v.m_Value, 0xF1));
#else
				return (x*v.x) + (y*v.y) + (z*v.z) + (w*v.w);
#endif
			}

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Vector4);
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

#ifdef LUMOS_SSEVEC4
			inline Vector4 operator+(float v) const { return _mm_add_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector4 operator-(float v) const { return _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector4 operator*(float v) const { return _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
			inline Vector4 operator/(float v) const { return _mm_div_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator+=(float v) { m_Value = _mm_add_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator-=(float v) { m_Value = _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator*=(float v) { m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
			inline void operator/=(float v) { m_Value = _mm_div_ps(m_Value, _mm_set1_ps(v)); }

			inline Vector4 operator+(const Vector4 &v) const { return _mm_add_ps(m_Value, v.m_Value); }
			inline Vector4 operator-(const Vector4 &v) const { return _mm_sub_ps(m_Value, v.m_Value); }
			inline Vector4 operator*(const Vector4 &v) const { return _mm_mul_ps(m_Value, v.m_Value); }
			inline Vector4 operator/(const Vector4 &v) const { return _mm_div_ps(m_Value, v.m_Value); }
			inline void operator+=(const Vector4 &v) { m_Value = _mm_add_ps(m_Value, v.m_Value); }
			inline void operator-=(const Vector4 &v) { m_Value = _mm_sub_ps(m_Value, v.m_Value); }
			inline void operator*=(const Vector4 &v) { m_Value = _mm_mul_ps(m_Value, v.m_Value); }
			inline void operator/=(const Vector4 &v) { m_Value = _mm_div_ps(m_Value, v.m_Value); }

			inline Vector4 operator-() const { return _mm_set_ps(-w, -z, -y, -x); }
			inline bool operator==(const Vector4 &v) const { return _mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) == 0; }
			inline bool operator!=(const Vector4 &v) const { return _mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) != 0; }
#else
			inline Vector4 operator+(float v) const { return Vector4(x + v, y + v, z + v, w + v); }
			inline Vector4 operator-(float v) const { return Vector4(x - v, y - v, z - v, w - v); }
			inline Vector4 operator*(float v) const { return Vector4(x * v, y * v, z * v, w * v); }
			inline Vector4 operator/(float v) const { return Vector4(x / v, y / v, z / v, w / v); }
			inline void operator+=(float v) { x += v; y += v; z += v; w += v; }
			inline void operator-=(float v) { x -= v; y -= v; z -= v; w -= v; }
			inline void operator*=(float v) { x *= v; y *= v; z *= v; w *= v; }
			inline void operator/=(float v) { x /= v; y /= v; z /= v; w /= v; }

			inline Vector4 operator+(const Vector4 &v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
			inline Vector4 operator-(const Vector4 &v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
			inline Vector4 operator*(const Vector4 &v) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
			inline Vector4 operator/(const Vector4 &v) const { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }
			inline void operator+=(const Vector4 &v) { x += v.x; y += v.y; z += v.z; w += v.w; }
			inline void operator-=(const Vector4 &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; }
			inline void operator*=(const Vector4 &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; }
			inline void operator/=(const Vector4 &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; }

			inline Vector4 operator-() const { return Vector4(-x,-y,-z,-w); }
			inline bool operator==(const Vector4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
			inline bool operator!=(const Vector4 &v) const { return (v.x == x && v.y == y && v.z == z && v.w == w) ? false : true;; }
#endif
			friend std::ostream &operator<<(std::ostream &o, const Vector4 &v) { return o << "Vector4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")"; }
		};

		inline Vector4 operator+(float f, const Vector4 &v) { return v + f; }
		inline Vector4 operator*(float f, const Vector4 &v) { return v * f; }

#ifdef LUMOS_SSEVEC4
		inline Vector4 operator-(float f, const Vector4 &v) { return Vector4(_mm_set1_ps(f)) - v; }
		inline Vector4 operator/(float f, const Vector4 &v) { return Vector4(_mm_set1_ps(f)) / v; }
#else
		inline Vector4 operator-(float f, const Vector4 &v) { return Vector4(f) - v; }
		inline Vector4 operator/(float f, const Vector4 &v) { return Vector4(f) / v; }
#endif
	}
}

namespace std 
{
	template<>
	struct hash<Lumos::Maths::Vector4>
	{
		size_t operator()(const Lumos::Maths::Vector4& x) const
		{
			return hash<float>()(x.GetX()) ^ (hash<float>()(x.GetY()) * 997u) ^ (hash<float>()(x.GetZ()) * 999983u) ^ (hash<float>()(x.GetW()) * 999999937);

		}
	};
}
