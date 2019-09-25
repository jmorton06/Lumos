#include "lmpch.h"
#include "Rectangle.h"
#include "MathsUtilities.h"

namespace Lumos
{
	namespace Maths
	{
		Rectangle::Rectangle() :
			x(0), y(0), width(0), height(0)
		{
		}

		Rectangle::Rectangle(int x, int y, int width, int height) :
			x(x), y(y), width(width), height(height)
		{
		}

		bool Rectangle::Intersect(const Rectangle& rectangle) const
		{
			return y < rectangle.y + rectangle.height && y + height > rectangle.y && x + width > rectangle.x && x < rectangle.x + rectangle.width;
		}

		Rectangle Rectangle::IntersectRectangle(const Rectangle& rectangle) const
		{
			if (Intersect(rectangle))
			{
				int left = Max(this->x, rectangle.x);
				int right = Min(this->x + this->width, rectangle.x + rectangle.width);
				int top = Max(this->y, rectangle.y);
				int bottom = Min(this->y + this->height, rectangle.y + rectangle.height);

				int width = abs(left - right);
				int height = abs(top - bottom);

				return Rectangle(left, top, width, height);
			}

			return Rectangle(0, 0, 0, 0);
		}

		int Rectangle::Area() const
		{
			return this->width * this->height;
		}

		Rectangle& Rectangle::operator+=(const Rectangle& right)
		{
			this->x += right.x;
			this->y += right.y;
			this->width += right.width;
			this->height += right.height;
			return *this;
		}

		Rectangle& Rectangle::operator-=(const Rectangle& right)
		{
			this->x -= right.x;
			this->y -= right.y;
			this->width -= right.width;
			this->height -= right.height;
			return *this;
		}

		Rectangle& Rectangle::operator*=(const Rectangle& right)
		{
			this->x *= right.x;
			this->y *= right.y;
			this->width *= right.width;
			this->height *= right.height;
			return *this;
		}

		Rectangle& Rectangle::operator/=(const Rectangle& right)
		{
			this->x /= right.x;
			this->y /= right.y;
			this->width /= right.width;
			this->height /= right.height;
			return *this;
		}

		bool Rectangle::operator>(const Rectangle& right) const
		{
			return this->Area() > right.Area();
		}

		bool Rectangle::operator>=(const Rectangle& right) const
		{
			return this->Area() >= right.Area();
		}

		bool Rectangle::operator<(const Rectangle& right) const
		{
			return this->Area() < right.Area();
		}

		bool Rectangle::operator<=(const Rectangle& right) const
		{
			return this->Area() <= right.Area();
		}

		bool Rectangle::operator==(const Rectangle& right) const
		{
			return this->Area() == right.Area();
		}

		bool Rectangle::operator!=(const Rectangle& right)
		{
			return !(*this == right);
		}

		Rectangle operator+(const Rectangle& leftRectangle, const Rectangle& rightRectangle)
		{
			return Rectangle(leftRectangle.x + rightRectangle.x, leftRectangle.y + rightRectangle.y, leftRectangle.width + rightRectangle.width, leftRectangle.height + rightRectangle.height);
		}

		Rectangle operator-(const Rectangle& leftRectangle, const Rectangle& rightRectangle)
		{
			return Rectangle(leftRectangle.x - rightRectangle.x, leftRectangle.y - rightRectangle.y, leftRectangle.width - rightRectangle.width, leftRectangle.height * rightRectangle.height);
		}

		Rectangle operator*(const Rectangle& leftRectangle, const Rectangle& rightRectangle)
		{
			return Rectangle(leftRectangle.x * rightRectangle.x, leftRectangle.y * rightRectangle.y, leftRectangle.width * rightRectangle.width, leftRectangle.height * rightRectangle.height);
		}

		Rectangle operator/(const Rectangle& leftRectangle, const Rectangle& rightRectangle)
		{
			return Rectangle(leftRectangle.x / rightRectangle.x, leftRectangle.y / rightRectangle.y, leftRectangle.width / rightRectangle.width, leftRectangle.height / rightRectangle.height);
		}

		std::ostream& operator<<(std::ostream& os, const Rectangle& rectangle)
		{
			return os << "{" << rectangle.x << ", " << rectangle.y << ", " << rectangle.width << ", " << rectangle.height << "}";
		}
	}
}