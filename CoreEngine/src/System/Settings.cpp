#include "JM.h"
#include "Settings.h"
#include "JMLog.h"

namespace jm
{

	namespace System
	{
		CFG::CFG(const String& filename)
		{
			Init(filename);
		}

		void CFG::Init(const String& filename)
		{
			if (filename.empty())
			{
				JM_CORE_ERROR("Failed to Open : ", filename);
				return;
			}

			std::ifstream fileIn(filename);
			if (!fileIn.is_open())
			{
				JM_CORE_ERROR("Failed to Open : ", filename);
				return;
			}

			m_Buffer << fileIn.rdbuf();

			m_CFGFilename = std::move(filename);
			// 'filename' is now empty. It's contents were take by 'm_CFGFilename'

			Parse();
		}

		void CFG::Parse()
		{
			String line;
			std::istringstream s(m_Buffer.str());
			String commentKey("#");

			while (std::getline(s, line))
			{
				if (line.empty() || line[0] == '#')
				{
					continue;
				}

				// Split the line
				std::vector<String> seglist;
				String segment;
				std::istringstream ss(line);


				while (std::getline(ss, segment, '='))
				{
					// Trim the white space
					std::stringstream trimmer;
					trimmer << segment;
					segment.clear();
					trimmer >> segment;

					//std::transform(segment.begin(), segment.end(), segment.begin(), ::tolower);

					seglist.push_back(segment);
				}

				// Only 
				if (seglist.size() == 2)
				{
					m_CFGMap[seglist[0]] = seglist[1];
				}
			}
		}

		bool CFG::FindBool(const String& name, bool& value)
		{
			std::map<String, String>::iterator it;

			it = m_CFGMap.find(name);
			if (it != m_CFGMap.end())
			{
				if (it->second == "true")
					value = true;
				else if (it->second == "false")
					value = false;
				else
					JM_CORE_ERROR("Invalid value set to name - ", it->second);

				return true;
			}
			else
			{
				JM_CORE_ERROR("Couldn't Find value - ", name, " in Settings File");
				return false;
			}
		}

		bool CFG::FindString(const String& name, String& value)
		{
			const std::map<String, String>::iterator it = m_CFGMap.find(name);
			if (it != m_CFGMap.end())
			{
				value = it->second;
				return true;
			}
			else
			{
				JM_CORE_ERROR("Couldn't Find value - ", name, " in Settings File");
				return false;
			}
		}

		bool CFG::FindInt(const String& name, int& value)
		{
			const std::map<String, String>::iterator it = m_CFGMap.find(name);
			if (it != m_CFGMap.end())
			{
#ifdef JM_PLATFORM_MOBILE
				value = 0;
#else
				value = std::stoi(it->second.c_str());
#endif
				return true;
			}
			else
			{
				JM_CORE_ERROR("Couldn't Find value - ", name, " in Settings File");
				return false;
			}
		}

		bool CFG::FindUInt(const String& name, uint& value)
		{
			const std::map<String, String>::iterator it = m_CFGMap.find(name);
			if (it != m_CFGMap.end())
			{
#ifdef JM_PLATFORM_MOBILE
				value = 0;
#else
				value = std::stoi(it->second.c_str());
#endif

				return true;
			}
			else
			{
				JM_CORE_ERROR("Couldn't Find value - ", name, " in Settings File");
				return false;
			}
		}

		bool CFG::FindFloat(const String& name, float& value)
		{
			const std::map<String, String>::iterator it = m_CFGMap.find(name);
			if (it != m_CFGMap.end())
			{
#ifdef JM_PLATFORM_MOBILE
				value = 0.0f;
#else
				value = std::stof(it->second.c_str());
#endif
				return true;
			}
			else
			{
				JM_CORE_ERROR("Couldn't Find value - ", name, " in Settings File");
				return false;
			}
		}
	}
}