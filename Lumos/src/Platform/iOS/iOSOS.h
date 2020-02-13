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
        const char* GetExecutablePath() override;
        
        void* GetIOSView() const { return m_IOSView; }
        void SetIOSView(void* view) { m_IOSView = view; }
        
        String GetAssetPath() const;
        
        void OnFrame();
        void OnQuit();
        void OnKeyPressed(char keycode);
        void OnScreenPressed(u32 x, u32 y, u32 count);
        
        static void SetWindowSize(float x, float y) { m_X = x; m_Y = y;}
        static float m_X,m_Y;
        
        static iOSOS* Get() { return (iOSOS*)s_Instance; }
    private:
        
        void* m_IOSView;
    
    };
}
