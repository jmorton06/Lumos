#include "Precompiled.h"
#include "Environment.h"

#include "API/Texture.h"
#include "Core/VFS.h"

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
			
			bool failed = false;

			for(u32 i = 0; i < m_NumMips; i++)
			{
				envFiles[i] = m_FilePath + "_Env_" + StringUtilities::ToString(i) + "_" + StringUtilities::ToString(currWidth) + "x" + StringUtilities::ToString(currHeight) + m_FileType;
				irrFiles[i] = m_FilePath + "_Irr_" + StringUtilities::ToString(i) + "_" + StringUtilities::ToString(currWidth) + "x" + StringUtilities::ToString(currHeight) + m_FileType;

				currHeight /= 2;
				currWidth /= 2;
				
				std::string newPath;
				if(!VFS::Get()->ResolvePhysicalPath(envFiles[i], newPath))
				{
					LUMOS_LOG_ERROR("Failed to load {0}", envFiles[i]);
					failed = true;
					break;
				}
				
				if(!VFS::Get()->ResolvePhysicalPath(irrFiles[i], newPath))
				{
					LUMOS_LOG_ERROR("Failed to load {0}", irrFiles[i]);
					failed = true;
					break;
				}
			}
			
			if(!failed)
			{
                TextureParameters params;
                params.srgb = true;
                TextureLoadOptions loadOptions;
                m_Environmnet = Graphics::TextureCube::CreateFromVCross(envFiles, m_NumMips, params, loadOptions);
                m_IrradianceMap = Graphics::TextureCube::CreateFromVCross(irrFiles, m_NumMips, params, loadOptions);
			}
			else
			{
				LUMOS_LOG_ERROR("Failed to load environment");
			}

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
