#include "lmpch.h"
#include "iOSWindow.h"
#include "Graphics/API/GraphicsContext.h"
#include <UIKit/UIKit.h>

void* Lumos::iOSWindow::iosView = nullptr;

namespace Lumos
{
	iOSWindow::iOSWindow(const WindowProperties& properties)
	{
		m_Init = false;
		m_Init = Init(properties, "title");

		Graphics::GraphicsContext::Create(properties, nullptr);

	}

	iOSWindow::~iOSWindow()
	{
	}


	bool iOSWindow::Init(const WindowProperties& properties, const String& title)
	{
		return true;
	}

	void iOSWindow::SetWindowTitle(const String& title)
	{

	}

	void iOSWindow::ToggleVSync()
	{

	}
    
	void iOSWindow::OnUpdate()
	{

	}

	void iOSWindow::SetBorderlessWindow(bool borderless)
	{

	}

	void iOSWindow::MakeDefault()
	{
		CreateFunc = CreateFunciOS;
	}

	Window* iOSWindow::CreateFunciOS(const WindowProperties& properties)
	{
		return lmnew iOSWindow(properties);
	}
}
