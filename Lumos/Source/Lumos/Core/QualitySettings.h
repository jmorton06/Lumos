#pragma once

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
        float RendererScale = 0.75f;
#else
        float RendererScale = 1.0f;

#endif
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

        void SetGeneralLevel(uint8_t level)
        {
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
                EnableFXAA  = false;
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
