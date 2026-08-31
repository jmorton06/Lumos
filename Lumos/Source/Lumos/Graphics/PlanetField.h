#pragma once
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    namespace Graphics
    {
        class StorageBuffer;

        // One procedural planet, packed as 4 vec4 to match Planet.vert's SSBO.
        struct PlanetInstance
        {
            Vec4 PosRadius; // xyz centre, w radius (world units)
            Vec4 Star;      // xyz star world pos (light), w type (0 terrestrial, 1 gas, 2 procedural)
            Vec4 Tint;      // rgb base colour, w seed
            Vec4 Params;    // x axial tilt, y cloud amount, z spin rate, w rings (0 = none)
        };

        TDArray<PlanetInstance>& GetPlanetInstances();
        void AddPlanet(const Vec3& pos, float radius, const Vec3& starPos, int type,
                       const Vec4& tint, float seed, float cloud = 0.5f, float spin = 0.1f,
                       float tilt = 0.0f, bool rings = false);
        void ClearPlanets();

        StorageBuffer* PlanetUploadBuffer(uint32_t frameIndex);
        void PlanetReleaseBuffer();

        void SetSystemFrame(bool enabled, const Vec3& camLocal);
        bool SystemFrameEnabled();
        Vec3 SystemFrameCamLocal();
    }
}
