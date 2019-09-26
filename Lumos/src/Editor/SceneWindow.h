#include "lmpch.h"
#include "EditorWindow.h"

namespace Lumos
{
	class SceneWindow : public EditorWindow
	{
	public:
		SceneWindow();
		~SceneWindow() = default;

		void OnImGui() override;
	};
}