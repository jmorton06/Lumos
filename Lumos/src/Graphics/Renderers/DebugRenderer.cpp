#include "lmpch.h"
#include "DebugRenderer.h"
#include "Core/OS/Window.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/IndexBuffer.h"
#include "Graphics/API/VertexArray.h"
#include "Graphics/API/Texture.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Sprite.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/RenderManager.h"
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
#include "Core/Profiler.h"

namespace Lumos
{
	using namespace Graphics;

    DebugRenderer* DebugRenderer::s_Instance = nullptr;

    static const uint32_t MaxLines = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices = MaxLines * 6;
	#define MAX_BATCH_DRAW_CALLS	100
	#define RENDERER_LINE_SIZE	RENDERER2DLINE_VERTEX_SIZE * 4
	#define RENDERER_BUFFER_SIZE	RENDERER_LINE_SIZE * MaxLineVertices

    void DebugRenderer::Init(u32 width, u32 height, bool drawToGBuffer)
    {
        if(s_Instance)
            return;
        
        s_Instance = new DebugRenderer();
        
        s_Instance->m_Renderer2D = new Graphics::Renderer2D(width, height, drawToGBuffer, false, true, false);
        s_Instance->m_LineRenderer = new Graphics::LineRenderer(width, height, drawToGBuffer, false);
        s_Instance->m_PointRenderer = new Graphics::PointRenderer(width, height, drawToGBuffer, false);
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
            s_Instance->m_Renderer2D->SubmitTriangle(v0,v1,v2, colour);
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
		for (int i = 2; i < n_verts; ++i)
		{
			GenDrawTriangle(false, verts[0], verts[i - 1], verts[i], colour);
		}
	}

	void DebugRenderer::DrawPolygonNDT(int n_verts, const Maths::Vector3* verts, const Maths::Vector4& colour)
	{
		for (int i = 2; i < n_verts; ++i)
		{
			GenDrawTriangle(true, verts[0], verts[i - 1], verts[i], colour);
		}
	}

	void DebugRenderer::DebugDraw(const Maths::BoundingBox& box, const Maths::Vector4 &edgeColour, float width)
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

	void DebugRenderer::DebugDraw(const Maths::Sphere& sphere, const Maths::Vector4 &colour)
	{
		Lumos::DebugRenderer::DrawPointNDT(sphere.center_, sphere.radius_, colour);
	}
    
    void DebugRenderer::DebugDraw(const Maths::Frustum &frustum, const Maths::Vector4 &colour)
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

	void DebugRenderer::RenderInternal(Scene* scene)
	{
        LUMOS_PROFILE_FUNC;
        if(m_Renderer2D)
        {
            m_Renderer2D->BeginRenderPass();
            m_Renderer2D->BeginScene(scene);
            m_Renderer2D->SetSystemUniforms(m_Renderer2D->GetShader());
            m_Renderer2D->SubmitTriangles();
            m_Renderer2D->Present();
            m_Renderer2D->End();
        }

        if(m_PointRenderer)
            m_PointRenderer->RenderInternal(scene);
        if(m_LineRenderer)
            m_LineRenderer->RenderInternal(scene);
	}

	void DebugRenderer::OnResizeInternal(u32 width, u32 height)
	{
        if(m_Renderer2D)
            m_Renderer2D->OnResize(width, height);
        if(m_LineRenderer)
            m_LineRenderer->OnResize(width, height);
        if(m_PointRenderer)
            m_PointRenderer->OnResize(width, height);
	}
}
