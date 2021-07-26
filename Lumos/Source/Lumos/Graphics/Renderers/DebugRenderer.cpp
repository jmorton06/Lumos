#include "Precompiled.h"
#include "DebugRenderer.h"
#include "Core/OS/Window.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Swapchain.h"
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
#include "RenderGraph.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Graphics/Renderers/LineRenderer.h"
#include "Graphics/Renderers/PointRenderer.h"
#include "Maths/Transform.h"
#include "Maths/Frustum.h"
#include "Maths/BoundingBox.h"
#include "Maths/Sphere.h"
#include "Audio/SoundNode.h"

namespace Lumos
{
    using namespace Graphics;

    DebugRenderer* DebugRenderer::s_Instance = nullptr;

    static const uint32_t MaxLines = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices = MaxLines * 6;
#define MAX_BATCH_DRAW_CALLS 100
#define RENDERER_LINE_SIZE RENDERER2DLINE_VERTEX_SIZE * 4
#define RENDERER_BUFFER_SIZE RENDERER_LINE_SIZE* MaxLineVertices

    void DebugRenderer::Init(uint32_t width, uint32_t height)
    {
        if(s_Instance)
            return;

        s_Instance = new DebugRenderer();

        s_Instance->m_Renderer2D = new Graphics::Renderer2D(width, height, false, true, false);
        s_Instance->m_LineRenderer = new Graphics::LineRenderer(width, height, false);
        s_Instance->m_PointRenderer = new Graphics::PointRenderer(width, height, false);
    }

    void DebugRenderer::Release()
    {
        delete s_Instance;
        s_Instance = nullptr;
    }

    DebugRenderer::DebugRenderer()
    {
        m_Renderer2D = nullptr;
        m_LineRenderer = nullptr;
        m_PointRenderer = nullptr;
    }

    DebugRenderer::~DebugRenderer()
    {
        delete m_LineRenderer;
        delete m_Renderer2D;
        delete m_PointRenderer;
    }

