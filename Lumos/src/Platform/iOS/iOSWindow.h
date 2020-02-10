#include "Core/OS/Window.h"

namespace Lumos
{
    class iOSWindow : public Window
    {
    public:
        iOSWindow(const WindowProperties& properties);
        ~iOSWindow();

        void ToggleVSync() override;
		void SetWindowTitle(const String& title) override;
		void SetBorderlessWindow(bool borderless) override;
		void OnUpdate() override;

		bool Init(const WindowProperties& properties, const String& title);

		inline void* GetHandle() override { return (void*)iosView; }

		inline uint GetWidth()  const override { return m_Data.Width; }
		inline uint GetHeight() const override { return m_Data.Height; }
        
        bool GetExit() const override { return false; }
        void SetExit(bool exit) override { }
        
        void SetVSync(bool set) override { }
        float GetScreenRatio() const override { return (float)m_Data.Width / (float)m_Data.Height; }
        
        void SetEventCallback(const EventCallbackFn& callback) override { };
        void SetIcon(const String& filePath, const String& smallIconFilePath) override {};
        void UpdateCursorImGui() override {};
        
        String GetTitle() const override { return "Mobile"; }

        static void MakeDefault();
        static void* GetIOSView() { return iosView; }
        static void SetIOSView(void* view) { iosView = view; }
    protected:

		static Window* CreateFunciOS(const WindowProperties& properties);

        struct WindowData
		{
			std::string Title;
			u32 Width, Height;
			bool VSync;
			bool Exit;
			//Graphics::RenderAPI m_RenderAPI;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;

        static void* iosView;
    };
}
