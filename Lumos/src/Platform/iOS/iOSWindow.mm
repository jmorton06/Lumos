#include "lmpch.h"
#include "iOSWindow.h"
#include "iOSOS.h"
#include "Graphics/API/GraphicsContext.h"
#include <UIKit/UIKit.h>

void* Lumos::iOSWindow::iosView = nullptr;

namespace Lumos
{
	iOSWindow::iOSWindow(const WindowProperties& properties)
	{
		m_Init = false;
        auto prop = properties;
        
        prop.Width = (u32)((iOSOS*)iOSOS::Instance())->m_X;
        prop.Height = (u32)((iOSOS*)iOSOS::Instance())->m_Y;

		m_Init = Init(prop, "title");
        
		Graphics::GraphicsContext::Create(prop, nullptr);

	}

	iOSWindow::~iOSWindow()
	{
	}


	bool iOSWindow::Init(const WindowProperties& properties, const String& title)
	{
        LUMOS_LOG_INFO("Creating window - Title : {0}, Width : {1}, Height : {2}", properties.Title, properties.Width, properties.Height);

        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Width;
        m_Data.Exit = false;
        
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
