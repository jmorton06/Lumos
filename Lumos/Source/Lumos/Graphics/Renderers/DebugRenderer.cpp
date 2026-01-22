#include "Precompiled.h"
#include "DebugRenderer.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Audio/SoundNode.h"
#include "Graphics/Camera/Camera.h"
#include "Maths/Transform.h"
#include "Maths/Frustum.h"
#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Ray.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"

#include <cstdarg>
#include <stb/stb_sprintf.h>

namespace Lumos
{
    using namespace Graphics;

    DebugRenderer* DebugRenderer::s_Instance = nullptr;

    static const uint32_t MaxLines        = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices  = MaxLines * 6;
#define MAX_BATCH_DRAW_CALLS 100
#define RENDERER_LINE_SIZE RENDERER2DLINE_VERTEX_SIZE * 4
#define RENDERER_BUFFER_SIZE RENDERER_LINE_SIZE* MaxLineVertices

#ifdef LUMOS_PLATFORM_WINDOWS
#define VSNPRINTF(...) stbsp_vsnprintf(__VA_ARGS__)
#elif LUMOS_PLATFORM_MACOS
#define VSNPRINTF(...) stbsp_vsnprintf(__VA_ARGS__)
#elif LUMOS_PLATFORM_LINUX
#define VSNPRINTF(...) stbsp_vsnprintf(__VA_ARGS__)
#elif LUMOS_PLATFORM_MOBILE
#define VSNPRINTF(...) stbsp_vsnprintf(__VA_ARGS__)
#else
#define VSNPRINTF(...) 0
#endif

    void DebugRenderer::Init()
    {
        if(s_Instance)
            return;

        s_Instance = new DebugRenderer();
    }

    void DebugRenderer::Release()
    {
        LUMOS_PROFILE_FUNCTION();
        delete s_Instance;
        s_Instance = nullptr;
    }

    void DebugRenderer::Reset(float dt)
    {
        LUMOS_PROFILE_FUNCTION();
        s_Instance->ClearDrawList(s_Instance->m_DrawList, dt);
        s_Instance->ClearDrawList(s_Instance->m_DrawListNDT, dt);

        s_Instance->m_TextList.Clear();
        s_Instance->m_TextListNDT.Clear();
        s_Instance->m_TextListCS.Clear();
        s_Instance->m_NumStatusEntries    = 0;
        s_Instance->m_MaxStatusEntryWidth = 0.0f;
    }

    void DebugRenderer::ClearDrawList(DebugDrawList& drawlist, float dt)
    {
        drawlist.m_DebugTriangles.RemoveIf([dt](TriangleInfo& triangle)
                                           {
            triangle.time -= dt;
            return triangle.time <= Maths::M_EPSILON; });

        drawlist.m_DebugLines.RemoveIf([dt](LineInfo& line)
                                       {
            line.time -= dt;
            return line.time <= Maths::M_EPSILON; });

        drawlist.m_DebugThickLines.RemoveIf([dt](LineInfo& line)
                                            {
            line.time -= dt;
            return line.time <= Maths::M_EPSILON; });

        drawlist.m_DebugPoints.RemoveIf([dt](PointInfo& point)
                                        {
            point.time -= dt;
            return point.time <= Maths::M_EPSILON; });
    }

    void DebugRenderer::ClearLogEntries()
    {
        s_Instance->m_vLogEntries.Clear();
        s_Instance->m_LogEntriesOffset = 0;
    }

    void DebugRenderer::SortLists()
    {
        float cs_size_x = LOG_TEXT_SIZE / s_Instance->m_Width * 2.0f;
        float cs_size_y = LOG_TEXT_SIZE / s_Instance->m_Height * 2.0f;
        size_t log_len  = s_Instance->m_vLogEntries.Size();

        float max_x = 0.0f;
        for(size_t i = 0; i < log_len; ++i)
        {
            max_x = Maths::Max(max_x, s_Instance->m_vLogEntries[i].text.length() * cs_size_x * 0.6f);

            size_t idx                              = (i + s_Instance->m_LogEntriesOffset) % MAX_LOG_SIZE;
            float alpha                             = 1.0f - ((float)log_len - (float)i) / (float)log_len;
            s_Instance->m_vLogEntries[idx].colour.w = alpha;
            float aspect                            = (float)s_Instance->m_Width / (float)s_Instance->m_Height;
            DrawTextCs(Vec4(-aspect, -1.0f + ((log_len - i - 1) * cs_size_y) + cs_size_y, 0.0f, 1.0f), LOG_TEXT_SIZE, s_Instance->m_vLogEntries[idx].text, s_Instance->m_vLogEntries[idx].colour);
        }
    }

