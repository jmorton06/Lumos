#pragma once

#include "Graphics/RHI/GraphicsContext.h"

namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT GLContext : public GraphicsContext
        {
        public:
            GLContext(const WindowDesc& properties, Window* window);
            ~GLContext();

            void Present() override;
            void Init() override {};

            inline static GLContext* Get() { return static_cast<GLContext*>(s_Context); }

            size_t GetMinUniformBufferOffsetAlignment() const override { return 256; }

            bool FlipImGUITexture() const override { return true; }

            float GetGPUMemoryUsed() override { return 0.0f; }
            float GetTotalGPUMemory() override { return 0.0f; }

            void WaitIdle() const override { }

            void OnImGui() override;
            static void MakeDefault();

        protected:
            static GraphicsContext* CreateFuncGL(const WindowDesc& properties, Window* cont);
        };
    }
}
