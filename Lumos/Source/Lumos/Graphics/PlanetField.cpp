#include "Precompiled.h"
#include "PlanetField.h"
#include "Graphics/RHI/StorageBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        static TDArray<PlanetInstance> s_Planets;
        static constexpr uint32_t kPlanetBufferCount        = 3;
        static StorageBuffer* s_Buffers[kPlanetBufferCount] = { nullptr, nullptr, nullptr };

        TDArray<PlanetInstance>& GetPlanetInstances()
        {
            return s_Planets;
        }

        void AddPlanet(const Vec3& pos, float radius, const Vec3& starPos, int type,
                       const Vec4& tint, float seed, float cloud, float spin,
                       float tilt, bool rings)
        {
            PlanetInstance p;
            p.PosRadius = Vec4(pos, radius);
            p.Star      = Vec4(starPos, (float)type);
            p.Tint      = Vec4(tint.x, tint.y, tint.z, seed);
            p.Params    = Vec4(tilt, cloud, spin, rings ? 1.0f : 0.0f);
            s_Planets.PushBack(p);
        }

        void ClearPlanets()
        {
            s_Planets.Clear();
        }

        // Rebuild this frame's SSBO from the current planets and return it, or null.
        StorageBuffer* PlanetUploadBuffer(uint32_t frameIndex)
        {
            uint32_t count = (uint32_t)s_Planets.Size();
            if(count == 0)
                return nullptr;

            StorageBuffer*& buf = s_Buffers[frameIndex % kPlanetBufferCount];
            uint32_t size       = count * sizeof(PlanetInstance);
            if(!buf)
                buf = StorageBuffer::Create(size, s_Planets.Data());
            else
                buf->Resize(size, s_Planets.Data());
            return buf;
        }

        void PlanetReleaseBuffer()
        {
            for(uint32_t i = 0; i < kPlanetBufferCount; i++)
            {
                delete s_Buffers[i];
                s_Buffers[i] = nullptr;
            }
        }

        static bool s_SystemFrameEnabled = false;
        static Vec3 s_SystemCamLocal     = Vec3(0.0f);

        void SetSystemFrame(bool enabled, const Vec3& camLocal)
        {
            s_SystemFrameEnabled = enabled;
            s_SystemCamLocal     = camLocal;
        }

        bool SystemFrameEnabled() { return s_SystemFrameEnabled; }
        Vec3 SystemFrameCamLocal() { return s_SystemCamLocal; }
    }
}
