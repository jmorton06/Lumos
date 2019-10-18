#pragma once

namespace sol
{
	class state;
}

namespace Lumos
{
	namespace Scripting
	{
		void BindMathsLua(sol::state* state);
	}
}