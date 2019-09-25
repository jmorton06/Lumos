#pragma once
#include "lmpch.h"

namespace Lumos
{
	class EditorWindow
	{
	public:
		virtual ~EditorWindow() = default;

		virtual const String& GetName() const = 0;
		virtual void OnImGui() = 0;
	};
}