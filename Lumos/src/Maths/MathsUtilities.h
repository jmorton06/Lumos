#pragma once
#include "LM.h"

namespace Lumos
{
	namespace maths
	{
		template <class T> const T& Max(const T& a, const T& b)
		{
			return (a < b) ? b : a;
		}

		template <class T> const T& Min(const T& a, const T& b)
		{
			return !(b < a) ? a : b;
		}

		template <typename T> T Clamp(const T& value, const T& low, const T& high)
		{
			return value < low ? low : (value > high ? high : value);
		}

		template <typename T> T Squared(T v)
		{
			return v * v;
		}

		inline float Abs(float x)
		{
			return x >= 0 ? x : -x;
		}

		inline int Abs(int x)
		{
			return x >= 0 ? x : -x;
		}

		static constexpr  float		PI = 3.14159265358979323846f;
		static constexpr  float		PI_OVER_360 = PI / 360.0f;

		//Radians to degrees
		static constexpr  double RadToDeg(const double deg)
		{
			return deg * 180.0 / PI;
		};

		static constexpr  float RadToDeg(const float deg)
		{
			return deg * 180.0f / PI;
		};

		//Degrees to radians
		static constexpr double DegToRad(const double rad)
		{
			return rad * PI / 180.0;
		};

		static constexpr  float DegToRad(const float rad)
		{
			return rad * PI / 180.0f;
		};

		static constexpr  float Lerp(const float a, const float b, const float t)
		{
			return	a * (1.0f - t) + b * t;
		}
        
        static const std::string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        
        inline String GenerateUUID()
        {
            String uuid = std::string(36,' ');
            int rnd = 0;
            
            uuid[8] = '-';
            uuid[13] = '-';
            uuid[18] = '-';
            uuid[23] = '-';
            
            uuid[14] = '4';
            
            for(int i = 0; i < 36; i++)
            {
                if (i != 8 && i != 13 && i != 18 && i != 14 && i != 23)
                {
                    if (rnd <= 0x02)
                    {
                        rnd = 0x2000000 + (std::rand() * 0x1000000) | 0;
                    }
                    rnd >>= 4;
                    uuid[i] = CHARS[(i == 19) ? ((rnd & 0xf) & 0x3) | 0x8 : rnd & 0xf];
                }
            }
            return uuid;
        }
	}
}
