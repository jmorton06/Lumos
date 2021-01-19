#pragma once
#include "Core/Core.h"

#include <string>

namespace Lumos
{
    namespace Maths
    {
        class Vector4;
    }
	enum PowerState
	{
		POWERSTATE_UNKNOWN,
		POWERSTATE_ON_BATTERY,
		POWERSTATE_NO_BATTERY,
		POWERSTATE_CHARGING,
		POWERSTATE_CHARGED
	};
    
	class LUMOS_EXPORT OS
	{
        public:
		OS() = default;
		virtual ~OS() = default;
        
		virtual void Run() = 0;
        
		static void Create();
		static void Release();
		static void SetInstance(OS* instance)
		{
			s_Instance = instance;
		}
        
		static OS* Instance()
		{
			return s_Instance;
		}
		static std::string PowerStateToString(PowerState state);
        
		virtual std::string GetExecutablePath() = 0;
		virtual std::string GetAssetPath()
		{
			return "";
		};
		virtual void Vibrate() const {};
        virtual void SetTitleBarColour(const Maths::Vector4& colour, bool dark = true) {};
        
		//Mobile only
		virtual void ShowKeyboard(){};
		virtual void HideKeyboard(){};
        
        protected:
		static OS* s_Instance;
	};
}
