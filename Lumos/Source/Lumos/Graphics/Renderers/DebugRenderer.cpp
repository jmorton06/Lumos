#include "Precompiled.h"
#include "DebugRenderer.h"
#include "Core/OS/Window.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "RenderPasses.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Camera/Camera.h"
#include "Maths/Transform.h"
#include "Maths/Frustum.h"
#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Ray.h"
#include "Maths/Maths.h"
#include "Audio/SoundNode.h"
#include <cstdarg>

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
#define VSNPRINTF(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList) vsnprintf_s(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList)
#elif LUMOS_PLATFORM_MACOS
#define VSNPRINTF(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList) vsnprintf_l(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList)
#elif LUMOS_PLATFORM_LINUX
#define VSNPRINTF(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList) vsnprintf(_DstBuf, _DstSize, _Format, _ArgList)
#elif LUMOS_PLATFORM_MOBILE
#define VSNPRINTF(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList) 0
#else
#define VSNPRINTF(_DstBuf, _DstSize, _MaxCount, _Format, _ArgList) 0
#endif

#ifndef LUMOS_PLATFORM_WINDOWS
#define _TRUNCATE 0
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

    void DebugRenderer::Reset()
    {
        LUMOS_PROFILE_FUNCTION();
        s_Instance->m_DrawList.m_DebugTriangles.clear();
        s_Instance->m_DrawList.m_DebugLines.clear();
        s_Instance->m_DrawList.m_DebugThickLines.clear();
        s_Instance->m_DrawList.m_DebugPoints.clear();

        s_Instance->m_DrawListNDT.m_DebugTriangles.clear();
        s_Instance->m_DrawListNDT.m_DebugLines.clear();
        s_Instance->m_DrawListNDT.m_DebugThickLines.clear();
        s_Instance->m_DrawListNDT.m_DebugPoints.clear();

        s_Instance->m_TextList.clear();
        s_Instance->m_TextListNDT.clear();
        s_Instance->m_TextListCS.clear();
        s_Instance->m_NumStatusEntries    = 0;
        s_Instance->m_MaxStatusEntryWidth = 0.0f;
    }

    void DebugRenderer::ClearLogEntries()
    {
        s_Instance->m_vLogEntries.clear();
        s_Instance->m_LogEntriesOffset = 0;
    }

    void DebugRenderer::SortLists()
    {
        float cs_size_x = LOG_TEXT_SIZE / s_Instance->m_Width * 2.0f;
        float cs_size_y = LOG_TEXT_SIZE / s_Instance->m_Height * 2.0f;
        size_t log_len  = s_Instance->m_vLogEntries.size();

        float max_x = 0.0f;
        for(size_t i = 0; i < log_len; ++i)
        {
            max_x = Maths::Max(max_x, s_Instance->m_vLogEntries[i].text.length() * cs_size_x * 0.6f);

            size_t idx                              = (i + s_Instance->m_LogEntriesOffset) % MAX_LOG_SIZE;
            float alpha                             = 1.0f - ((float)log_len - (float)i) / (float)log_len;
            s_Instance->m_vLogEntries[idx].colour.w = alpha;
            DrawTextCs(glm::vec4(-1.0f + cs_size_x * 0.5f, -1.0f + ((log_len - i - 1) * cs_size_y) + cs_size_y, 0.0f, 1.0f), LOG_TEXT_SIZE, s_Instance->m_vLogEntries[idx].text, s_Instance->m_vLogEntries[idx].colour);
        }
    }

    DebugRenderer::DebugRenderer()
    {
        m_vLogEntries.clear();
        m_LogEntriesOffset = 0;
    }

    DebugRenderer::~DebugRenderer()
    {
    }

    // Draw Point (circle)
    void DebugRenderer::GenDrawPoint(bool ndt, const glm::vec3& pos, float point_radius, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugPoints.emplace_back(pos, point_radius, colour);
        else
            s_Instance->m_DrawList.m_DebugPoints.emplace_back(pos, point_radius, colour);
    }

    void DebugRenderer::DrawPoint(const glm::vec3& pos, float point_radius, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawPoint(false, pos, point_radius, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawPoint(const glm::vec3& pos, float point_radius, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawPoint(false, pos, point_radius, colour);
    }
    void DebugRenderer::DrawPointNDT(const glm::vec3& pos, float point_radius, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawPoint(true, pos, point_radius, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawPointNDT(const glm::vec3& pos, float point_radius, const glm::vec4& colour)
    {
        GenDrawPoint(true, pos, point_radius, colour);
    }

    // Draw Line with a given thickness
    void DebugRenderer::GenDrawThickLine(bool ndt, const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugThickLines.emplace_back(start, end, colour);
        else
            s_Instance->m_DrawList.m_DebugThickLines.emplace_back(start, end, colour);
    }
    void DebugRenderer::DrawThickLine(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawThickLine(false, start, end, line_width, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawThickLine(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawThickLine(false, start, end, line_width, colour);
    }
    void DebugRenderer::DrawThickLineNDT(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawThickLine(true, start, end, line_width, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawThickLineNDT(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawThickLine(true, start, end, line_width, colour);
    }

    // Draw line with thickness of 1 screen pixel regardless of distance from camera
    void DebugRenderer::GenDrawHairLine(bool ndt, const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        if(ndt)
            s_Instance->m_DrawListNDT.m_DebugLines.emplace_back(start, end, colour);
        else
            s_Instance->m_DrawList.m_DebugLines.emplace_back(start, end, colour);
    }
    void DebugRenderer::DrawHairLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(false, start, end, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawHairLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(false, start, end, colour);
    }
    void DebugRenderer::DrawHairLineNDT(const glm::vec3& start, const glm::vec3& end, const glm::vec3& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(true, start, end, glm::vec4(colour, 1.0f));
    }
    void DebugRenderer::DrawHairLineNDT(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(true, start, end, colour);
    }

    // Draw Matrix (x,y,z axis at pos)
    void DebugRenderer::DrawMatrix(const glm::mat4& mtx)
    {
        LUMOS_PROFILE_FUNCTION();
        // glm::vec3 position = mtx[3];
        // GenDrawHairLine(false, position, position + glm::vec3(mtx[0], mtx[1], mtx[2]), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        // GenDrawHairLine(false, position, position + glm::vec3(mtx[4], mtx[5], mtx[6]), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        // GenDrawHairLine(false, position, position + glm::vec3(mtx[8], mtx[9], mtx[10]), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrix(const glm::mat3& mtx, const glm::vec3& position)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(false, position, position + mtx[0], glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        GenDrawHairLine(false, position, position + mtx[1], glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        GenDrawHairLine(false, position, position + mtx[2], glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrixNDT(const glm::mat4& mtx)
    {
        LUMOS_PROFILE_FUNCTION();
        // glm::vec3 position = mtx[3];
        // GenDrawHairLine(true, position, position + glm::vec3(mtx[0], mtx[1], mtx[2]), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        // GenDrawHairLine(true, position, position + glm::vec3(mtx[4], mtx[5], mtx[6]), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        // GenDrawHairLine(true, position, position + glm::vec3(mtx[8], mtx[9], mtx[10]), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrixNDT(const glm::mat3& mtx, const glm::vec3& position)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawHairLine(true, position, position + mtx[0], glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        GenDrawHairLine(true, position, position + mtx[1], glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        GenDrawHairLine(true, position, position + mtx[2], glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }

    // Draw Triangle
    void DebugRenderer::GenDrawTriangle(bool ndt, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        s_Instance->m_DrawList.m_DebugTriangles.emplace_back(v0, v1, v2, colour);
    }

    void DebugRenderer::DrawTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawTriangle(false, v0, v1, v2, colour);
    }

    void DebugRenderer::DrawTriangleNDT(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        GenDrawTriangle(true, v0, v1, v2, colour);
    }

    // Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
    void DebugRenderer::DrawPolygon(int n_verts, const glm::vec3* verts, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        for(int i = 2; i < n_verts; ++i)
        {
            GenDrawTriangle(false, verts[0], verts[i - 1], verts[i], colour);
        }
    }

    void DebugRenderer::DrawPolygonNDT(int n_verts, const glm::vec3* verts, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        for(int i = 2; i < n_verts; ++i)
        {
            GenDrawTriangle(true, verts[0], verts[i - 1], verts[i], colour);
        }
    }

    void DebugRenderer::DrawTextCs(const glm::vec4& cs_pos, const float font_size, const std::string& text, const glm::vec4& colour)
    {
        glm::vec3 cs_size = glm::vec3(font_size / GetInstance()->m_Width, font_size / GetInstance()->m_Height, 0.0f);
        cs_size           = cs_size * cs_pos.w;

        // Work out the starting position of text based off desired alignment
        float x_offset      = 0.0f;
        const auto text_len = static_cast<int>(text.length());

        DebugText& dText = GetInstance()->m_TextListCS.emplace_back();
        dText.text       = text;
        dText.Position   = cs_pos;
        dText.colour     = colour;
        dText.Size       = font_size;

        // Add each characters to the draw list individually
        // for (int i = 0; i < text_len; ++i)
        //{
        //     glm::vec4 char_pos = glm::vec4(cs_pos.x + x_offset, cs_pos.y, cs_pos.z, cs_pos.w);
        //     glm::vec4 char_data = glm::vec4(cs_size.x, cs_size.y, static_cast<float>(text[i]), 0.0f);

        //    GetInstance()->m_vChars.push_back(char_pos);
        //    GetInstance()->m_vChars.push_back(char_data);
        //    GetInstance()->m_vChars.push_back(colour);
        //    GetInstance()->m_vChars.push_back(colour);    //We dont really need this, but we need the padding to match the same vertex format as all the other debug drawables

        //    x_offset += cs_size.x * 1.2f;
        //}
    }

    // Draw Text WorldSpace
    void DebugRenderer::DrawTextWs(const glm::vec3& pos, const float font_size, const glm::vec4& colour, const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        std::string formatted_text = std::string(buf, static_cast<size_t>(length));

        // glm::vec4 cs_pos = GetInstance()->m_ProjViewMtx * glm::vec4(pos, 1.0f);
        // DrawTextCs(cs_pos, font_size, formatted_text, colour);

        DebugText& dText = GetInstance()->m_TextList.emplace_back();
        dText.text       = text;
        dText.Position   = glm::vec4(pos, 1.0f);
        dText.colour     = colour;
        dText.Size       = font_size;
    }

    void DebugRenderer::DrawTextWsNDT(const glm::vec3& pos, const float font_size, const glm::vec4& colour, const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        std::string formatted_text = std::string(buf, static_cast<size_t>(length));

        // glm::vec4 cs_pos = GetInstance()->m_ProjViewMtx * glm::vec4(pos, 1.0f);
        // cs_pos.z = (1.0f * cs_pos.w);
        // DrawTextCs(cs_pos, font_size, formatted_text, colour);

        DebugText& dText = GetInstance()->m_TextListNDT.emplace_back();
        dText.text       = text;
        dText.Position   = glm::vec4(pos, 1.0f);
        dText.colour     = colour;
        dText.Size       = font_size;
    }

    // Status Entry
    void DebugRenderer::AddStatusEntry(const glm::vec4& colour, const std::string text, ...)
    {
        float cs_size_x = STATUS_TEXT_SIZE / GetInstance()->m_Width * 2.0f;
        float cs_size_y = STATUS_TEXT_SIZE / GetInstance()->m_Height * 2.0f;

        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        std::string formatted_text = std::string(buf, static_cast<size_t>(length));

        DrawTextCs(glm::vec4(-1.0f + cs_size_x * 0.5f, 1.0f - (GetInstance()->m_NumStatusEntries * cs_size_y) + cs_size_y, -1.0f, 1.0f), STATUS_TEXT_SIZE, formatted_text, colour);
        GetInstance()->m_NumStatusEntries++;
        GetInstance()->m_MaxStatusEntryWidth = Maths::Max(GetInstance()->m_MaxStatusEntryWidth, cs_size_x * 0.6f * length);
    }

    // Log

    void DebugRenderer::AddLogEntry(const glm::vec3& colour, const std::string& text)
    {
        /*    time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);*/

        // std::stringstream ss;
        // ss << "[" << ltm.tm_hour << ":" << ltm.tm_min << ":" << ltm.tm_sec << "] ";

        LogEntry le;
        le.text   = /*ss.str() + */ text; // +"\n";
        le.colour = glm::vec4(colour.x, colour.y, colour.z, 1.0f);

        if(GetInstance()->m_vLogEntries.size() < MAX_LOG_SIZE)
            GetInstance()->m_vLogEntries.push_back(le);
        else
        {
            GetInstance()->m_vLogEntries[GetInstance()->m_LogEntriesOffset] = le;
            GetInstance()->m_LogEntriesOffset                               = (GetInstance()->m_LogEntriesOffset + 1) % MAX_LOG_SIZE;
        }

        LUMOS_LOG_WARN(text);
    }

    void DebugRenderer::Log(const glm::vec3& colour, const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;
        AddLogEntry(colour, std::string(buf, static_cast<size_t>(length)));
    }

    void DebugRenderer::Log(const std::string text, ...)
    {
        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;
        AddLogEntry(glm::vec3(0.4f, 1.0f, 0.6f), std::string(buf, static_cast<size_t>(length)));
    }

    void DebugRenderer::LogE(const char* filename, int linenumber, const std::string text, ...)
    {
        // Error Format:
        //<text>
        //         -> <line number> : <file name>

        va_list args;
        va_start(args, text);

        char buf[1024];

        int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

        va_end(args);

        int length = (needed < 0) ? 1024 : needed;

        Log(glm::vec3(1.0f, 0.25f, 0.25f), "[ERROR] %s:%d", filename, linenumber);
        AddLogEntry(glm::vec3(1.0f, 0.5f, 0.5f), "\t \x01 \"" + std::string(buf, static_cast<size_t>(length)) + "\"");

        std::cout << std::endl;
    }

    void DebugRenderer::DebugDraw(const Maths::BoundingBox& box, const glm::vec4& edgeColour, bool cornersOnly, float width)
    {
        LUMOS_PROFILE_FUNCTION();
        glm::vec3 uuu = box.Max();
        glm::vec3 lll = box.Min();

        glm::vec3 ull(uuu.x, lll.y, lll.z);
        glm::vec3 uul(uuu.x, uuu.y, lll.z);
        glm::vec3 ulu(uuu.x, lll.y, uuu.z);

        glm::vec3 luu(lll.x, uuu.y, uuu.z);
        glm::vec3 llu(lll.x, lll.y, uuu.z);
        glm::vec3 lul(lll.x, uuu.y, lll.z);

        // Draw edges
        if(!cornersOnly)
        {
            DrawThickLineNDT(luu, uuu, width, edgeColour);
            DrawThickLineNDT(lul, uul, width, edgeColour);
            DrawThickLineNDT(llu, ulu, width, edgeColour);
            DrawThickLineNDT(lll, ull, width, edgeColour);

            DrawThickLineNDT(lul, lll, width, edgeColour);
            DrawThickLineNDT(uul, ull, width, edgeColour);
            DrawThickLineNDT(luu, llu, width, edgeColour);
            DrawThickLineNDT(uuu, ulu, width, edgeColour);

            DrawThickLineNDT(lll, llu, width, edgeColour);
            DrawThickLineNDT(ull, ulu, width, edgeColour);
            DrawThickLineNDT(lul, luu, width, edgeColour);
            DrawThickLineNDT(uul, uuu, width, edgeColour);
        }
        else
        {
            DrawThickLineNDT(luu, luu + (uuu - luu) * 0.25f, width, edgeColour);
            DrawThickLineNDT(luu + (uuu - luu) * 0.75f, uuu, width, edgeColour);

            DrawThickLineNDT(lul, lul + (uul - lul) * 0.25f, width, edgeColour);
            DrawThickLineNDT(lul + (uul - lul) * 0.75f, uul, width, edgeColour);

            DrawThickLineNDT(llu, llu + (ulu - llu) * 0.25f, width, edgeColour);
            DrawThickLineNDT(llu + (ulu - llu) * 0.75f, ulu, width, edgeColour);

            DrawThickLineNDT(lll, lll + (ull - lll) * 0.25f, width, edgeColour);
            DrawThickLineNDT(lll + (ull - lll) * 0.75f, ull, width, edgeColour);

            DrawThickLineNDT(lul, lul + (lll - lul) * 0.25f, width, edgeColour);
            DrawThickLineNDT(lul + (lll - lul) * 0.75f, lll, width, edgeColour);

            DrawThickLineNDT(uul, uul + (ull - uul) * 0.25f, width, edgeColour);
            DrawThickLineNDT(uul + (ull - uul) * 0.75f, ull, width, edgeColour);

            DrawThickLineNDT(luu, luu + (llu - luu) * 0.25f, width, edgeColour);
            DrawThickLineNDT(luu + (llu - luu) * 0.75f, llu, width, edgeColour);

            DrawThickLineNDT(uuu, uuu + (ulu - uuu) * 0.25f, width, edgeColour);
            DrawThickLineNDT(uuu + (ulu - uuu) * 0.75f, ulu, width, edgeColour);

            DrawThickLineNDT(lll, lll + (llu - lll) * 0.25f, width, edgeColour);
            DrawThickLineNDT(lll + (llu - lll) * 0.75f, llu, width, edgeColour);

            DrawThickLineNDT(ull, ull + (ulu - ull) * 0.25f, width, edgeColour);
            DrawThickLineNDT(ull + (ulu - ull) * 0.75f, ulu, width, edgeColour);

            DrawThickLineNDT(lul, lul + (luu - lul) * 0.25f, width, edgeColour);
            DrawThickLineNDT(lul + (luu - lul) * 0.75f, luu, width, edgeColour);

            DrawThickLineNDT(uul, uul + (uuu - uul) * 0.25f, width, edgeColour);
            DrawThickLineNDT(uul + (uuu - uul) * 0.75f, uuu, width, edgeColour);
        }
    }

    void DebugRenderer::DebugDraw(const Maths::BoundingSphere& sphere, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        Lumos::DebugRenderer::DrawPointNDT(sphere.GetCenter(), sphere.GetRadius(), colour);
    }

    void DebugRenderer::DebugDraw(Maths::Frustum& frustum, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        auto* vertices = frustum.GetVerticies();

        DebugRenderer::DrawHairLine(vertices[0], vertices[1], colour);
        DebugRenderer::DrawHairLine(vertices[1], vertices[2], colour);
        DebugRenderer::DrawHairLine(vertices[2], vertices[3], colour);
        DebugRenderer::DrawHairLine(vertices[3], vertices[0], colour);
        DebugRenderer::DrawHairLine(vertices[4], vertices[5], colour);
        DebugRenderer::DrawHairLine(vertices[5], vertices[6], colour);
        DebugRenderer::DrawHairLine(vertices[6], vertices[7], colour);
        DebugRenderer::DrawHairLine(vertices[7], vertices[4], colour);
        DebugRenderer::DrawHairLine(vertices[0], vertices[4], colour);
        DebugRenderer::DrawHairLine(vertices[1], vertices[5], colour);
        DebugRenderer::DrawHairLine(vertices[2], vertices[6], colour);
        DebugRenderer::DrawHairLine(vertices[3], vertices[7], colour);
    }

    void DebugRenderer::DebugDraw(Graphics::Light* light, const glm::quat& rotation, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        // Directional
        if(light->Type < 0.1f)
        {
            glm::vec3 offset(0.0f, 0.1f, 0.0f);
            DrawHairLine(glm::vec3(light->Position) + offset, glm::vec3(light->Position + (light->Direction) * 2.0f) + offset, colour);
            DrawHairLine(glm::vec3(light->Position) - offset, glm::vec3(light->Position + (light->Direction) * 2.0f) - offset, colour);

            DrawHairLine(glm::vec3(light->Position), glm::vec3(light->Position + (light->Direction) * 2.0f), colour);
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

    void DebugRenderer::DebugDraw(SoundNode* sound, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        DrawPoint(sound->GetPosition(), sound->GetRadius(), colour);
    }

    void DebugRenderer::DebugDrawCircle(int numVerts, float radius, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float step = 360.0f / float(numVerts);

        for(int i = 0; i < numVerts; i++)
        {
            float cx          = Maths::Cos(step * i) * radius;
            float cy          = Maths::Sin(step * i) * radius;
            glm::vec3 current = glm::vec3(cx, cy, 0.0f);

            float nx       = Maths::Cos(step * (i + 1)) * radius;
            float ny       = Maths::Sin(step * (i + 1)) * radius;
            glm::vec3 next = glm::vec3(nx, ny, 0.0f);

            DrawHairLine(position + (rotation * current), position + (rotation * next), colour);
        }
    }
    void DebugRenderer::DebugDrawSphere(float radius, const glm::vec3& position, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float offset = 0.0f;
        DebugDrawCircle(20, radius, position, glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, position, glm::quat(glm::vec3(90.0f, 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, position, glm::quat(glm::vec3(0.0f, 90.0f, 90.0f)), colour);
    }

    void DebugRenderer::DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float endAngle        = Maths::Tan(angle * 0.5f) * length;
        glm::vec3 forward     = -(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 endPosition = position + forward * length;
        float offset          = 0.0f;
        DebugDrawCircle(numCircleVerts, endAngle, endPosition, rotation, colour);

        for(int i = 0; i < numLinesToCircle; i++)
        {
            float a         = i * 90.0f;
            glm::vec3 point = rotation * glm::vec3(Maths::Cos(a), Maths::Sin(a), 0.0f) * endAngle;
            DrawHairLine(position, position + point + forward * length, colour);
        }
    }

    void DebugDrawArc(int numVerts, float radius, const glm::vec3& start, const glm::vec3& end, const glm::quat& rotation, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        float step    = 180.0f / numVerts;
        glm::quat rot = glm::lookAt(rotation * start, rotation * end, glm::vec3(0.0f, 1.0f, 0.0f));
        rot           = rotation * rot;

        glm::vec3 arcCentre = (start + end) * 0.5f;
        for(int i = 0; i < numVerts; i++)
        {
            float cx          = Maths::Cos(step * i) * radius;
            float cy          = Maths::Sin(step * i) * radius;
            glm::vec3 current = glm::vec3(cx, cy, 0.0f);

            float nx       = Maths::Cos(step * (i + 1)) * radius;
            float ny       = Maths::Sin(step * (i + 1)) * radius;
            glm::vec3 next = glm::vec3(nx, ny, 0.0f);

            DebugRenderer::DrawHairLine(arcCentre + (rot * current), arcCentre + (rot * next), colour);
        }
    }

    void DebugRenderer::DebugDrawCapsule(const glm::vec3& position, const glm::quat& rotation, float height, float radius, const glm::vec4& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        glm::vec3 up = (rotation * glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 topSphereCentre    = position + up * (height * 0.5f);
        glm::vec3 bottomSphereCentre = position - up * (height * 0.5f);

        DebugDrawCircle(20, radius, topSphereCentre, rotation * glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f)), colour);
        DebugDrawCircle(20, radius, bottomSphereCentre, rotation * glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f)), colour);

        // Draw 10 arcs
        // Sides
        float step = 360.0f / float(20);
        for(int i = 0; i < 20; i++)
        {
            float z = Maths::Cos(step * i) * radius;
            float x = Maths::Sin(step * i) * radius;

            glm::vec3 offset = rotation * glm::vec4(x, 0.0f, z, 0.0f);
            DrawHairLine(bottomSphereCentre + offset, topSphereCentre + offset, colour);

            if(i < 10)
            {
                float z2 = Maths::Cos(step * (i + 10)) * radius;
                float x2 = Maths::Sin(step * (i + 10)) * radius;

                glm::vec3 offset2 = rotation * glm::vec4(x2, 0.0f, z2, 0.0f);
                // Top Hemishpere
                DebugDrawArc(20, radius, topSphereCentre + offset, topSphereCentre + offset2, rotation, colour);
                // Bottom Hemisphere
                DebugDrawArc(20, radius, bottomSphereCentre + offset, bottomSphereCentre + offset2, rotation * glm::quat(glm::vec3(glm::radians(180.0f), 0.0f, 0.0f)), colour);
            }
        }
    }

    void DebugRenderer::DebugDraw(const Maths::Ray& ray, const glm::vec4& colour, float distance)
    {
        LUMOS_PROFILE_FUNCTION();
        DrawHairLine(ray.Origin, ray.Origin + ray.Direction * distance, colour);
    }

}
