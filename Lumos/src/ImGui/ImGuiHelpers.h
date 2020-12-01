#pragma once

#include "Maths/Maths.h"
#include <imgui/imgui.h>

namespace Lumos
{
	namespace Graphics
	{
		class Texture2D;
		class TextureCube;
	}

	namespace ImGuiHelpers
	{
		enum class PropertyFlag
		{
			None = 0,
			ColorProperty = 1
		};

		enum Theme
		{
			Black = 0,
			Dark,
			Grey,
			Light,
			Blue,
			ClassicLight,
			ClassicDark,
			Classic,
			Cherry,
			Cinder
		};

		bool Property(const std::string& name, bool& value);
		bool Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, Maths::Vector2& value, PropertyFlag flags);
		bool Property(const std::string& name, Maths::Vector2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, Maths::Vector3& value, PropertyFlag flags);
		bool Property(const std::string& name, Maths::Vector3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, Maths::Vector4& value, bool exposeW, PropertyFlag flags);
		bool Property(const std::string& name, Maths::Vector4& value, float min = -1.0f, float max = 1.0f, bool exposeW = false, PropertyFlag flags = PropertyFlag::None);

		void Tooltip(const std::string& text);
		void Tooltip(const char* text);

		void Tooltip(Graphics::Texture2D* texture, const Maths::Vector2& size);
		void Tooltip(Graphics::Texture2D* texture, const Maths::Vector2& size, const std::string& text);
		void Image(Graphics::Texture2D* texture, const Maths::Vector2& size);
		void Image(Graphics::TextureCube* texture, const Maths::Vector2& size);

		void SetTheme(Theme theme);

		bool BufferingBar(const char* label, float value, const Maths::Vector2& size_arg, const u32& bg_col, const u32& fg_col);
		bool Spinner(const char* label, float radius, int thickness, const u32& color);
    
        Maths::Vector4 GetSelectedColour();

	}
}

namespace ImGui
{
    // Dupe of DragFloatN with a tweak to add colored lines
    bool DragFloatN_Colored(const char* label, float* v, int components, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* display_format = "%.2f", float power = 1.0f);

    bool DragFloat3Coloured(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);
    bool DragFloat4Coloured(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);
    bool DragFloat2Coloured(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);

    void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full);
    bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
                      const char* display_format, float power);
}


static _FORCE_INLINE_ ImVec2 operator*(const ImVec2& lhs, const float rhs)   { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
static _FORCE_INLINE_ ImVec2 operator/(const ImVec2& lhs, const float rhs)   { return ImVec2(lhs.x/rhs, lhs.y/rhs); }
static _FORCE_INLINE_ ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static _FORCE_INLINE_ ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }
static _FORCE_INLINE_ ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static _FORCE_INLINE_ ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x/rhs.x, lhs.y/rhs.y); }
static _FORCE_INLINE_ ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)     { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static _FORCE_INLINE_ ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)     { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static _FORCE_INLINE_ ImVec2& operator*=(ImVec2& lhs, const float rhs)       { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static _FORCE_INLINE_ ImVec2& operator/=(ImVec2& lhs, const float rhs)       { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static _FORCE_INLINE_ ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w); }
static _FORCE_INLINE_ ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w); }
static _FORCE_INLINE_ ImVec4 operator*(const ImVec4& lhs, const float rhs)   { return ImVec4(lhs.x*rhs, lhs.y*rhs,lhs.z*rhs,lhs.w*rhs); }
static _FORCE_INLINE_ ImVec4 operator/(const ImVec4& lhs, const float rhs)   { return ImVec4(lhs.x/rhs, lhs.y/rhs,lhs.z/rhs,lhs.w/rhs); }
static _FORCE_INLINE_ ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x*rhs.x, lhs.y*rhs.y,lhs.z*rhs.z,lhs.w*rhs.w); }
static _FORCE_INLINE_ ImVec4 operator/(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x/rhs.x,lhs.y/rhs.y,lhs.z/rhs.z,lhs.w/rhs.w); }
static _FORCE_INLINE_ ImVec4& operator+=(ImVec4& lhs, const ImVec4& rhs)     { lhs.x += rhs.x; lhs.y += rhs.y;lhs.z += rhs.z;lhs.w += rhs.w;return lhs; }
static _FORCE_INLINE_ ImVec4& operator-=(ImVec4& lhs, const ImVec4& rhs)     { lhs.x -= rhs.x; lhs.y -= rhs.y;lhs.z -= rhs.z;lhs.w -= rhs.w; return lhs; }
static _FORCE_INLINE_ ImVec4& operator*=(ImVec4& lhs, const float rhs)       { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static _FORCE_INLINE_ ImVec4& operator/=(ImVec4& lhs, const float rhs)       { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static _FORCE_INLINE_ std::ostream& operator<<(std::ostream& ostream, const ImVec2 a) { ostream<< "{ " << a.x << ", " << a.y << " }"; return ostream; }
static _FORCE_INLINE_ std::ostream& operator<<(std::ostream& ostream, const ImVec4 a) { ostream<< "{ " << a.x << ", " << a.y << ", " << a.z << ", " << a.w << " }"; return ostream; }
