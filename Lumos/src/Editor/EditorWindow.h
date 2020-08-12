#pragma once
#include "lmpch.h"

namespace Lumos
{
	class Editor;
	class Scene;

	class EditorWindow
	{
	public:
		virtual ~EditorWindow() = default;

		const std::string& GetName() const
		{
			return m_Name;
		}
		const std::string& GetSimpleName() const
		{
			return m_SimpleName;
		}

		void SetEditor(Editor* editor)
		{
			m_Editor = editor;
		}
		Editor* GetEditor()
		{
			return m_Editor;
		}
		bool& Active()
		{
			return m_Active;
		}
		void SetActive(bool active)
		{
			m_Active = active;
		}
		virtual void OnImGui() = 0;
		virtual void OnNewScene(Scene* scene)
		{
		}

	protected:
		bool m_Active = true;
		std::string m_Name;
		std::string m_SimpleName;
		Editor* m_Editor = nullptr;
	};
}