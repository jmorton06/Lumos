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
