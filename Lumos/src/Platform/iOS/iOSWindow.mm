#include "lmpch.h"
#include "iOSWindow.h"
#include "iOSOS.h"
#include "Graphics/API/GraphicsContext.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include <UIKit/UIKit.h>

namespace Lumos
{
	iOSWindow::iOSWindow(const WindowProperties& properties)
	{
		m_Init = false;
        auto prop = properties;
        
        auto iosOS = (iOSOS*)iOSOS::Instance();

        prop.Width = (u32)iosOS->GetWidth();
        prop.Height = (u32)iosOS->GetHeight();

		m_Init = Init(prop, "title");
        
        m_Handle = iosOS->GetIOSView();
        
		Graphics::GraphicsContext::Create(prop, m_Handle);
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
        if(down)
        {
            KeyPressedEvent event(key, 0);
            m_Data.EventCallback(event);
        }
        else
        {
            KeyReleasedEvent event(key);
            m_Data.EventCallback(event);
        }
    }

    void iOSWindow::OnTouchEvent(u32 xPos, u32 yPos, u32 count, bool down)
    {
        MouseMovedEvent event((float)xPos, (float)yPos);
        m_Data.EventCallback(event);
        
        if(down)
        {
            MouseButtonPressedEvent event2(LUMOS_MOUSE_LEFT);
            m_Data.EventCallback(event2);
        }
        else
        {
            MouseButtonReleasedEvent event2(LUMOS_MOUSE_LEFT);
            m_Data.EventCallback(event2);
        }
    }

    void iOSWindow::OnMouseMovedEvent(u32 xPos, u32 yPos)
    {
        MouseMovedEvent event(xPos, yPos);
        m_Data.EventCallback(event);
    }

    void iOSWindow::OnResizeEvent(u32 width, u32 height)
    {
        WindowResizeEvent event(width, height);
        m_Data.EventCallback(event);
    }
}
