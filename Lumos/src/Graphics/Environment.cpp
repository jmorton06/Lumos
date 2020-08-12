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

		Environment::Environment(const std::string& filepath, bool genPrefilter, bool genIrradiance)
		{
		}

		Environment::Environment(const std::string& name, u32 numMip, u32 width, u32 height, const std::string& fileType)
		{
			m_Width = width;
			m_Height = height;
			m_NumMips = numMip;
			m_FilePath = name;
			m_FileType = fileType;

			Load();
		}

		void Environment::Load(const std::string& name, u32 numMip, u32 width, u32 height, const std::string& fileType)
		{
			m_Width = width;
			m_Height = height;
			m_NumMips = numMip;
			m_FilePath = name;
			m_FileType = fileType;

			Load();
		}

		void Environment::Load()
		{
			std::string* envFiles = new std::string[m_NumMips];
			std::string* irrFiles = new std::string[m_NumMips];

			u32 currWidth = m_Width;
			u32 currHeight = m_Height;

			for(u32 i = 0; i < m_NumMips; i++)
			{
				envFiles[i] = m_FilePath + "_Env_" + StringFormat::ToString(i) + "_" + StringFormat::ToString(currWidth) + "x" + StringFormat::ToString(currHeight) + m_FileType;
				irrFiles[i] = m_FilePath + "_Irr_" + StringFormat::ToString(i) + "_" + StringFormat::ToString(currWidth) + "x" + StringFormat::ToString(currHeight) + m_FileType;

				currHeight /= 2;
				currWidth /= 2;
			}

			m_Environmnet = Graphics::TextureCube::CreateFromVCross(envFiles, m_NumMips);
			m_IrradianceMap = Graphics::TextureCube::CreateFromVCross(irrFiles, m_NumMips);

			delete[] envFiles;
			delete[] irrFiles;
		}

		Environment::~Environment()
		{
		}

		void Environment::SetEnvironmnet(TextureCube* environmnet)
		{
			m_Environmnet = Ref<TextureCube>(environmnet);
		}

		void Environment::SetPrefilteredEnvironment(TextureCube* prefilteredEnvironment)
		{
			m_PrefilteredEnvironment = Ref<TextureCube>(prefilteredEnvironment);
		}

		void Environment::SetIrradianceMap(TextureCube* irradianceMap)
		{
			m_IrradianceMap = Ref<TextureCube>(irradianceMap);
		}
	}
}