    void DebugRenderer::SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer)
    {
        if(!s_Instance)
            return;
        if(s_Instance->m_LineRenderer)
            s_Instance->m_LineRenderer->SetRenderTarget(texture, rebuildFramebuffer);
        if(s_Instance->m_Renderer2D)
            s_Instance->m_Renderer2D->SetRenderTarget(texture, rebuildFramebuffer);
        if(s_Instance->m_PointRenderer)
            s_Instance->m_PointRenderer->SetRenderTarget(texture, rebuildFramebuffer);
    }

    //Draw Point (circle)
    void DebugRenderer::GenDrawPoint(bool ndt, const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour)
    {
        if(s_Instance && s_Instance->m_PointRenderer)
            s_Instance->m_PointRenderer->Submit(pos, point_radius, colour);
    }

    void DebugRenderer::DrawPoint(const Maths::Vector3& pos, float point_radius, const Maths::Vector3& colour)
    {
        GenDrawPoint(false, pos, point_radius, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawPoint(const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour)
    {
        GenDrawPoint(false, pos, point_radius, colour);
    }
    void DebugRenderer::DrawPointNDT(const Maths::Vector3& pos, float point_radius, const Maths::Vector3& colour)
    {
        GenDrawPoint(true, pos, point_radius, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawPointNDT(const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour)
    {
        GenDrawPoint(true, pos, point_radius, colour);
    }

    //Draw Line with a given thickness
    void DebugRenderer::GenDrawThickLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour)
    {
        if(s_Instance && s_Instance->m_LineRenderer)
            s_Instance->m_LineRenderer->Submit(start, end, colour);
    }
    void DebugRenderer::DrawThickLine(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector3& colour)
    {
        GenDrawThickLine(false, start, end, line_width, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawThickLine(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour)
    {
        GenDrawThickLine(false, start, end, line_width, colour);
    }
    void DebugRenderer::DrawThickLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector3& colour)
    {
        GenDrawThickLine(true, start, end, line_width, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawThickLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour)
    {
        GenDrawThickLine(true, start, end, line_width, colour);
    }

    //Draw line with thickness of 1 screen pixel regardless of distance from camera
    void DebugRenderer::GenDrawHairLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour)
    {
        if(s_Instance && s_Instance->m_LineRenderer)
            s_Instance->m_LineRenderer->Submit(start, end, colour);
    }
    void DebugRenderer::DrawHairLine(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector3& colour)
    {
        GenDrawHairLine(false, start, end, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawHairLine(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour)
    {
        GenDrawHairLine(false, start, end, colour);
    }
    void DebugRenderer::DrawHairLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector3& colour)
    {
        GenDrawHairLine(true, start, end, Maths::Vector4(colour, 1.0f));
    }
    void DebugRenderer::DrawHairLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour)
    {
        GenDrawHairLine(true, start, end, colour);
    }

    //Draw Matrix (x,y,z axis at pos)
    void DebugRenderer::DrawMatrix(const Maths::Matrix4& mtx)
    {
        //Maths::Vector3 position = mtx.Translation();
        //GenDrawHairLine(false, position, position + Maths::Vector3(mtx[0], mtx[1], mtx[2]), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        //GenDrawHairLine(false, position, position + Maths::Vector3(mtx[4], mtx[5], mtx[6]), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        //GenDrawHairLine(false, position, position + Maths::Vector3(mtx[8], mtx[9], mtx[10]), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrix(const Maths::Matrix3& mtx, const Maths::Vector3& position)
    {
        GenDrawHairLine(false, position, position + mtx.Column(0), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        GenDrawHairLine(false, position, position + mtx.Column(1), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        GenDrawHairLine(false, position, position + mtx.Column(2), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrixNDT(const Maths::Matrix4& mtx)
    {
        //Maths::Vector3 position = mtx.Translation();
        //GenDrawHairLine(true, position, position + Maths::Vector3(mtx[0], mtx[1], mtx[2]), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        //GenDrawHairLine(true, position, position + Maths::Vector3(mtx[4], mtx[5], mtx[6]), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        //GenDrawHairLine(true, position, position + Maths::Vector3(mtx[8], mtx[9], mtx[10]), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    }
    void DebugRenderer::DrawMatrixNDT(const Maths::Matrix3& mtx, const Maths::Vector3& position)
    {
        GenDrawHairLine(true, position, position + mtx.Column(0), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        GenDrawHairLine(true, position, position + mtx.Column(1), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        GenDrawHairLine(true, position, position + mtx.Column(2), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    }

    //Draw Triangle
    void DebugRenderer::GenDrawTriangle(bool ndt, const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour)
    {
        if(s_Instance && s_Instance->m_Renderer2D)
            s_Instance->m_Renderer2D->SubmitTriangle(v0, v1, v2, colour);
    }

    void DebugRenderer::DrawTriangle(const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour)
    {
        GenDrawTriangle(false, v0, v1, v2, colour);
    }

    void DebugRenderer::DrawTriangleNDT(const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour)
    {
        GenDrawTriangle(true, v0, v1, v2, colour);
    }

    //Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
    void DebugRenderer::DrawPolygon(int n_verts, const Maths::Vector3* verts, const Maths::Vector4& colour)
    {
        for(int i = 2; i < n_verts; ++i)
        {
            GenDrawTriangle(false, verts[0], verts[i - 1], verts[i], colour);
        }
    }

    void DebugRenderer::DrawPolygonNDT(int n_verts, const Maths::Vector3* verts, const Maths::Vector4& colour)
    {
        for(int i = 2; i < n_verts; ++i)
        {
            GenDrawTriangle(true, verts[0], verts[i - 1], verts[i], colour);
        }
    }

    void DebugRenderer::DebugDraw(const Maths::BoundingBox& box, const Maths::Vector4& edgeColour, bool cornersOnly, float width)
    {
        Maths::Vector3 uuu = box.max_;
        Maths::Vector3 lll = box.min_;

        Maths::Vector3 ull(uuu.x, lll.y, lll.z);
        Maths::Vector3 uul(uuu.x, uuu.y, lll.z);
        Maths::Vector3 ulu(uuu.x, lll.y, uuu.z);

        Maths::Vector3 luu(lll.x, uuu.y, uuu.z);
        Maths::Vector3 llu(lll.x, lll.y, uuu.z);
        Maths::Vector3 lul(lll.x, uuu.y, lll.z);

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

    void DebugRenderer::DebugDraw(const Maths::Sphere& sphere, const Maths::Vector4& colour)
    {
        Lumos::DebugRenderer::DrawPointNDT(sphere.center_, sphere.radius_, colour);
    }

    void DebugRenderer::DebugDraw(const Maths::Frustum& frustum, const Maths::Vector4& colour)
    {
        auto* vertices = frustum.vertices_;

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

    void DebugRenderer::Begin()
    {
        if(m_LineRenderer)
            m_LineRenderer->Begin();
        if(m_Renderer2D)
            m_Renderer2D->BeginSimple();
        if(m_PointRenderer)
            m_PointRenderer->Begin();
    }

    void DebugRenderer::ClearInternal()
    {
        if(m_Renderer2D)
            m_Renderer2D->Clear();
        if(m_PointRenderer)
            m_PointRenderer->Clear();
        if(m_LineRenderer)
            m_LineRenderer->Clear();
    }

    void DebugRenderer::BeginSceneInternal(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_Renderer2D)
        {
            m_Renderer2D->BeginScene(scene, overrideCamera, overrideCameraTransform);
        }

        if(m_PointRenderer)
            m_PointRenderer->BeginScene(scene, overrideCamera, overrideCameraTransform);
        if(m_LineRenderer)
            m_LineRenderer->BeginScene(scene, overrideCamera, overrideCameraTransform);
    }

    void DebugRenderer::RenderInternal()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_Renderer2D)
        {
            m_Renderer2D->Begin();
            m_Renderer2D->SetSystemUniforms(m_Renderer2D->GetShader().get());
            m_Renderer2D->SubmitTriangles();
            m_Renderer2D->Present();
            m_Renderer2D->End();
        }

        if(m_PointRenderer)
            m_PointRenderer->RenderInternal();
        if(m_LineRenderer)
            m_LineRenderer->RenderInternal();
    }

    void DebugRenderer::OnResizeInternal(uint32_t width, uint32_t height)
    {
        if(m_Renderer2D)
            m_Renderer2D->OnResize(width, height);
        if(m_LineRenderer)
            m_LineRenderer->OnResize(width, height);
        if(m_PointRenderer)
            m_PointRenderer->OnResize(width, height);
    }

    void DebugRenderer::DebugDraw(Graphics::Light* light, const Maths::Quaternion& rotation, const Maths::Vector4& colour)
    {
        //Directional
        if(light->Type < 0.1f)
        {
            auto flipRotation = rotation * Maths::Quaternion::EulerAnglesToQuaternion(180.0f, 0.0f, 0.0f);
            Maths::Vector3 offset(0.0f, 0.1f, 0.0f);
            DrawHairLine((light->Position).ToVector3() + offset, (light->Position + (light->Direction) * 2.0f).ToVector3() + offset, colour);
            DrawHairLine((light->Position).ToVector3() - offset, (light->Position + (light->Direction) * 2.0f).ToVector3() - offset, colour);

            DrawHairLine((light->Position).ToVector3(), (light->Position + (light->Direction) * 2.0f).ToVector3(), colour);
            DebugDrawCone(20, 4, 30.0f, 1.5f, (light->Position - (light->Direction) * 1.5f).ToVector3(), flipRotation, colour);
        }
        //Spot
        else if(light->Type < 1.1f)
        {
            DebugDrawCone(20, 4, light->Angle * Maths::M_RADTODEG, light->Intensity, light->Position.ToVector3(), rotation, colour);
        }
        //Point
        else
        {
            DebugDrawSphere(light->Radius / 2.0f, light->Position.ToVector3(), colour);
        }
    }

    void DebugRenderer::DebugDraw(SoundNode* sound, const Maths::Vector4& colour)
    {
        DrawPoint(sound->GetPosition(), sound->GetRadius(), colour);
    }

    void DebugRenderer::DebugDrawCircle(int numVerts, float radius, const Maths::Vector3& position, const Maths::Quaternion& rotation, const Maths::Vector4& colour)
    {
        float step = 360.0f / float(numVerts);

        for(int i = 0; i < numVerts; i++)
        {
            float cx = Maths::Cos(step * i) * radius;
            float cy = Maths::Sin(step * i) * radius;
            Maths::Vector3 current = Maths::Vector3(cx, cy);

            float nx = Maths::Cos(step * (i + 1)) * radius;
            float ny = Maths::Sin(step * (i + 1)) * radius;
            Maths::Vector3 next = Maths::Vector3(nx, ny);

            DrawHairLine(position + (rotation * current), position + (rotation * next), colour);
        }
    }
    void DebugRenderer::DebugDrawSphere(float radius, const Maths::Vector3& position, const Maths::Vector4& colour)
    {
        float offset = 0.0f;
        DebugDrawCircle(20, radius, position, Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f), colour);
        DebugDrawCircle(20, radius, position, Maths::Quaternion::EulerAnglesToQuaternion(90.0f, 0.0f, 0.0f), colour);
        DebugDrawCircle(20, radius, position, Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 90.0f, 90.0f), colour);
    }

    void DebugRenderer::DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const Maths::Vector3& position, const Maths::Quaternion& rotation, const Maths::Vector4& colour)
    {
        float endAngle = Maths::Tan(angle * 0.5f) * length;
        Maths::Vector3 forward = -(rotation * Maths::Vector3::FORWARD);
        Maths::Vector3 endPosition = position + forward * length;
        float offset = 0.0f;
        DebugDrawCircle(numCircleVerts, endAngle, endPosition, rotation, colour);

        for(int i = 0; i < numLinesToCircle; i++)
        {
            float a = i * 90.0f;
            Maths::Vector3 point = rotation * Maths::Vector3(Maths::Cos(a), Maths::Sin(a)) * endAngle;
            DrawHairLine(position, position + point + forward * length, colour);
        }
    }
}
