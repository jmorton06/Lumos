#pragma once
#include "LM.h"

namespace Lumos
{

	namespace System
	{
		class LUMOS_EXPORT CFG
		{

		public:
			explicit CFG(const String& filename);
			void Init(const String& filename);
			void Parse();

			bool FindBool(const String& name, bool& value);
			bool FindFloat(const String& name, float& value);
			bool FindString(const String& name, String& value);
			bool FindInt(const String& name, int& value);
			bool FindUInt(const String& name, uint& value);

		private:
			String m_CFGFilename;
			std::map<String, String> m_CFGMap;
			std::stringstream m_Buffer;
		};
	}
}

