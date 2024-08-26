#pragma once

#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
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
            Light(const Vec3& direction = Vec3(0.0f), const Vec4& colour = Vec4(1.0f), float intensity = 120000.0f, const LightType& type = LightType::DirectionalLight, const Vec3& position = Vec3(), float radius = 1.0f, float angle = 0.0f);

            void OnImGui();
            static std::string LightTypeToString(Graphics::LightType type);
            static float StringToLightType(const std::string& type);

            Vec4 Colour;
            Vec4 Position;
            Vec4 Direction;
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
