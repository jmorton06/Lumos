#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Transform.h"

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent : public LumosComponent
	{
	public:
		explicit TransformComponent();
		explicit TransformComponent(const Maths::Matrix4& matrix);

        void SetWorldMatrix(const Maths::Matrix4& matrix);
		void OnUpdateComponent(float dt) override;
        void OnIMGUI() override;
		
		Maths::Transform& GetTransform() { return m_Transform; }

		nlohmann::json Serialise() override
        {
            nlohmann::json output;
            output["transform"] = m_Transform.Serialise();
            return output;
            
        };
		void Deserialise(nlohmann::json& data) override
        {
            m_Transform.Deserialise(data["transform"]);
        };

    private:
		Maths::Transform m_Transform;
	};
}
