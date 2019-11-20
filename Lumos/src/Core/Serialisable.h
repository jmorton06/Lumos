#pragma once
#include "lmpch.h"

#include "Typename.h"
#include <jsonhpp/json.hpp>

namespace Lumos
{
	class Serialisable
	{
	public:
		virtual nlohmann::json Serialise() = 0;
		virtual void Deserialise(nlohmann::json& data) = 0;
		template <typename T> bool IsType();
	};

	template<typename T>
	_FORCE_INLINE_ bool Serialisable::IsType()
	{
		return Serialise()["typeID"] == LUMOS_TYPENAME(T);
	}
}
