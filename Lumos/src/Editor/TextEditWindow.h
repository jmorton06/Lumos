#pragma once
#include "lmpch.h"
#include "EditorWindow.h"

#include <imgui/plugins/ImTextEditor.h>

namespace Lumos
{
	class TextEditWindow : public EditorWindow
	{
	public:
		TextEditWindow(const String& filePath);
		~TextEditWindow() = default;

		void OnImGui() override;
        void OnClose();
		
	private:

        String m_FilePath;
        TextEditor editor;
	};
}
