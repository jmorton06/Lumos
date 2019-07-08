#pragma once
#include "LM.h"
#include "Entity/Entity.h"
#include <set>

namespace Lumos
{
	struct TimeStep;
    class Entity;
    
    constexpr u8 MAX_COMPONENTS = 1000;
    // A simple type alias
    using Signature = std::bitset<MAX_COMPONENTS>;

	class LUMOS_EXPORT ISystem
	{
	public:
		ISystem() = default;
		virtual ~ISystem() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate(TimeStep* dt) = 0;
		virtual void OnIMGUI() = 0;
        
        inline const String& GetName() const { return m_DebugName; }
        
        std::set<Entity*> m_Entities;
    protected:
        String m_DebugName;
	};
}
