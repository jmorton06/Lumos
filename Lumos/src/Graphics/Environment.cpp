#include "lmpch.h"
#include "Environment.h"

#include "API/Texture.h"

namespace Lumos
{
    namespace Graphics
    {
        Environment::Environment()
        {
            m_Environmnet = nullptr;
            m_PrefilteredEnvironment = nullptr;
            m_IrradianceMap = nullptr;
        }

        Environment::Environment(const String &filepath, bool genPrefilter, bool genIrradiance)
        {
            
        }
        
        Environment::Environment(const String& name, u32 numMip, u32 width, u32 height, const String& fileType)
        {
            String* envFiles = new String[numMip];
            String* irrFiles = new String[numMip];
        
            u32 currWidth = width;
            u32 currHeight = height;

            for(int i = 0; i < numMip; i++)
            {
                envFiles[i] = name + "_Env_" + StringFormat::ToString(i) + "_" + StringFormat::ToString(currWidth) + "x" + StringFormat::ToString(currHeight) + fileType;
                irrFiles[i] = name + "_Irr_" + StringFormat::ToString(i) + "_" + StringFormat::ToString(currWidth) + "x" + StringFormat::ToString(currHeight) + fileType;
            
                currHeight /= 2;
                currWidth /= 2;
            }
        
            m_Environmnet = Graphics::TextureCube::CreateFromVCross(envFiles, numMip);
            m_IrradianceMap = Graphics::TextureCube::CreateFromVCross(irrFiles, numMip);
        }

        Environment::~Environment()
        {
        }

        void Environment::SetEnvironmnet(TextureCube* environmnet)
        {
            m_Environmnet = Ref<TextureCube>(environmnet);
        }

        void Environment::SetPrefilteredEnvironment(TextureCube *prefilteredEnvironment)
        {
            m_PrefilteredEnvironment = Ref<TextureCube>(prefilteredEnvironment);
        }

        void Environment::SetIrradianceMap(TextureCube *irradianceMap)
        {
            m_IrradianceMap = Ref<TextureCube>(irradianceMap);
        }
    }
}
