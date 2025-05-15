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
	iOSWindow::iOSWindow()
	{
		m_Init = false;
    }
    
	iOSWindow::~iOSWindow()
	{
	}
    
    
	bool iOSWindow::Init(const WindowDesc& properties)
	{
        auto prop = properties;
        
        iOSOS* iosOS = iOSOS::Get();
        
        prop.Width = (uint32_t)iosOS->GetWidth();
        prop.Height = (uint32_t)iosOS->GetHeight();
        
        m_Handle = (void*)iosOS->GetLayerPtr();
        LINFO("Creating window - Title : %s, Width : %i, Height : %i", (const char*)properties.Title.str, properties.Width, properties.Height);
        
        m_Data.Title = ToStdString(properties.Title);
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.Exit = false;
        
		m_GraphicsContext = SharedPtr<Graphics::GraphicsContext>(Graphics::GraphicsContext::Create());
        m_GraphicsContext->Init();
        
		m_SwapChain = SharedPtr<Graphics::SwapChain>(Graphics::SwapChain::Create(m_Data.Width, m_Data.Height));
		m_SwapChain->Init(m_Data.VSync, (Window*)this);
        
        m_Init = true;
        
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
        
        m_QueuedEvents.Clear();
    }
    
	void iOSWindow::MakeDefault()
	{
		CreateFunc = CreateFunciOS;
	}
    
	Window* iOSWindow::CreateFunciOS()
	{
		return new iOSWindow();
	}
    
    void iOSWindow::OnKeyEvent(Lumos::InputCode::Key key, bool down)
    {
        if(down)
        {
            KeyPressedEvent* event = new KeyPressedEvent(key, 0);
            m_QueuedEvents.PushBack(event);
        }
        else
        {
            KeyReleasedEvent* event = new KeyReleasedEvent(key);
            m_QueuedEvents.PushBack(event);
        }
    }
    
    void iOSWindow::OnTouchEvent(uint32_t xPos, uint32_t yPos, uint32_t count, bool down)
    {
        MouseMovedEvent* event = new MouseMovedEvent((float)xPos, (float)yPos);
        m_QueuedEvents.PushBack(event);
        
        
        if(down)
        {
            MouseButtonPressedEvent* event2 = new MouseButtonPressedEvent((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_QueuedEvents.PushBack(event2);
        }
        else
        {
            MouseButtonReleasedEvent* event2 = new MouseButtonReleasedEvent((Lumos::InputCode::MouseKey)Lumos::iOSKeyCodes::iOSTouchToLumosMouseKey(count));
            m_QueuedEvents.PushBack(event2);
            
        }
    }
    
    void iOSWindow::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        MouseMovedEvent* event = new MouseMovedEvent(xPos, yPos);
        m_QueuedEvents.PushBack(event);
    }
    
    void iOSWindow::OnResizeEvent(uint32_t width, uint32_t height)
    {
        WindowResizeEvent* event = new WindowResizeEvent(width, height);
        m_QueuedEvents.PushBack(event);
    }
}
