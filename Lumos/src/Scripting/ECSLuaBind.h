#pragma once

namespace sol
{
	class state;
}

namespace Lumos::Scripting
{
	void BindECSLua(sol::state* state);
}