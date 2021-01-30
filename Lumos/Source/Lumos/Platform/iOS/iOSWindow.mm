#include "Precompiled.h"
#include "iOSWindow.h"
#include "iOSOS.h"
#include "Graphics/API/GraphicsContext.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include "iOSKeyCodes.h"

#include <UIKit/UIKit.h>

namespace Lumos
{
	iOSWindow::iOSWindow(const WindowProperties& properties)
	{
		m_Init = false;
        auto prop = properties;
        
        auto iosOS = (iOSOS*)iOSOS::Instance();

        prop.Width = (uint32_t)iosOS->GetWidth();
        prop.Height = (uint32_t)iosOS->GetHeight();

		m_Init = Init(prop, "title");
        
        m_Handle = iosOS->GetIOSView();
        
		Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(properties.RenderAPI));
		Graphics::GraphicsContext::Create(properties, this);
		Graphics::GraphicsContext::GetContext()->Init();
		
	}

	iOSWindow::~iOSWindow()
	{
		Graphics::GraphicsContext::Release();
	}


	bool iOSWindow::Init(const WindowProperties& properties, const std::string& title)
	{
        LUMOS_LOG_INFO("Creating window - Title : {0}, Width : {1}, Height : {2}", properties.Title, properties.Width, properties.Height);

        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.Exit = false;
        
		return true;
	}

	void iOSWindow::SetWindowTitle(const std::string& title)
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
		return new iOSWindow(properties);
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

    void iOSWindow::OnTouchEvent(uint32_t xPos, uint32_t yPos, uint32_t count, bool down)
    {
        MouseMovedEvent event((float)xPos, (float)yPos);
        m_Data.EventCallback(event);
        
        if(down)
        {
            MouseButtonPressedEvent event2((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_Data.EventCallback(event2);
        }
        else
        {
            MouseButtonReleasedEvent event2((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_Data.EventCallback(event2);
        }
    }

    void iOSWindow::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        MouseMovedEvent event(xPos, yPos);
        m_Data.EventCallback(event);
    }

    void iOSWindow::OnResizeEvent(uint32_t width, uint32_t height)
    {
        WindowResizeEvent event(width, height);
        m_Data.EventCallback(event);
    }
}
