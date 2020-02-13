#include "lmpch.h"
#include "iOSWindow.h"
#include "iOSOS.h"
#include "Graphics/API/GraphicsContext.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

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
        m_Data.Height = properties.Height;
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

    void iOSWindow::OnKeyEvent(Lumos::InputCode::Key key, bool down)
    {
        KeyPressedEvent event(key, 0);
        m_Data.EventCallback(event);
    }

    void iOSWindow::OnTouchEvent(u32 xPos, u32 yPos, u32 count, bool down)
    {
        MouseMovedEvent event((float)xPos, (float)yPos);
        m_Data.EventCallback(event);
        
        MouseButtonPressedEvent event2(count);
        m_Data.EventCallback(event2);
    }
}