    DebugRenderer::DebugRenderer()
    {
        m_vLogEntries.Clear();
        m_LogEntriesOffset = 0;
    }

    DebugRenderer::~DebugRenderer()
    {
    }

    // Draw Point (circle)
    void DebugRenderer::GenDrawPoint(bool ndt, const Vec3& pos, float point_radius, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugPoints.EmplaceBack(pos, point_radius, colour, time);
        else
            s_Instance->m_DrawList.m_DebugPoints.EmplaceBack(pos, point_radius, colour, time);
    }

    void DebugRenderer::DrawPoint(const Vec3& pos, float point_radius, bool depthTested, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawPoint(!depthTested, pos, point_radius, colour, time);
    }

    // Draw Line with a given thickness
    void DebugRenderer::GenDrawThickLine(bool ndt, const Vec3& start, const Vec3& end, float line_width, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugThickLines.EmplaceBack(start, end, colour, time);
        else
            s_Instance->m_DrawList.m_DebugThickLines.EmplaceBack(start, end, colour, time);
    }

    void DebugRenderer::DrawThickLine(const Vec3& start, const Vec3& end, float line_width, bool depthTested, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawThickLine(!depthTested, start, end, line_width, colour, time);
    }

    // Draw line with thickness of 1 screen pixel regardless of distance from camera
    void DebugRenderer::GenDrawHairLine(bool ndt, const Vec3& start, const Vec3& end, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugLines.EmplaceBack(start, end, colour, time);
        else
            s_Instance->m_DrawList.m_DebugLines.EmplaceBack(start, end, colour, time);
    }

    void DebugRenderer::DrawHairLine(const Vec3& start, const Vec3& end, bool depthTested, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(!depthTested, start, end, colour, time);
    }

    // Draw Matrix (x,y,z axis at pos)
    void DebugRenderer::DrawMatrix(const Mat4& mtx, bool depthTested, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        Vec3 position = Vec3(mtx[13], mtx[14], mtx[15]);
        GenDrawHairLine(!depthTested, position, position + Vec3(mtx[0], mtx[1], mtx[2]), Vec4(1.0f, 0.0f, 0.0f, 1.0f), time);
        GenDrawHairLine(!depthTested, position, position + Vec3(mtx[4], mtx[5], mtx[6]), Vec4(0.0f, 1.0f, 0.0f, 1.0f), time);
        GenDrawHairLine(!depthTested, position, position + Vec3(mtx[8], mtx[9], mtx[10]), Vec4(0.0f, 0.0f, 1.0f, 1.0f), time);
    }
    void DebugRenderer::DrawMatrix(const Mat3& mtx, const Vec3& position, bool depthTested, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        // GenDrawHairLine(!depthTested, position, position + Vec3(mtx[0], mtx[1], mtx[2]), Vec4(1.0f, 0.0f, 0.0f, 1.0f), time);
        // GenDrawHairLine(!depthTested, position, position + Vec3(mtx[4], mtx[5], mtx[6]), Vec4(0.0f, 1.0f, 0.0f, 1.0f), time);
        // GenDrawHairLine(!depthTested, position, position + Vec3(mtx[8], mtx[9], mtx[10]), Vec4(0.0f, 0.0f, 1.0f, 1.0f), time);
    }

    // Draw Triangle
    void DebugRenderer::GenDrawTriangle(bool ndt, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugTriangles.EmplaceBack(v0, v1, v2, colour, time);
        else
            s_Instance->m_DrawList.m_DebugTriangles.EmplaceBack(v0, v1, v2, colour, time);
    }

