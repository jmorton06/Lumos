#pragma once
#include "Core/Core.h"

namespace Lumos
{
    namespace Graphics
    {
        // In frame order - the pass cutoff relies on it.
        enum RenderPassID : u32
        {
            RenderPass_DepthPrePass = 0,
            RenderPass_SSAO,
            RenderPass_SSAOBlur,
            RenderPass_ContactShadows,
            RenderPass_Shadow,
            RenderPass_GI,
            RenderPass_LightCulling,
            RenderPass_Forward,
            RenderPass_Cosmo,
            RenderPass_Skybox,
            RenderPass_2D,
            RenderPass_2DLit,
            RenderPass_Particle,
            RenderPass_PointCloud,
            RenderPass_LineCloud,
            RenderPass_Planet,
            RenderPass_SSR,
            RenderPass_VolumetricFog,
            RenderPass_TAA,
            RenderPass_DepthOfField,
            RenderPass_Bloom,
            RenderPass_Debanding,
            RenderPass_EyeAdaptation,
            RenderPass_ToneMapping,
            RenderPass_MotionBlur,
            RenderPass_Vignette,
            RenderPass_Sharpen,
            RenderPass_SMAA,
            RenderPass_FXAA,
            RenderPass_ChromaticAberation,
            RenderPass_FilmicGrain,
            RenderPass_Debug,
            RenderPass_Text,
            RenderPass_WorldText,
            RenderPass_UI,
            RenderPass_Final,
            RenderPass_Count
        };

        struct RenderPassInfo
        {
            const char* Name;
            const char* Category;
            const char* Summary;
            const char* Reads;
            const char* Writes;
            const char* LookFor;
            // SceneRenderSettings::DebugMode that shows this pass's own buffer, 0 = none
            i32 BufferView;
        };

        enum class RenderPassState : u8
        {
            Skipped, // disabled or nothing for it to do this frame
            Ran,
            CutOff, // after the pass cutoff
        };

        struct RenderPassRecord
        {
            RenderPassState State = RenderPassState::Skipped;
            u32 DrawCalls         = 0;
            f32 CpuMs             = 0.0f;
        };

        LUMOS_EXPORT const RenderPassInfo& GetRenderPassInfo(RenderPassID id);
    }
}
