#include "LM.h"																									


#include "GLFMWindow.h"
#include "Graphics/API/Context.h"

namespace Lumos
{

	GLFMWindow::GLFMWindow(const WindowProperties& properties, const String& title)
	{
		m_Init = false;
		//m_Exit = false;
		//m_VSync = properties.vsync;
		m_Timer = new Timer();
		//m_NumSuperSamples = properties.numSuperSamples;

		m_Init = Init(properties, title);

		graphics::Context::Create(properties, nullptr);

	}

	GLFMWindow::~GLFMWindow()
	{
		if (m_Timer != nullptr)
		{
			delete m_Timer;
			m_Timer = nullptr;
		}
	}


	bool GLFMWindow::Init(const WindowProperties& properties, const String& title)
	{
		return true;
	}

	void GLFMWindow::SetWindowTitle(const String& title)
	{

	}

	void GLFMWindow::ToggleVSync()
	{

	}
    
	void GLFMWindow::OnUpdate()
	{

	}

	void GLFMWindow::SetBorderlessWindow(bool borderless)
	{

	}


}