    void DebugRenderer::DrawTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, bool depthTested, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawTriangle(!depthTested, v0, v1, v2, colour, time);
    }

    // Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
    void DebugRenderer::DrawPolygon(int n_verts, const Vec3* verts, bool depthTested, const Vec4& colour, float time)
    {
        LUMOS_PROFILE_FUNCTION();
        for(int i = 2; i < n_verts; ++i)
        {
            GenDrawTriangle(!depthTested, verts[0], verts[i - 1], verts[i], colour, time);
        }
    }

    void DebugRenderer::DrawTextCs(const Vec4& cs_pos, const float font_size, const std::string& text, const Vec4& colour)
    {
        Vec3 cs_size = Vec3(font_size / GetInstance()->m_Width, font_size / GetInstance()->m_Height, 0.0f);
        cs_size      = cs_size * cs_pos.w;

        // Work out the starting position of text based off desired alignment
        float x_offset      = 0.0f;
        const auto text_len = static_cast<int>(text.length());

        DebugText& dText = GetInstance()->m_TextListCS.EmplaceBack();
        dText.text       = text;
        dText.Position   = cs_pos;
        dText.colour     = colour;
        dText.Size       = font_size;

        // Add each characters to the draw list individually
        // for (int i = 0; i < text_len; ++i)
        //{
        //     Vec4 char_pos = Vec4(cs_pos.x + x_offset, cs_pos.y, cs_pos.z, cs_pos.w);
        //     Vec4 char_data = Vec4(cs_size.x, cs_size.y, static_cast<float>(text[i]), 0.0f);

        //    GetInstance()->m_vChars.PushBack(char_pos);
        //    GetInstance()->m_vChars.PushBack(char_data);
        //    GetInstance()->m_vChars.PushBack(colour);
        //    GetInstance()->m_vChars.PushBack(colour);    //We dont really need this, but we need the padding to match the same vertex format as all the other debug drawables

        //    x_offset += cs_size.x * 1.2f;
        //}
    }

    // Draw Text WorldSpace
    void DebugRenderer::DrawTextWs(const Vec3& pos, const float font_size, bool depthTested, const Vec4& colour, float time, const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        std::string formatted_text = std::string(buf, static_cast<size_t>(length));

        // Vec4 cs_pos = GetInstance()->m_ProjViewMtx * Vec4(pos, 1.0f);
        // DrawTextCs(cs_pos, font_size, formatted_text, colour);

        DebugText& dText = depthTested ? GetInstance()->m_TextList.EmplaceBack() : GetInstance()->m_TextListNDT.EmplaceBack();
        dText.text       = formatted_text;
        dText.Position   = Vec4(pos, 1.0f);
        dText.colour     = colour;
        dText.Size       = font_size;
        dText.time       = time;
    }

    // Status Entry
    void DebugRenderer::AddStatusEntry(const Vec4& colour, const std::string text, ...)
    {
        float cs_size_x = STATUS_TEXT_SIZE / GetInstance()->m_Width * 2.0f;
        float cs_size_y = STATUS_TEXT_SIZE / GetInstance()->m_Height * 2.0f;

        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        std::string formatted_text = std::string(buf, static_cast<size_t>(length));

        DrawTextCs(Vec4(-1.0f + cs_size_x * 0.5f, 1.0f - (GetInstance()->m_NumStatusEntries * cs_size_y) + cs_size_y, -1.0f, 1.0f), STATUS_TEXT_SIZE, formatted_text, colour);
        GetInstance()->m_NumStatusEntries++;
        GetInstance()->m_MaxStatusEntryWidth = Maths::Max(GetInstance()->m_MaxStatusEntryWidth, cs_size_x * 0.6f * length);
    }

    // Log

    void DebugRenderer::AddLogEntry(const Vec3& colour, const std::string& text)
    {
        /*    time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);*/

        // std::stringstream ss;
        // ss << "[" << ltm.tm_hour << ":" << ltm.tm_min << ":" << ltm.tm_sec << "] ";

        LogEntry le;
        le.text   = /*ss.str() + */ text; // +"\n";
        le.colour = Vec4(colour.x, colour.y, colour.z, 1.0f);

        if(GetInstance()->m_vLogEntries.Size() < MAX_LOG_SIZE)
            GetInstance()->m_vLogEntries.PushBack(le);
        else
        {
            GetInstance()->m_vLogEntries[GetInstance()->m_LogEntriesOffset] = le;
            GetInstance()->m_LogEntriesOffset                               = (GetInstance()->m_LogEntriesOffset + 1) % MAX_LOG_SIZE;
        }

        LWARN(text.c_str());
    }

    void DebugRenderer::Log(const Vec3& colour, const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;
        AddLogEntry(colour, std::string(buf, static_cast<size_t>(length)));
    }

    void DebugRenderer::Log(const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;
        AddLogEntry(Vec3(0.4f, 1.0f, 0.6f), std::string(buf, static_cast<size_t>(length)));
    }

    void DebugRenderer::LogE(const char* filename, int linenumber, const std::string text, ...)
    {
        // Error Format:
        //<text>
        //         -> <line number> : <file name>

        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        Log(Vec3(1.0f, 0.25f, 0.25f), "[ERROR] %s:%d", filename, linenumber);
        AddLogEntry(Vec3(1.0f, 0.5f, 0.5f), "\t \x01 \"" + std::string(buf, static_cast<size_t>(length)) + "\"");

        std::cout << std::endl;
    }

    void DebugRenderer::DebugDraw(const Maths::BoundingBox& box, const Vec4& edgeColour, bool cornersOnly, bool depthTested, float width)
    {
        LUMOS_PROFILE_FUNCTION();
        Vec3 uuu = box.Max();
        Vec3 lll = box.Min();

        Vec3 ull(uuu.x, lll.y, lll.z);
        Vec3 uul(uuu.x, uuu.y, lll.z);
        Vec3 ulu(uuu.x, lll.y, uuu.z);

        Vec3 luu(lll.x, uuu.y, uuu.z);
        Vec3 llu(lll.x, lll.y, uuu.z);
        Vec3 lul(lll.x, uuu.y, lll.z);

        // Draw edges
        if(!cornersOnly)
        {
            DrawThickLine(luu, uuu, width, depthTested, edgeColour);
            DrawThickLine(lul, uul, width, depthTested, edgeColour);
            DrawThickLine(llu, ulu, width, depthTested, edgeColour);
            DrawThickLine(lll, ull, width, depthTested, edgeColour);
            DrawThickLine(lul, lll, width, depthTested, edgeColour);
            DrawThickLine(uul, ull, width, depthTested, edgeColour);
            DrawThickLine(luu, llu, width, depthTested, edgeColour);
            DrawThickLine(uuu, ulu, width, depthTested, edgeColour);
            DrawThickLine(lll, llu, width, depthTested, edgeColour);
            DrawThickLine(ull, ulu, width, depthTested, edgeColour);
            DrawThickLine(lul, luu, width, depthTested, edgeColour);
            DrawThickLine(uul, uuu, width, depthTested, edgeColour);
        }
        else
        {
            DrawThickLine(luu, luu + (uuu - luu) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(luu + (uuu - luu) * 0.75f, uuu, width, depthTested, edgeColour);
            DrawThickLine(lul, lul + (uul - lul) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(lul + (uul - lul) * 0.75f, uul, width, depthTested, edgeColour);
            DrawThickLine(llu, llu + (ulu - llu) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(llu + (ulu - llu) * 0.75f, ulu, width, depthTested, edgeColour);
            DrawThickLine(lll, lll + (ull - lll) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(lll + (ull - lll) * 0.75f, ull, width, depthTested, edgeColour);
            DrawThickLine(lul, lul + (lll - lul) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(lul + (lll - lul) * 0.75f, lll, width, depthTested, edgeColour);
            DrawThickLine(uul, uul + (ull - uul) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(uul + (ull - uul) * 0.75f, ull, width, depthTested, edgeColour);
            DrawThickLine(luu, luu + (llu - luu) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(luu + (llu - luu) * 0.75f, llu, width, depthTested, edgeColour);
            DrawThickLine(uuu, uuu + (ulu - uuu) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(uuu + (ulu - uuu) * 0.75f, ulu, width, depthTested, edgeColour);
            DrawThickLine(lll, lll + (llu - lll) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(lll + (llu - lll) * 0.75f, llu, width, depthTested, edgeColour);
            DrawThickLine(ull, ull + (ulu - ull) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(ull + (ulu - ull) * 0.75f, ulu, width, depthTested, edgeColour);
            DrawThickLine(lul, lul + (luu - lul) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(lul + (luu - lul) * 0.75f, luu, width, depthTested, edgeColour);
            DrawThickLine(uul, uul + (uuu - uul) * 0.25f, width, depthTested, edgeColour);
            DrawThickLine(uul + (uuu - uul) * 0.75f, uuu, width, depthTested, edgeColour);
        }
    }

    void DebugRenderer::DebugDraw(const Maths::BoundingSphere& sphere, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        Lumos::DebugRenderer::DrawPoint(sphere.GetCenter(), sphere.GetRadius(), false, colour);
    }

    void DebugRenderer::DebugDraw(Maths::Frustum& frustum, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        auto* vertices = frustum.GetVerticies();

        DebugRenderer::DrawHairLine(vertices[0], vertices[1], false, colour);
        DebugRenderer::DrawHairLine(vertices[1], vertices[2], false, colour);
        DebugRenderer::DrawHairLine(vertices[2], vertices[3], false, colour);
        DebugRenderer::DrawHairLine(vertices[3], vertices[0], false, colour);
        DebugRenderer::DrawHairLine(vertices[4], vertices[5], false, colour);
        DebugRenderer::DrawHairLine(vertices[5], vertices[6], false, colour);
        DebugRenderer::DrawHairLine(vertices[6], vertices[7], false, colour);
        DebugRenderer::DrawHairLine(vertices[7], vertices[4], false, colour);
        DebugRenderer::DrawHairLine(vertices[0], vertices[4], false, colour);
        DebugRenderer::DrawHairLine(vertices[1], vertices[5], false, colour);
        DebugRenderer::DrawHairLine(vertices[2], vertices[6], false, colour);
        DebugRenderer::DrawHairLine(vertices[3], vertices[7], false, colour);
    }

    void DebugRenderer::DebugDraw(Graphics::Light* light, const Quat& rotation, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        // Directional
        if(light->Type < 0.1f)
        {
            Vec3 offset(0.0f, 0.1f, 0.0f);
            DrawHairLine(Vec3(light->Position) + offset, Vec3(light->Position + (light->Direction) * 2.0f) + offset, false, colour);
            DrawHairLine(Vec3(light->Position) - offset, Vec3(light->Position + (light->Direction) * 2.0f) - offset, false, colour);

            DrawHairLine(Vec3(light->Position), Vec3(light->Position + (light->Direction) * 2.0f), false, colour);
            DebugDrawCone(20, 4, 30.0f, 1.5f, (light->Position - (light->Direction) * 1.5f), rotation, colour);
        }
        // Spot
        else if(light->Type < 1.1f)
        {
            DebugDrawCone(20, 4, light->Angle * Maths::M_RADTODEG, light->Intensity, light->Position, rotation, colour);
        }
        // Point
        else
        {
            DebugDrawSphere(light->Radius, light->Position, colour);
        }
    }

    void DebugRenderer::DebugDraw(SoundNode* sound, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        DrawPoint(sound->GetPosition(), sound->GetRadius(), false, colour);
    }

    void DebugRenderer::DebugDrawCircle(int numVerts, float radius, const Vec3& position, const Quat& rotation, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float step = 360.0f / float(numVerts);

        for(int i = 0; i < numVerts; i++)
        {
            float cx     = Maths::Cos(step * i) * radius;
            float cy     = Maths::Sin(step * i) * radius;
            Vec3 current = Vec3(cx, cy, 0.0f);

            float nx  = Maths::Cos(step * (i + 1)) * radius;
            float ny  = Maths::Sin(step * (i + 1)) * radius;
            Vec3 next = Vec3(nx, ny, 0.0f);

            DrawHairLine(position + (rotation * current), position + (rotation * next), false, colour);
        }
    }
    void DebugRenderer::DebugDrawSphere(float radius, const Vec3& position, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float offset = 0.0f;
        DebugDrawCircle(20, radius, position, Quat(Vec3(0.0f, 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, position, Quat(Vec3(90.0f, 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, position, Quat(Vec3(0.0f, 90.0f, 90.0f)), colour);
    }

    void DebugRenderer::DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const Vec3& position, const Quat& rotation, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float endAngle   = Maths::Tan(angle * 0.5f) * length;
        Vec3 forward     = -(rotation * Vec3(0.0f, 0.0f, -1.0f));
        Vec3 endPosition = position + forward * length;
        float offset     = 0.0f;
        DebugDrawCircle(numCircleVerts, endAngle, endPosition, rotation, colour);

        for(int i = 0; i < numLinesToCircle; i++)
        {
            float a    = i * 90.0f;
            Vec3 point = rotation * Vec3(Maths::Cos(a), Maths::Sin(a), 0.0f) * endAngle;
            DrawHairLine(position, position + point + forward * length, false, colour);
        }
    }

    void DebugDrawArc(int numVerts, float radius, const Vec3& start, const Vec3& end, const Quat& rotation, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float step = 180.0f / numVerts;
        Quat rot   = Mat4::LookAt(rotation * start, rotation * end, Vec3(0.0f, 1.0f, 0.0f)).Rotation();
        rot        = rotation * rot;

        Vec3 arcCentre = (start + end) * 0.5f;
        for(int i = 0; i < numVerts; i++)
        {
            float cx     = Maths::Cos(step * i) * radius;
            float cy     = Maths::Sin(step * i) * radius;
            Vec3 current = Vec3(cx, cy, 0.0f);

            float nx  = Maths::Cos(step * (i + 1)) * radius;
            float ny  = Maths::Sin(step * (i + 1)) * radius;
            Vec3 next = Vec3(nx, ny, 0.0f);

            DebugRenderer::DrawHairLine(arcCentre + (rot * current), arcCentre + (rot * next), false, colour);
        }
    }

    void DebugRenderer::DebugDrawCapsule(const Vec3& position, const Quat& rotation, float height, float radius, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        Vec3 up = (rotation * Vec3(0.0f, 1.0f, 0.0f));

        Vec3 topSphereCentre    = position + up * (height * 0.5f);
        Vec3 bottomSphereCentre = position - up * (height * 0.5f);

        DebugDrawCircle(20, radius, topSphereCentre, rotation * Quat(Vec3(90.0f, 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, bottomSphereCentre, rotation * Quat(Vec3(90.0f, 0.0f, 0.0f)), colour);

        // Draw 10 arcs
        // Sides
        float step = 360.0f / float(20);
        for(int i = 0; i < 20; i++)
        {
            float z = Maths::Cos(step * i) * radius;
            float x = Maths::Sin(step * i) * radius;

            Vec3 offset = rotation * Vec3(x, 0.0f, z);
            DrawHairLine(bottomSphereCentre + offset, topSphereCentre + offset, false, colour);

            if(i < 10)
            {
                float z2 = Maths::Cos(step * (i + 10)) * radius;
                float x2 = Maths::Sin(step * (i + 10)) * radius;

                Vec3 offset2 = rotation * Vec3(x2, 0.0f, z2);
                // Top Hemishpere
                DebugDrawArc(20, radius, topSphereCentre + offset, topSphereCentre + offset2, rotation, colour);
                // Bottom Hemisphere
                DebugDrawArc(20, radius, bottomSphereCentre + offset, bottomSphereCentre + offset2, rotation * Quat(Vec3(180.0f, 0.0f, 0.0f)), colour);
            }
        }
    }

    void DebugDrawPyramid(int numVerts, float baseSize, float height, const Vec3& position, const Quat& rotation, const Vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();

        // Calculate half base size
        float halfBaseSize = baseSize * 0.5f;

        // Vertices for the base of the pyramid
        Vec3 baseVertices[4] = {
            Vec3(-halfBaseSize, 0.0f, 0.0f),
            Vec3(halfBaseSize, 0.0f, 0.0f),
            Vec3(halfBaseSize, 0.0f, baseSize),
            Vec3(-halfBaseSize, 0.0f, baseSize)
        };

        // Top vertex of the pyramid
        Vec3 topVertex(0.0f, height, halfBaseSize);

        // Transform base vertices and top vertex by rotation and position
        for(int i = 0; i < 4; ++i)
        {
            baseVertices[i] = position + rotation * baseVertices[i];
        }
        topVertex = position + rotation * topVertex;

        // Draw the base of the pyramid
        for(int i = 0; i < 4; ++i)
        {
            int j = (i + 1) % 4;
            DebugRenderer::DrawHairLine(baseVertices[i], baseVertices[j], false, colour);
        }

        // Draw lines from the top vertex to each base vertex
        for(int i = 0; i < 4; ++i)
        {
            DebugRenderer::DrawHairLine(topVertex, baseVertices[i], false, colour);
        }
    }

    void DebugRenderer::DebugDrawBone(const Vec3& parent, const Vec3& child, const Vec4& colour)
    {
        DrawHairLine(parent, child, false, colour);
    }

    void DebugRenderer::DebugDraw(const Maths::Ray& ray, const Vec4& colour, float distance)
    {
        LUMOS_PROFILE_FUNCTION();
        DrawHairLine(ray.Origin, ray.Origin + ray.Direction * distance, false, colour);
    }

}
