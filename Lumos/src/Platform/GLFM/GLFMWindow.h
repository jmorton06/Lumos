#pragma once

#include "LM.h"
#include "Input/Window.h"

namespace Lumos
{

	class LUMOS_EXPORT GLFMWindow : public Window
	{
	public:
		GLFMWindow(const WindowProperties& properties, const String& title);
		~GLFMWindow();

		void ToggleVSync() override;
		void SwapBuffers() override;
		void SetWindowTitle(const String& title) override;
		void SetBorderlessWindow(bool borderless) override;
		void Update() override;

		bool Init(const WindowProperties& properties, const String& title);

		inline void* GetHandle() override { return (void*)nullptr; }//m_Handle; }

		inline uint GetWidth()  const override { return 0; }
		inline uint GetHeight() const override { return 0; }

	protected:

		uint m_Handle;
	};
}