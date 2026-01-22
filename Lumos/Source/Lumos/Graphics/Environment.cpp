#include "Precompiled.h"
#include "Environment.h"
#include "Core/Application.h"
#include "Renderers/SceneRenderer.h"
#include "RHI/Texture.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"

namespace Lumos
{
    namespace Graphics
    {
        Environment::Environment()
        {
            m_Environmnet            = nullptr;
            m_PrefilteredEnvironment = nullptr;
            m_IrradianceMap          = nullptr;
            m_IrrFactor              = 32.0f / 1024.0f;
            m_Mode                   = 1;
            m_Parameters             = { 2.0f, 0.0f, 0.0f, 1.0f };
        }

        Environment::Environment(const std::string& filepath, bool genPrefilter, bool genIrradiance)
        {
        }

        Environment::Environment(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType)
        {
            m_Width     = width;
            m_Height    = height;
            m_NumMips   = numMip;
            m_FilePath  = name;
            m_FileType  = fileType;
            m_IrrFactor = irrSizeFactor;

            Load();
        }

        void Environment::Load(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType)
        {
            m_Width     = width;
            m_Height    = height;
            m_NumMips   = numMip;
            m_FilePath  = name;
            m_FileType  = fileType;
            m_IrrFactor = irrSizeFactor;

            Load();
        }

        void Environment::Load()
        {
            LUMOS_PROFILE_FUNCTION();

            std::string* envFiles = new std::string[m_NumMips];
            std::string* irrFiles = new std::string[m_NumMips];

            uint32_t currWidth  = m_Width;
            uint32_t currHeight = m_Height;

            bool failed = false;

            if(m_Mode == 0)
            {
                m_Parameters.w = m_Mode;

                if(m_FileType == ".hdr")
                {
                    Application::Get().GetSceneRenderer()->CreateCubeMap(m_FilePath + "_Env_" + StringUtilities::ToString(0) + "_" + StringUtilities::ToString(currWidth) + "x" + StringUtilities::ToString(currHeight) + m_FileType, m_Parameters, m_Environmnet, m_IrradianceMap);

                    delete[] envFiles;
                    delete[] irrFiles;

                    return;
                }
                else
                {
                    ArenaTemp Scratch = ScratchBegin(0, 0);
                    for(uint32_t i = 0; i < m_NumMips; i++)
                    {
                        envFiles[i] = m_FilePath + "_Env_" + StringUtilities::ToString(i) + "_" + StringUtilities::ToString(currWidth) + "x" + StringUtilities::ToString(currHeight) + m_FileType;

                        currHeight /= 2;
                        currWidth /= 2;

                        if(currHeight < 1 || currWidth < 1)
                            break;

                        String8 newPath;
                        if(!FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8StdS(envFiles[i]), &newPath))
                        {
                            LERROR("Failed to load %s", envFiles[i].c_str());
                            failed = true;
                            break;
                        }
                    }

                    currWidth  = (uint32_t)((float)m_Width * m_IrrFactor);
                    currHeight = (uint32_t)((float)m_Height * m_IrrFactor);

                    int numMipsIrr = 0;

                    for(uint32_t i = 0; i < m_NumMips; i++)
                    {
                        irrFiles[i] = m_FilePath + "_Irr_" + StringUtilities::ToString(i) + "_" + StringUtilities::ToString(currWidth) + "x" + StringUtilities::ToString(currHeight) + m_FileType;

                        currHeight /= 2;
                        currWidth /= 2;

                        if(currHeight < 1 || currWidth < 1)
                            break;

                        String8 newPath;
                        if(!FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8StdS(irrFiles[i]), &newPath))
                        {
                            LERROR("Failed to load %s", irrFiles[i].c_str());
                            failed = true;
                            break;
                        }

                        numMipsIrr++;
                    }

                    if(!failed)
                    {
                        TextureDesc params;
                        params.srgb = true;
                        TextureLoadOptions loadOptions;
                        m_Environmnet   = SharedPtr<TextureCube>(Graphics::TextureCube::CreateFromVCross(envFiles, m_NumMips, params, loadOptions));
                        m_IrradianceMap = SharedPtr<TextureCube>(Graphics::TextureCube::CreateFromVCross(irrFiles, numMipsIrr, params, loadOptions));
                    }
                    else
                    {
                        LERROR("Failed to load environment");
                    }

                    ScratchEnd(Scratch);
                }
            }
            else // if (m_Mode == 1)
            {
                m_Parameters.w = m_Mode;
                Application::Get().GetSceneRenderer()->CreateCubeMap("", m_Parameters, m_Environmnet, m_IrradianceMap);
            }

            delete[] envFiles;
            delete[] irrFiles;
        }

        Environment::~Environment()
        {
        }

        void Environment::SetEnvironmnet(TextureCube* environmnet)
        {
            m_Environmnet = SharedPtr<TextureCube>(environmnet);
        }

        void Environment::SetPrefilteredEnvironment(TextureCube* prefilteredEnvironment)
        {
            m_PrefilteredEnvironment = SharedPtr<TextureCube>(prefilteredEnvironment);
        }

        void Environment::SetIrradianceMap(TextureCube* irradianceMap)
        {
            m_IrradianceMap = SharedPtr<TextureCube>(irradianceMap);
        }
    }
}
