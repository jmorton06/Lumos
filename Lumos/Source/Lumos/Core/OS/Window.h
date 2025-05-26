#pragma once
#include "Core/Reference.h"
#include "Events/Event.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/GraphicsContext.h"

#include "Maths/Vector2.h"
#include "Maths/MathsFwd.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/Function.h"

namespace Lumos
{
    struct LUMOS_EXPORT WindowDesc
    {
        WindowDesc(uint32_t width = 1280, uint32_t height = 720, int renderAPI = 0, const String8& title = Str8Lit("Lumos"), bool fullscreen = false, bool vSync = true, bool borderless = false)
            : Width(width)
            , Height(height)
            , Title(title)
            , Fullscreen(fullscreen)
            , VSync(vSync)
            , Borderless(borderless)
            , RenderAPI(renderAPI)
        {
        }

        uint32_t Width, Height;
        bool Fullscreen;
        bool VSync;
        bool Borderless;
        bool ShowConsole = true;
        String8 Title;
        int RenderAPI;
        TDArray<String8> IconPaths;
        TDArray<uint32_t> IconDataSizes;
        TDArray<uint8_t*> IconData;
    };

    class LUMOS_EXPORT Window
    {
    public:
        using EventCallbackFn = Function<void(Event&)>;

        static Window* Create();
        virtual ~Window();
        virtual bool Init(const WindowDesc& windowDesc) = 0;

        bool HasInitialised() const
        {
            return m_Init;
        };

        virtual bool GetExit() const    = 0;
        virtual void SetExit(bool exit) = 0;

        inline void SetHasResized(bool resized)
        {
            m_HasResized = resized;
        }
        inline bool GetHasResized() const
        {
            return m_HasResized;
        }

        virtual void ToggleVSync()                            = 0;
        virtual void SetVSync(bool set)                       = 0;
        virtual void SetWindowTitle(const std::string& title) = 0;
        virtual void OnUpdate()                               = 0;
        virtual void ProcessInput() { };
        virtual void SetBorderlessWindow(bool borderless) = 0;
        virtual void* GetHandle()
        {
            return nullptr;
        };
        virtual float GetScreenRatio() const = 0;
        virtual void HideMouse(bool hide) { };
        virtual void SetMousePosition(const Vec2& pos) { };
        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void UpdateCursorImGui()                               = 0;
        virtual void SetIcon(const WindowDesc& desc)                   = 0;
        virtual void Maximise() { };
        virtual std::string GetTitle() const = 0;
        virtual uint32_t GetWidth() const    = 0;
        virtual uint32_t GetHeight() const   = 0;
        virtual float GetDPIScale() const { return 1.0f; }
        virtual bool GetVSync() const { return m_VSync; };
        virtual void UpdateControllers() { }

        void SetWindowFocus(bool focus) { m_WindowFocus = focus; }
        bool GetWindowFocus() const { return m_WindowFocus; }

        const SharedPtr<Lumos::Graphics::SwapChain>& GetSwapChain() const { return m_SwapChain; }
        const SharedPtr<Lumos::Graphics::GraphicsContext>& GetGraphicsContext() const { return m_GraphicsContext; }

    protected:
        static Window* (*CreateFunc)();

        Window() = default;

        bool m_Init = false;
        Vec2 m_Position;
        bool m_VSync       = false;
        bool m_HasResized  = false;
        bool m_WindowFocus = true;

        SharedPtr<Lumos::Graphics::SwapChain> m_SwapChain;
        SharedPtr<Lumos::Graphics::GraphicsContext> m_GraphicsContext;
    };

}
