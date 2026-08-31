#include "Precompiled.h"
#include "LineCloud.h"
#include "Graphics/RHI/StorageBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        LineCloud::~LineCloud()
        {
            delete m_Buffer;
        }

        void LineCloud::AddLine(const Vec3& a, const Vec3& b, const Vec4& colour, float width,
                                float dashPeriod, float duty, float arcAtA, float widthB)
        {
            LineCloudSegment seg;
            seg.A      = Vec4(a, width);
            seg.B      = Vec4(b, arcAtA);
            seg.Colour = colour;
            seg.Params = Vec4(dashPeriod, duty, widthB, 0.0f);
            m_Segments.PushBack(seg);
        }

        void LineCloud::Finish()
        {
            m_Count = (uint32_t)m_Segments.Size();
            if(m_Count == 0)
                return;

            uint32_t size = m_Count * sizeof(LineCloudSegment);
            if(!m_Buffer)
                m_Buffer = StorageBuffer::Create(size, m_Segments.Data());
            else
                m_Buffer->Resize(size, m_Segments.Data());
        }

        void LineCloud::Clear()
        {
            m_Segments.Clear();
            m_Count = 0;
        }

        void LineCloud::ReleaseBuffer()
        {
            delete m_Buffer;
            m_Buffer = nullptr;
            m_Count  = 0;
            m_Segments.Clear();
        }

        static TDArray<SharedPtr<LineCloud>> s_ActiveLineClouds;

        TDArray<SharedPtr<LineCloud>>& GetActiveLineClouds()
        {
            return s_ActiveLineClouds;
        }

        void RegisterLineCloud(const SharedPtr<LineCloud>& cloud)
        {
            s_ActiveLineClouds.PushBack(cloud);
        }

        void ClearLineClouds()
        {
            s_ActiveLineClouds.Clear();
        }

        static LineCloud s_ImmediateLines[2];
        static bool s_ImmediateInit = false;

        void DrawImmediateLine(const Vec3& a, const Vec3& b, const Vec4& colour, float width,
                               bool local, float dashPeriod, float duty, float widthB, float arcAtA)
        {
            if(!s_ImmediateInit)
            {
                s_ImmediateLines[1].SetLocal(true);
                s_ImmediateInit = true;
            }
            s_ImmediateLines[local ? 1 : 0].AddLine(a, b, colour, width, dashPeriod, duty, arcAtA, widthB);
        }

        LineCloud* GetImmediateLineCloud(bool local)
        {
            return &s_ImmediateLines[local ? 1 : 0];
        }

        void ResetImmediateLineClouds()
        {
            s_ImmediateLines[0].Clear();
            s_ImmediateLines[1].Clear();
        }

        void ReleaseImmediateLineClouds()
        {
            s_ImmediateLines[0].ReleaseBuffer();
            s_ImmediateLines[1].ReleaseBuffer();
        }
    }
}
