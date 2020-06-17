#pragma once

#include "lmpch.h"
#include "Maths/Maths.h"

namespace Lumos
{

namespace Graphics
{
    class RenderPass;
    class Pipeline;
    class DescriptorSet;
    class CommandBuffer;
    class UniformBuffer;
    class Renderable2D;
    class Framebuffer;
    class Texture;
    class Shader;
    class IndexBuffer;
    class VertexArray;
    class Renderer2D; 
    class LineRenderer;
    class PointRenderer;
    struct Light;
}
    class SoundNode;
	class Texture2D;
	class Material;
    class Scene;

	namespace Maths
	{
		class Sphere;
		class BoundingBox;
        class Frustum;
	}

	class LUMOS_EXPORT DebugRenderer
	{
		friend class Scene;
		friend class GraphicsPipeline;
		friend class Application;
	public:
        static void Init(u32 width, u32 height, bool drawToGBuffer);
        static void Release();
        static void Render(Scene* scene) { if(s_Instance) s_Instance->RenderInternal(scene); }
		static void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer);
        
        DebugRenderer();
        ~DebugRenderer();
	
		//Note: Functions appended with 'NDT' (no depth testing) will always be rendered in the foreground. This can be useful for debugging things inside objects.

		//Draw Point (circle)
		static void DrawPoint(const Maths::Vector3& pos, float point_radius, const Maths::Vector3& colour);
		static void DrawPoint(const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawPointNDT(const Maths::Vector3& pos, float point_radius, const Maths::Vector3& colour);
		static void DrawPointNDT(const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Draw Line with a given thickness
		static void DrawThickLine(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector3& colour);
		static void DrawThickLine(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawThickLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector3& colour);
		static void DrawThickLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Draw line with thickness of 1 screen pixel regardless of distance from camera
		static void DrawHairLine(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector3& colour);
		static void DrawHairLine(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawHairLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector3& colour);
		static void DrawHairLineNDT(const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Draw Matrix (x,y,z axis at pos)
		static void DrawMatrix(const Maths::Matrix4& transform_mtx);
		static void DrawMatrix(const Maths::Matrix3& rotation_mtx, const Maths::Vector3& position);
		static void DrawMatrixNDT(const Maths::Matrix4& transform_mtx);
		static void DrawMatrixNDT(const Maths::Matrix3& rotation_mtx, const Maths::Vector3& position);

		//Draw Triangle
		static void DrawTriangle(const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawTriangleNDT(const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
		static void DrawPolygon(int n_verts, const Maths::Vector3* verts, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawPolygonNDT(int n_verts, const Maths::Vector3* verts, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		static void DebugDraw(const Maths::BoundingBox& box, const Maths::Vector4& edgeColour, bool cornersOnly = false, float width = 0.02f);
		static void DebugDraw(const Maths::Sphere& sphere, const Maths::Vector4& colour);
        static void DebugDraw(const Maths::Frustum& frustum, const Maths::Vector4& colour);
        static void DebugDraw(Graphics::Light* light, const Maths::Vector4& colour);
        static void DebugDraw(SoundNode* sound, const Maths::Vector4& colour);

		static void OnResize(u32 width, u32 height) { if(s_Instance) s_Instance->OnResizeInternal(width, height);  }

	protected:
		//Actual functions managing data parsing to save code bloat - called by public functions
		static void GenDrawPoint(bool ndt, const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour);
		static void GenDrawThickLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour);
		static void GenDrawHairLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour);
		static void GenDrawTriangle(bool ndt, const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour);
    
        static DebugRenderer* GetInstance() { return s_Instance; }
        static void Reset() { if(s_Instance) s_Instance->Begin(); }
    
	private:

        void Begin();
        void RenderInternal(Scene* scene);
        void OnResizeInternal(u32 width, u32 height);
        
        static DebugRenderer* s_Instance;

        Graphics::Renderer2D* m_Renderer2D;
        Graphics::LineRenderer* m_LineRenderer;
        Graphics::PointRenderer* m_PointRenderer;


	};
}
