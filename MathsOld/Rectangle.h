#pragma once
#include "lmpch.h"
#include "Core/Serialisable.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT Rectangle
		{
		public:

			int x, y, width, height;

			Rectangle();
			Rectangle(int x, int y, int width, int height);

			bool Intersect(const Rectangle& rectangle) const;
			Rectangle IntersectRectangle(const Rectangle& rectangle) const;
			int Area() const;

			friend Rectangle operator+(const Rectangle& leftRectangle, const Rectangle& rightRectangle);
			friend Rectangle operator-(const Rectangle& leftRectangle, const Rectangle& rightRectangle);
			friend Rectangle operator*(const Rectangle& leftRectangle, const Rectangle& rightRectangle);
			friend Rectangle operator/(const Rectangle& leftRectangle, const Rectangle& rightRectangle);

			Rectangle& operator+=(const Rectangle& right);
			Rectangle& operator-=(const Rectangle& right);
			Rectangle& operator*=(const Rectangle& right);
			Rectangle& operator/=(const Rectangle& right);

			bool operator>(const Rectangle& right) const;
			bool operator>=(const Rectangle& right) const;
			bool operator<(const Rectangle& right) const;
			bool operator<=(const Rectangle& right) const;

			bool operator==(const Rectangle& right) const;
			bool operator!=(const Rectangle& right);

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Rectangle);
				output["x"] = x;
				output["y"] = y;
				output["width"] = width;
				output["height"] = height;

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				x = data["x"];
				y = data["y"];
				width = data["width"];
				height = data["height"];
			};

			friend std::ostream& operator<<(std::ostream& os, const Rectangle& rectangle);
		};
	}
}

