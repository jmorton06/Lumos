#pragma once

#include "Maths/Maths.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class LUMOS_EXPORT LightType
        {
            DirectionalLight = 0,
            SpotLight        = 1,
            PointLight       = 2
        };

        struct LUMOS_EXPORT MEM_ALIGN Light
        {
            Light(const glm::vec3& direction = glm::vec3(0.0f), const glm::vec4& colour = glm::vec4(1.0f), float intensity = 120000.0f, const LightType& type = LightType::DirectionalLight, const glm::vec3& position = glm::vec3(), float radius = 1.0f, float angle = 0.0f);

            void OnImGui();
            static std::string LightTypeToString(Graphics::LightType type);
            static float StringToLightType(const std::string& type);

            glm::vec4 Colour;
            glm::vec4 Position;
            glm::vec4 Direction;
            float Intensity;
            float Radius;
            float Type;
            float Angle;
        };

        template <class Archive>
        void serialize(Archive& archive, Graphics::Light& light)
        {
            archive(light.Position, light.Colour, light.Type, light.Angle, light.Direction, light.Intensity, light.Radius);
        }

    }
}
