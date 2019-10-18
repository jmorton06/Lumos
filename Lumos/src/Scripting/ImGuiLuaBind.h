#pragma once

namespace sol
{
	class state;
}

namespace Lumos::Scripting
{
	void BindImGuiLua(sol::state* solState);
}