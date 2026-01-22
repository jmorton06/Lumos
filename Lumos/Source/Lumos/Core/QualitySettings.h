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

    struct QualitySettings
    {
#ifdef LUMOS_PLATFORM_MACOS
        float RendererScale = 1.0f; // Keep at 1 until fix ui input with this
#else
        float RendererScale = 1.0f;

#endif
        // Shadows
        bool EnableShadows                       = true;
        ShadowQualitySetting ShadowQuality       = ShadowQualitySetting::Low;
        ShadowResolutionSetting ShadowResolution = ShadowResolutionSetting::Low;

        // Post-Process
        bool EnableBloom = false;
        bool EnableDOF   = false;
        bool EnableSSR   = false;
        bool EnableSSAO  = false;
        bool EnableFXAA  = true;

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
                break;
            }
            case 1:
            {
                EnableShadows    = true;
                ShadowQuality    = ShadowQualitySetting::Medium;
                ShadowResolution = ShadowResolutionSetting::Medium;

                EnableBloom = true;
                EnableDOF   = false;
                EnableSSR   = false;
                EnableSSAO  = false;
                EnableFXAA  = true;
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
                EnableSSR   = false;
                EnableSSAO  = true;
                EnableFXAA  = true;
                break;
            }
            }
        }
    };
}
