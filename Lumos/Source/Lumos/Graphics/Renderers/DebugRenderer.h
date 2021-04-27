#pragma once

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
        class Renderer2D;
        class LineRenderer;
        class PointRenderer;
        class Material;
        struct Light;
    }
    class SoundNode;
    class Texture2D;
    class Scene;
    class Camera;

    namespace Maths
    {
        class Sphere;
        class BoundingBox;
        class Frustum;
        class Transform;
    }

    class LUMOS_EXPORT DebugRenderer
    {
        friend class Scene;
        friend class GraphicsPipeline;
        friend class Application;
        friend class RenderGraph;

    public:
        static void Init(uint32_t width, uint32_t height);
        static void Release();

        static void Clear()
        {
            if(s_Instance)
                s_Instance->ClearInternal();
        }

        static void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            if(s_Instance)
                s_Instance->BeginSceneInternal(scene, s_Instance->m_OverrideCamera, s_Instance->m_OverrideCameraTransform);
        }

        static void Render()
        {
            if(s_Instance)
                s_Instance->RenderInternal();
        }
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
        static void DebugDraw(Graphics::Light* light, const Maths::Quaternion& rotation, const Maths::Vector4& colour);
        static void DebugDraw(SoundNode* sound, const Maths::Vector4& colour);
        static void DebugDrawSphere(float radius, const Maths::Vector3& position, const Maths::Vector4& colour);
        static void DebugDrawCircle(int numVerts, float radius, const Maths::Vector3& position, const Maths::Quaternion& rotation, const Maths::Vector4& colour);
        static void DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const Maths::Vector3& position, const Maths::Quaternion& rotation, const Maths::Vector4& colour);
        static void OnResize(uint32_t width, uint32_t height)
        {
            if(s_Instance)
                s_Instance->OnResizeInternal(width, height);
        }

        static void SetOverrideCamera(Camera* camera, Maths::Transform* overrideCameraTransform)
        {
            if(s_Instance)
            {
                s_Instance->m_OverrideCamera = camera;
                s_Instance->m_OverrideCameraTransform = overrideCameraTransform;
            }
        }

        static DebugRenderer* GetInstance()
        {
            return s_Instance;
        }
        static void Reset()
        {
            if(s_Instance)
                s_Instance->Begin();
        }

    protected:
        //Actual functions managing data parsing to save code bloat - called by public functions
        static void GenDrawPoint(bool ndt, const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour);
        static void GenDrawThickLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour);
        static void GenDrawHairLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour);
        static void GenDrawTriangle(bool ndt, const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour);

    private:
        void Begin();
        void BeginSceneInternal(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform);
        void RenderInternal();
        void ClearInternal();

        void OnResizeInternal(uint32_t width, uint32_t height);

        static DebugRenderer* s_Instance;

        Graphics::Renderer2D* m_Renderer2D;
        Graphics::LineRenderer* m_LineRenderer;
        Graphics::PointRenderer* m_PointRenderer;
        Camera* m_OverrideCamera = nullptr;
        Maths::Transform* m_OverrideCameraTransform = nullptr;
    };
}
