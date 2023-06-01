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
    }

    DebugRenderer::DebugRenderer()
    {
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
            DebugDrawSphere(light->Radius * 0.5f, light->Position, colour);
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
