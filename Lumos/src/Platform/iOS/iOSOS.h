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
        void OnKeyPressed(u32 keycode);
        
        static void SetWindowSize(float x, float y) { m_X = x; m_Y = y;}
        static float m_X,m_Y;
    private:
        
        void* m_IOSView;
    
    };
}
