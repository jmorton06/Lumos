#pragma once
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

    struct LineInfo
    {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec4 col;

        LineInfo(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec4& colour)
        {
            p1  = pos1;
            p2  = pos2;
            col = colour;
        }
    };

    struct PointInfo
    {
        glm::vec3 p1;
        glm::vec4 col;
        float size;

        PointInfo(const glm::vec3& pos1, float s, const glm::vec4& colour)
        {
            p1   = pos1;
            size = s;
            col  = colour;
        }
    };

    struct TriangleInfo
    {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
        glm::vec4 col;

        TriangleInfo(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3, const glm::vec4& colour)
        {
            p1  = pos1;
            p2  = pos2;
            p3  = pos3;
            col = colour;
        }
    };

    namespace Maths
    {
        class Sphere;
        class BoundingBox;
        class BoundingSphere;
        class Frustum;
        class Transform;
        class Ray;
    }

    class LUMOS_EXPORT DebugRenderer
    {
        friend class Scene;
        friend class Application;
        friend class RenderPasses;

    public:
        static void Init();
        static void Release();
        static void Reset();

        DebugRenderer();
        ~DebugRenderer();

        // Note: Functions appended with 'NDT' (no depth testing) will always be rendered in the foreground. This can be useful for debugging things inside objects.

        // Draw Point (circle)
        static void DrawPoint(const glm::vec3& pos, float point_radius, const glm::vec3& colour);
        static void DrawPoint(const glm::vec3& pos, float point_radius, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        static void DrawPointNDT(const glm::vec3& pos, float point_radius, const glm::vec3& colour);
        static void DrawPointNDT(const glm::vec3& pos, float point_radius, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Draw Line with a given thickness
        static void DrawThickLine(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec3& colour);
        static void DrawThickLine(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        static void DrawThickLineNDT(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec3& colour);
        static void DrawThickLineNDT(const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Draw line with thickness of 1 screen pixel regardless of distance from camera
        static void DrawHairLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& colour);
        static void DrawHairLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        static void DrawHairLineNDT(const glm::vec3& start, const glm::vec3& end, const glm::vec3& colour);
        static void DrawHairLineNDT(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Draw Matrix (x,y,z axis at pos)
        static void DrawMatrix(const glm::mat4& transform_mtx);
        static void DrawMatrix(const glm::mat3& rotation_mtx, const glm::vec3& position);
        static void DrawMatrixNDT(const glm::mat4& transform_mtx);
        static void DrawMatrixNDT(const glm::mat3& rotation_mtx, const glm::vec3& position);

        // Draw Triangle
        static void DrawTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        static void DrawTriangleNDT(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
        static void DrawPolygon(int n_verts, const glm::vec3* verts, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        static void DrawPolygonNDT(int n_verts, const glm::vec3* verts, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        static void DebugDraw(const Maths::BoundingBox& box, const glm::vec4& edgeColour, bool cornersOnly = false, float width = 0.02f);
        static void DebugDraw(const Maths::BoundingSphere& sphere, const glm::vec4& colour);
        static void DebugDraw(Maths::Frustum& frustum, const glm::vec4& colour);
        static void DebugDraw(Graphics::Light* light, const glm::quat& rotation, const glm::vec4& colour);
        static void DebugDraw(const Maths::Ray& ray, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float distance = 1000.0f);
        static void DebugDraw(SoundNode* sound, const glm::vec4& colour);
        static void DebugDrawSphere(float radius, const glm::vec3& position, const glm::vec4& colour);
        static void DebugDrawCircle(int numVerts, float radius, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& colour);
        static void DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& colour);
        static void DebugDrawCapsule(const glm::vec3& position, const glm::quat& rotation, float height, float radius, const glm::vec4& colour);

        const std::vector<TriangleInfo>& GetTriangles(bool depthTested = false) const { return (depthTested ? m_DrawList.m_DebugTriangles : m_DrawListNDT.m_DebugTriangles); }
        const std::vector<LineInfo>& GetLines(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugLines : m_DrawListNDT.m_DebugLines; }
        const std::vector<LineInfo>& GetThickLines(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugThickLines : m_DrawListNDT.m_DebugThickLines; }
        const std::vector<PointInfo>& GetPoints(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugPoints : m_DrawListNDT.m_DebugPoints; }

        static DebugRenderer* GetInstance()
        {
            return s_Instance;
        }

    protected:
        // Actual functions managing data parsing to save code bloat - called by public functions
        static void GenDrawPoint(bool ndt, const glm::vec3& pos, float point_radius, const glm::vec4& colour);
        static void GenDrawThickLine(bool ndt, const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour);
        static void GenDrawHairLine(bool ndt, const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour);
        static void GenDrawTriangle(bool ndt, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour);

    private:
        void ClearInternal();

        static DebugRenderer* s_Instance;

        struct DebugDrawList
        {
            std::vector<TriangleInfo> m_DebugTriangles;
            std::vector<LineInfo> m_DebugLines;
            std::vector<PointInfo> m_DebugPoints;
            std::vector<LineInfo> m_DebugThickLines;
        };

        DebugDrawList m_DrawList;
        DebugDrawList m_DrawListNDT;
    };
}
