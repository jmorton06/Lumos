#include "Precompiled.h"
#include "iOSWindow.h"
#include "iOSOS.h"
#include "Graphics/RHI/GraphicsContext.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include "iOSKeyCodes.h"

#include <UIKit/UIKit.h>

namespace Lumos
{
	iOSWindow::iOSWindow(const WindowDesc& properties)
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


	bool iOSWindow::Init(const WindowDesc& properties, const std::string& title)
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

    void iOSWindow::ProcessInput()
    {
        for(auto& event : m_QueuedEvents)
        {
            m_Data.EventCallback(*event);
            delete event;
        }
        
        m_QueuedEvents.clear();
    }

	void iOSWindow::MakeDefault()
	{
		CreateFunc = CreateFunciOS;
	}

	Window* iOSWindow::CreateFunciOS(const WindowDesc& properties)
	{
		return new iOSWindow(properties);
	}

    void iOSWindow::OnKeyEvent(Lumos::InputCode::Key key, bool down)
    {
        if(down)
        {
            KeyPressedEvent* event = new KeyPressedEvent(key, 0);
            m_QueuedEvents.push_back(event);
        }
        else
        {
            KeyReleasedEvent* event = new KeyReleasedEvent(key);
            m_QueuedEvents.push_back(event);
        }
    }

    void iOSWindow::OnTouchEvent(uint32_t xPos, uint32_t yPos, uint32_t count, bool down)
    {
        MouseMovedEvent* event = new MouseMovedEvent((float)xPos, (float)yPos);
        m_QueuedEvents.push_back(event);

        
        if(down)
        {
            MouseButtonPressedEvent* event2 = new MouseButtonPressedEvent((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_QueuedEvents.push_back(event2);
        }
        else
        {
            MouseButtonReleasedEvent* event2 = new MouseButtonReleasedEvent((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_QueuedEvents.push_back(event2);

        }
    }

    void iOSWindow::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        MouseMovedEvent* event = new MouseMovedEvent(xPos, yPos);
        m_QueuedEvents.push_back(event);
    }

    void iOSWindow::OnResizeEvent(uint32_t width, uint32_t height)
    {
        WindowResizeEvent* event = new WindowResizeEvent(width, height);
        m_QueuedEvents.push_back(event);
    }
}
