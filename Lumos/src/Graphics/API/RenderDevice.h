#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class RenderDevice
        {
        public:
            RenderDevice() = default;
            virtual ~RenderDevice() = default;
            
            virtual void Init() = 0;
            
            static void Create();
            static void Release();
            
        private:
            static RenderDevice* s_Instance;
        };
    }
}
