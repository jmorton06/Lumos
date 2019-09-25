#include "lmpch.h"																									


#include "GLFMWindow.h"
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{

	GLFMWindow::GLFMWindow(const WindowProperties& properties)
	{
		m_Init = false;
		m_Init = Init(properties, "title");

		Graphics::GraphicsContext::Create(properties, nullptr);

	}

	GLFMWindow::~GLFMWindow()
	{
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


	void GLFMWindow::MakeDefault()
	{
		CreateFunc = CreateFuncGLFW;
	}

	Window* GLFMWindow::CreateFuncGLFM(const WindowProperties& properties)
	{
		return lmnew GLFMWindow(properties);
	}
}
