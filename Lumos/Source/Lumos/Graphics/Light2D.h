#pragma once

#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class LUMOS_EXPORT Light2DType
        {
            Point  = 0,
            Spot   = 1,
            Global = 2 // ambient / directional, lights everything
        };

        // Forward 2D light. Position/Direction are filled from the entity Transform each frame.
        struct LUMOS_EXPORT MEM_ALIGN Light2D
        {
            Light2D(const Vec4& colour = Vec4(1.0f), float intensity = 1.0f, float radius = 5.0f, const Light2DType& type = Light2DType::Point);

            void OnImGui();
            static std::string LightTypeToString(Graphics::Light2DType type);

            Vec4 Colour;
            Vec4 Position;  // xy = world position (z unused)
            Vec4 Direction; // xy = facing dir (spot), z used for global
            float Intensity;
            float Radius;
            float Type;
            float Height;     // virtual z height above the sprite plane, drives normal-map shading
            float InnerAngle; // spot cone, cos of inner angle (full bright)
            float OuterAngle; // spot cone, cos of outer angle (cutoff)
            float Falloff;    // >1 sharpens the radial attenuation curve
            float _pad;
        };

        template <class Archive>
        void serialize(Archive& archive, Graphics::Light2D& light)
        {
            archive(light.Colour, light.Intensity, light.Radius, light.Type,
                    light.Height, light.InnerAngle, light.OuterAngle, light.Falloff);
        }
    }
}
