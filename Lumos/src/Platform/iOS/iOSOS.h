#include "lmpch.h"
#include "Core/OS/OS.h"

namespace Lumos
{
    class iOSOS : public OS
    {
    public:
        iOSOS();
        ~iOSOS();

        void Init();
        void Run() override {}
        String GetExecutablePath() override;
        String GetAssetPath() override;
        void Vibrate() const override;
        
        void* GetIOSView() const { return m_IOSView; }
        void SetIOSView(void* view) { m_IOSView = view; }
                
        void OnFrame();
        void OnQuit();
        void OnKeyPressed(char keycode, bool down);
        void OnScreenPressed(u32 x, u32 y, u32 count, bool down);
        void OnMouseMovedEvent(u32 xPos, u32 yPos);
        void OnScreenResize(u32 width, u32 height);
        
        static void Alert(const char *message, const char *title);

        String GetModelName() const;
        
        void SetWindowSize(float x, float y) { m_X = x; m_Y = y;}
        float GetWidth() { return m_X; }
        float GetHeight() { return m_Y; }
        
        static iOSOS* Get() { return (iOSOS*)s_Instance; }
    private:
        
        void* m_IOSView;
        float m_X, m_Y;
    
    };
}
