#pragma once
#include "Scene/Serialisation/Serialisation.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    namespace Graphics
    {
        class TextureCube;
        class Texture;

        class Environment
        {
        public:
            Environment();
            Environment(TextureCube* env)
            {
                m_Environmnet            = SharedPtr<TextureCube>(env);
                m_PrefilteredEnvironment = nullptr;
                m_IrradianceMap          = nullptr;
            }

            Environment(TextureCube* env, TextureCube* irr)
            {
                m_Environmnet            = SharedPtr<TextureCube>(env);
                m_IrradianceMap          = SharedPtr<TextureCube>(irr);
                m_PrefilteredEnvironment = nullptr;
            }

            Environment(const std::string& filepath, bool genPrefilter, bool genIrradiance);
            Environment(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType = ".tga");
            ~Environment();

            void Load(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType = ".tga");
            void Load();

            TextureCube* GetEnvironmentMap() const
            {
                return m_Environmnet.get();
            }
            TextureCube* GetPrefilteredMap() const
            {
                return m_PrefilteredEnvironment.get();
            }
            TextureCube* GetIrradianceMap() const
            {
                return m_IrradianceMap.get();
            }

            void SetEnvironmnet(TextureCube* environmnet);
            void SetPrefilteredEnvironment(TextureCube* prefilteredEnvironment);
            void SetIrradianceMap(TextureCube* irradianceMap);

            template <class Archive>
            void save(Archive& archive) const
            {
                archive(m_FilePath, m_NumMips, m_Width, m_Height, m_FileType, m_IrrFactor);
                archive(m_Mode, m_Parameters);
                archive(m_HorizonColour, m_ZenithColour, m_SunDirection);
                archive(m_FogColour, m_FogParams, m_CloudColour, m_CloudParams, m_CloudWindDir);
                archive(m_StarParams, m_StarColour, m_AuroraColour, m_AuroraParams);
            }

            template <class Archive>
            void load(Archive& archive)
            {
                archive(m_FilePath, m_NumMips, m_Width, m_Height, m_FileType, m_IrrFactor);

                if(Serialisation::CurrentSceneVersion >= 18)
                    archive(m_Mode, m_Parameters);
                if(Serialisation::CurrentSceneVersion >= 28)
                    archive(m_HorizonColour, m_ZenithColour, m_SunDirection);
                if(Serialisation::CurrentSceneVersion >= 29)
                    archive(m_FogColour, m_FogParams, m_CloudColour, m_CloudParams, m_CloudWindDir);
                if(Serialisation::CurrentSceneVersion >= 32)
                    archive(m_StarParams, m_StarColour, m_AuroraColour, m_AuroraParams);
                Load();
            }

            const std::string& GetFilePath() const { return m_FilePath; }
            const std::string& GetFileType() const { return m_FileType; }
            uint32_t GetNumMips() { return m_NumMips; }
            uint32_t GetWidth() { return m_Width; }
            uint32_t GetHeight() { return m_Height; }

            void SetFilePath(const std::string& path) { m_FilePath = path; }
            void SetFileType(const std::string& type) { m_FileType = type; }
            void SetNumMips(uint32_t num) { m_NumMips = num; }
            void SetWidth(uint32_t width) { m_Width = width; }
            void SetHeight(uint32_t height) { m_Height = height; }

            uint8_t GetMode() const { return m_Mode; }

            void SetMode(uint8_t mode) { m_Mode = mode; }

            const Vec4& GetParameters() const { return m_Parameters; }
            void SetParameters(const Vec4& param) { m_Parameters = param; }

            const Vec4& GetHorizonColour() const { return m_HorizonColour; }
            const Vec4& GetZenithColour() const  { return m_ZenithColour; }
            const Vec4& GetSunDirection() const  { return m_SunDirection; }
            void SetHorizonColour(const Vec4& c) { m_HorizonColour = c; }
            void SetZenithColour(const Vec4& c)  { m_ZenithColour  = c; }
            void SetSunDirection(const Vec4& d)  { m_SunDirection  = d; }

            const Vec4& GetFogColour() const { return m_FogColour; }
            const Vec4& GetFogParams() const { return m_FogParams; }
            void SetFogColour(const Vec4& c) { m_FogColour = c; }
            void SetFogParams(const Vec4& p) { m_FogParams = p; }

            const Vec4& GetCloudColour()  const { return m_CloudColour; }
            const Vec4& GetCloudParams()  const { return m_CloudParams; }
            const Vec4& GetCloudWindDir() const { return m_CloudWindDir; }
            void SetCloudColour(const Vec4& c)  { m_CloudColour  = c; }
            void SetCloudParams(const Vec4& p)  { m_CloudParams  = p; }
            void SetCloudWindDir(const Vec4& d) { m_CloudWindDir = d; }

            const Vec4& GetStarParams() const { return m_StarParams; }
            const Vec4& GetStarColour() const { return m_StarColour; }
            void SetStarParams(const Vec4& p) { m_StarParams = p; }
            void SetStarColour(const Vec4& c) { m_StarColour = c; }

            const Vec4& GetAuroraColour() const { return m_AuroraColour; }
            const Vec4& GetAuroraParams() const { return m_AuroraParams; }
            void SetAuroraColour(const Vec4& c) { m_AuroraColour = c; }
            void SetAuroraParams(const Vec4& p) { m_AuroraParams = p; }

        private:
            SharedPtr<TextureCube> m_Environmnet;
            SharedPtr<TextureCube> m_PrefilteredEnvironment;
            SharedPtr<TextureCube> m_IrradianceMap;

            uint32_t m_NumMips = 0;
            uint32_t m_Width   = 0;
            uint32_t m_Height  = 0;
            float m_IrrFactor  = 1.0f;
            std::string m_FilePath;
            std::string m_FileType;
            uint8_t m_Mode = 0;
            Vec4 m_Parameters;
            Vec4 m_HorizonColour = Vec4(0.66f, 0.78f, 0.92f, 1.0f);
            Vec4 m_ZenithColour  = Vec4(0.13f, 0.34f, 0.78f, 1.0f);
            Vec4 m_SunDirection  = Vec4(0.0f, 0.6f, 0.8f, 1.0f); // xyz = dir, w = intensity
            // Default off: density 0, linear end 0 => skipped in shader.
            Vec4 m_FogColour     = Vec4(0.78f, 0.84f, 0.92f, 0.0f);
            Vec4 m_FogParams     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            // Cloud defaults: disabled (style mode -1).
            Vec4 m_CloudColour   = Vec4(1.0f, 1.0f, 1.0f, 0.45f);
            Vec4 m_CloudParams   = Vec4(0.5f, 0.7f, 0.02f, -1.0f);
            Vec4 m_CloudWindDir  = Vec4(1.0f, 0.0f, 0.2f, 0.5f);
            // Stars: defaults off (density 0). Tint defaults to near-white.
            Vec4 m_StarParams    = Vec4(0.0f, 1.0f, 0.0f, 0.35f);
            Vec4 m_StarColour    = Vec4(0.95f, 0.95f, 1.0f, 1.0f);
            // Aurora: defaults off (intensity 0). Classic green base, magenta tip.
            Vec4 m_AuroraColour  = Vec4(0.2f, 0.9f, 0.5f, 0.5f);
            Vec4 m_AuroraParams  = Vec4(0.0f, 0.45f, 0.3f, 0.35f);
        };
    }
}
