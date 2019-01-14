#pragma once
#include "LM.h"
#include "MathsCommon.h"

namespace Lumos
{
	namespace maths
	{
#ifdef LUMOS_SSEVEC2

		class LUMOS_EXPORT MEM_ALIGN Vector2
#else
		class LUMOS_EXPORT Vector2
#endif
		{
		public:
			Vector2() { ToZero(); }
			Vector2(const float x, const float y) : x(x), y(y) {}
			Vector2(const float x) : x(x), y(x) { }
			~Vector2() {}

		private:
#ifdef LUMOS_SSEVEC2
			union 
			{
				struct
				{
					float x, y;
				};
				__m128 mmvalue;
			} MEM_ALIGN;
#else
			float x, y;
#endif

		public:

			float GetX() const { return x; }
			float GetY() const { return y; }
			void SetX(const float X) { x = X; }
			void SetY(const float Y) { y = Y; }

			void ToZero() { x = 0.0f; y = 0.0f; }

			inline void Normalise() 
			{
				float length = Length();

				if (length != 0.0f) 
				{
					length = 1.0f / length;
					x = x * length;
					y = y * length;
				}
			}

			inline float Length() const { return sqrt((x * x) + (y * y)); }

			inline friend std::ostream &operator<<(std::ostream &o, const Vector2 &v) 
			{
				o << "Vector2(" << v.x << "," << v.y << ")" << std::endl;
				return o;
			}

			inline Vector2 operator-(const Vector2 &a) const { return Vector2(x - a.x, y - a.y); }
			inline Vector2 operator+(const Vector2 &a) const { return Vector2(x + a.x, y + a.y); }
			inline Vector2 operator*(const float a) const { return Vector2(x * a, y * a); }
			inline Vector2 operator/(const float v) const { return Vector2(x / v, y / v); };
			inline bool operator==(const Vector2 &A) const { return (A.x == x && A.y == y) ? true : false; };

#ifdef LUMOS_SSEVEC2
			MEM_ALIGN_NEW_DELETE;
#endif

		};
	}
}
namespace std 
{
	template<>
	struct hash<Lumos::maths::Vector2>
	{
		size_t operator()(const Lumos::maths::Vector2& x) const
		{
			return hash<float>()(x.GetX()) ^ (hash<float>()(x.GetY()) * 997u);
		}
	};
}
