#pragma once
#include <cmath>
#include <cstring>

namespace Lumos
{
    enum class ShadowQualitySetting
    {
        None   = 0,
        Low    = 1,
        Medium = 2,
        High   = 3
    };

    enum class ShadowResolutionSetting
    {
        None   = 0,
        Low    = 1,
        Medium = 2,
        High   = 3
    };

    enum class SSRQualitySetting
    {
        Low    = 1,
        Medium = 2,
        High   = 3
    };

    enum class SSAOQualitySetting
    {
        Low    = 1,
        Medium = 2,
        High   = 3
    };

    struct QualitySettings
    {
#ifdef LUMOS_PLATFORM_MACOS
        float RendererScale = 1.0f;
#else
        float RendererScale = 1.0f;

#endif
        // 0 low, 1 medium, 2 high (SetGeneralLevel). Picked from the GPU at startup unless forced.
        uint8_t GeneralLevel = 1;
        // cheaper forward shading (Lambert, no spec AA, light cut-off) - low level, or a weak GPU at medium
        bool LowForwardShader = false;
        // early-z pre-pass; costs more than it saves on a weak GPU when nothing reads its G-buffer
        bool EnableDepthPrePass = true;
        // Textures bigger than this are halved as they load (memory: a phone kills the app well before the GPU runs out)
        uint32_t MaxTextureSize = 2048;

        // Frame-time driven render scale: steps down when frames run long, probes back up
        bool DynamicResolution = false;
        float MinScale         = 0.5f;
        float MaxScale         = 1.0f;
        float TargetFrameMs    = 16.7f;

        // Shadows
        bool EnableShadows                       = true;
        ShadowQualitySetting ShadowQuality       = ShadowQualitySetting::Low;
        ShadowResolutionSetting ShadowResolution = ShadowResolutionSetting::Low;

        // Post-Process
        bool EnableBloom = true;
        bool EnableDOF   = false;
        bool EnableSSR   = false;
        bool EnableSSAO  = false;
        bool EnableFXAA  = true;

        SSRQualitySetting SSRQuality   = SSRQualitySetting::Medium;
        SSAOQualitySetting SSAOQuality = SSAOQualitySetting::Medium;

        // SSAO kernel samples per quality (max 64 — size of the sample array).
        int SSAOSampleCount() const
        {
            switch(SSAOQuality)
            {
            case SSAOQualitySetting::Low:  return 8;
            case SSAOQualitySetting::High: return 32;
            default:                       return 16;
            }
        }

        int SSRStepCap() const
        {
            switch(SSRQuality)
            {
            case SSRQualitySetting::Low:  return 8;
            case SSRQualitySetting::High: return 24;
            default:                      return 16;
            }
        }
        int SSRBinaryStepCap() const
        {
            switch(SSRQuality)
            {
            case SSRQualitySetting::Low:  return 3;
            case SSRQualitySetting::High: return 5;
            default:                      return 4;
            }
        }

        u32 IrradianceMapSize = 64;
#ifdef LUMOS_PLATFORM_MACOS
        u32 EnvironmentMapSize = 128;
        u32 EnvironmentSamples = 32;
#else
        u32 EnvironmentMapSize = 512;
        u32 EnvironmentSamples = 512;
#endif

        // gpuType: 0 unknown, 1 integrated, 2 discrete, 3 virtual, 4 software (RenderCapabilities::GPUType)
        static uint8_t DetectLevel(int gpuType, uint32_t memoryMB, const char* gpuName)
        {
#ifdef LUMOS_PLATFORM_IOS
            // a phone throttles and has no room for high-end settings
            return DetectLevelRaw(gpuType, memoryMB, gpuName) > 1 ? 1 : DetectLevelRaw(gpuType, memoryMB, gpuName);
        }

        static uint8_t DetectLevelRaw(int gpuType, uint32_t memoryMB, const char* gpuName)
        {
#endif
            const char* name = gpuName ? gpuName : "";
            if(gpuType == 4)
                return 0;
            if(gpuType == 2)
                return memoryMB >= 3500 ? 2 : 1; // small discrete cards are mid range
            if(std::strstr(name, "Apple"))
                return (std::strstr(name, "Pro") || std::strstr(name, "Max") || std::strstr(name, "Ultra")) ? 2 : 1;
            // Intel / AMD integrated, mobile, unknown
            return gpuType == 1 ? 0 : 1;
        }

        // Pixels per frame the level is happy to shade; a retina window is 4x its point size
        static float PixelBudget(uint8_t level)
        {
            return level == 0 ? 1.5e6f : (level == 1 ? 2.6e6f : 1.0e9f);
        }

        static float ScaleForBudget(uint8_t level, uint32_t width, uint32_t height, float minScale)
        {
            const float pixels = (float)width * (float)height;
            if(pixels < 1.0f)
                return 1.0f;
            const float scale = std::sqrt(PixelBudget(level) / pixels);
            return scale < minScale ? minScale : (scale > 1.0f ? 1.0f : scale);
        }

        void SetGeneralLevel(uint8_t level)
        {
            GeneralLevel     = level > 2 ? 2 : level;
            LowForwardShader   = level == 0;
            EnableDepthPrePass = level != 0;
#ifdef LUMOS_PLATFORM_IOS
            MaxTextureSize = level == 0 ? 512 : 1024;
#else
            MaxTextureSize = level == 0 ? 1024 : 2048;
#endif
            switch(level)
            {
            case 0:
            {
                EnableShadows    = true;
                ShadowQuality    = ShadowQualitySetting::Low;
                ShadowResolution = ShadowResolutionSetting::Low;

                EnableBloom = false;
                EnableDOF   = false;
                EnableSSR   = false;
                EnableSSAO  = false;
                EnableFXAA  = true;
                SSRQuality  = SSRQualitySetting::Low;
                SSAOQuality = SSAOQualitySetting::Low;
                break;
            }
            case 1:
            {
                EnableShadows    = true;
                ShadowQuality    = ShadowQualitySetting::Medium;
                ShadowResolution = ShadowResolutionSetting::Medium;

                EnableBloom = true;
                EnableDOF   = false;
                EnableSSR   = true;
                EnableSSAO  = true;
                EnableFXAA  = true;
                SSRQuality  = SSRQualitySetting::Medium;
                SSAOQuality = SSAOQualitySetting::Medium;
                break;
            }
            default:
            case 2:
            {
                EnableShadows    = true;
                ShadowQuality    = ShadowQualitySetting::High;
                ShadowResolution = ShadowResolutionSetting::High;

                EnableBloom = true;
                EnableDOF   = true;
                EnableSSR   = true;
                EnableSSAO  = true;
                EnableFXAA  = true;
                SSRQuality  = SSRQualitySetting::High;
                SSAOQuality = SSAOQualitySetting::High;
                break;
            }
            }
        }
    };
}
