#include "LM.h"
#include "Core/OS.h"

namespace Lumos
{
    class iOSOS : public OS
    {
    public:
        iOSOS() {}
        ~iOSOS() {}

        void Run() override {}
        
        void* GetIOSView() const { return m_IOSView; }
        void SetIOSView(void* view) { m_IOSView = view; }
        
    private:
        
        void* m_IOSView;
    };
}
