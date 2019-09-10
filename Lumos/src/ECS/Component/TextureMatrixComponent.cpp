#include "LM.h"
#include "TextureMatrixComponent.h"
#include <imgui/imgui.h>

namespace Lumos
{
	TextureMatrixComponent::TextureMatrixComponent(const Maths::Matrix4& matrix)
		: m_TextureMatrix(matrix)
	{
		m_Name = "TextureMatrix";
	}

	void TextureMatrixComponent::OnImGui()
	{
	}

	nlohmann::json TextureMatrixComponent::Serialise()
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(TextureMatrixComponent);
		output["textureMatrix"] = m_TextureMatrix.Serialise();
		output["active"] = m_Active;

		return output;
	}

	void TextureMatrixComponent::Deserialise(nlohmann::json & data)
	{
		m_TextureMatrix.Deserialise(data["textureMatrix"]);
		m_Active = data["active"];
	}

}
