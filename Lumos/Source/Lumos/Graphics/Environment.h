#pragma once

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
                m_Environmnet = SharedRef<TextureCube>(env);
                m_PrefilteredEnvironment = nullptr;
                m_IrradianceMap = nullptr;
            }

            Environment(TextureCube* env, TextureCube* irr)
            {
                m_Environmnet = SharedRef<TextureCube>(env);
                m_IrradianceMap = SharedRef<TextureCube>(irr);
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
            }

            template <class Archive>
            void load(Archive& archive)
            {
                archive(m_FilePath, m_NumMips, m_Width, m_Height, m_FileType, m_IrrFactor);
                if(m_FilePath != "")
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

        private:
            SharedRef<TextureCube> m_Environmnet;
            SharedRef<TextureCube> m_PrefilteredEnvironment;
            SharedRef<TextureCube> m_IrradianceMap;

            uint32_t m_NumMips = 0;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            float m_IrrFactor = 1.0f;
            std::string m_FilePath;
            std::string m_FileType;
        };
    }
}
