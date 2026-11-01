#pragma once

#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Scene/Serialisation/Serialisation.h"
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

        enum class LUMOS_EXPORT LightShadowUpdate : u8
        {
            EveryFrame = 0,
            OnChange   = 1 // redrawn when the light, or something in its reach, moves
        };

        // What the shaders see of a light (struct Light in Buffers.glslh): Light starts with the same layout.
        // Direction.w carries the spot shadow slot (-1 = none).
        struct LUMOS_EXPORT MEM_ALIGN GPULight
        {
            Vec4 Colour;
            Vec4 Position;
            Vec4 Direction;
            float Intensity;
            float Radius;
            float Type;
            float Angle;
        };

        struct LUMOS_EXPORT MEM_ALIGN Light
        {
            Light(const Vec3& direction = Vec3(0.0f), const Vec4& colour = Vec4(1.0f), float intensity = 120000.0f, const LightType& type = LightType::DirectionalLight, const Vec3& position = Vec3(), float radius = 1.0f, float angle = 0.0f);

            void OnImGui();
            static std::string LightTypeToString(Graphics::LightType type);
            static float StringToLightType(const std::string& type);

            GPULight ToGPU() const
            {
                GPULight l;
                l.Colour    = Colour;
                l.Position  = Position;
                l.Direction = Direction;
                l.Intensity = Intensity;
                l.Radius    = Radius;
                l.Type      = Type;
                l.Angle     = Angle;
                return l;
            }

            Vec4 Colour;
            Vec4 Position;
            Vec4 Direction;
            float Intensity;
            float Radius;
            float Type;
            float Angle;

            // Shadows (spot lights; the first directional light always has the cascades)
            bool CastShadows       = false;
            u8 ShadowResolution    = 0; // 0 = by size on screen, 1..4 = 256 / 512 / 1024 / 2048 (capped by the scene setting)
            u8 ShadowUpdate        = 0; // LightShadowUpdate
            float ShadowBias       = 1.0f;  // shadow texels
            float ShadowNormalBias = 1.0f;  // shadow texels
            float ShadowNear       = 0.05f; // metres
        };

        template <class Archive>
        void save(Archive& archive, const Graphics::Light& light)
        {
            archive(light.Position, light.Colour, light.Type, light.Angle, light.Direction, light.Intensity, light.Radius);
            archive(light.CastShadows, light.ShadowResolution, light.ShadowUpdate, light.ShadowBias, light.ShadowNormalBias, light.ShadowNear);
        }

        template <class Archive>
        void load(Archive& archive, Graphics::Light& light)
        {
            archive(light.Position, light.Colour, light.Type, light.Angle, light.Direction, light.Intensity, light.Radius);
            if(Serialisation::CurrentSceneVersion > 40)
                archive(light.CastShadows, light.ShadowResolution, light.ShadowUpdate, light.ShadowBias, light.ShadowNormalBias, light.ShadowNear);
        }

    }
}
