#pragma once

#include "Maths/Maths.h"

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
