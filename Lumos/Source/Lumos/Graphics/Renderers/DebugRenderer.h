#pragma once
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
#define MAX_LOG_SIZE 25
#define LOG_TEXT_SIZE 14.0f
#define STATUS_TEXT_SIZE 16.0f

    struct LogEntry
    {
        Vec4 colour;
        std::string text;
    };

    struct DebugText
    {
        Vec4 colour;
        std::string text;
        float Size;
        Vec4 Position;
        float time;
    };

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
        Vec3 p1;
        Vec3 p2;
        Vec4 col;
        float time = 0.0f;

        LineInfo() = default;
        LineInfo(const Vec3& pos1, const Vec3& pos2, const Vec4& colour, float t = 0.0f)
        {
            p1   = pos1;
            p2   = pos2;
            col  = colour;
            time = t;
        }
    };

    struct PointInfo
    {
        Vec3 p1;
        Vec4 col;
        float size;
        float time = 0.0f;

        PointInfo() = default;
        PointInfo(const Vec3& pos1, float s, const Vec4& colour, float t = 0.0f)
        {
            p1   = pos1;
            size = s;
            col  = colour;
            time = t;
        }
    };

    struct TriangleInfo
    {
        Vec3 p1;
        Vec3 p2;
        Vec3 p3;
        Vec4 col;
        float time     = 0.0f;
        TriangleInfo() = default;
        TriangleInfo(const Vec3& pos1, const Vec3& pos2, const Vec3& pos3, const Vec4& colour, float t = 0.0f)
        {
            p1   = pos1;
            p2   = pos2;
            p3   = pos3;
            col  = colour;
            time = t;
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
        friend class SceneRenderer;

    public:
        static void Init();
        static void Release();
        static void Reset(float dt);

        DebugRenderer();
        ~DebugRenderer();

        // Note: Functions appended with 'NDT' (no depth testing) will always be rendered in the foreground. This can be useful for debugging things inside objects.

        // Draw Point (circle)
        static void DrawPoint(const Vec3& pos, float point_radius, bool depthTested = false, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Line with a given thickness
        static void DrawThickLine(const Vec3& start, const Vec3& end, float line_width, bool depthTested = false, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw line with thickness of 1 screen pixel regardless of distance from camera
        static void DrawHairLine(const Vec3& start, const Vec3& end, bool depthTested = false, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Matrix (x,y,z axis at pos)
        static void DrawMatrix(const Mat4& transform_mtx, bool depthTested = false, float time = 0.0f);
        static void DrawMatrix(const Mat3& rotation_mtx, const Vec3& position, bool depthTested = false, float time = 0.0f);

        // Draw Triangle
        static void DrawTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, bool depthTested = false, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
        static void DrawPolygon(int n_verts, const Vec3* verts, bool depthTested = false, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Text WorldSpace (pos given here in worldspace)
        static void DrawTextWs(const Vec3& pos, const float font_size, bool depthTested, const Vec4& colour, float time, const std::string text, ...); /// See "printf" for usage manual

        // Draw Text (pos is assumed to be pre-multiplied by projMtx * viewMtx at this point)
        static void DrawTextCs(const Vec4& pos, const float font_size, const std::string& text, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Add a status entry at the top left of the screen (Cleared each frame)
        static void AddStatusEntry(const Vec4& colour, const std::string text, ...); /// See "printf" for usuage manual

        // Add a log entry at the bottom left - persistent until scene reset
        static void Log(const Vec3& colour, const std::string text, ...); /// See "printf" for usuage manual
        static void Log(const std::string text, ...);                     // Default Text Colour
        static void LogE(const char* filename, int linenumber, const std::string text, ...);

        static void SortLists();
        static void ClearLogEntries();

        static void DebugDraw(const Maths::BoundingBox& box, const Vec4& edgeColour, bool cornersOnly = false, bool depthTested = false, float width = 0.02f);
        static void DebugDraw(const Maths::BoundingSphere& sphere, const Vec4& colour);
        static void DebugDraw(Maths::Frustum& frustum, const Vec4& colour);
        static void DebugDraw(Graphics::Light* light, const Quat& rotation, const Vec4& colour);
        static void DebugDraw(const Maths::Ray& ray, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f), float distance = 1000.0f);
        static void DebugDraw(SoundNode* sound, const Vec4& colour);
        static void DebugDrawSphere(float radius, const Vec3& position, const Vec4& colour);
        static void DebugDrawCircle(int numVerts, float radius, const Vec3& position, const Quat& rotation, const Vec4& colour);
        static void DebugDrawCone(int numCircleVerts, int numLinesToCircle, float angle, float length, const Vec3& position, const Quat& rotation, const Vec4& colour);
        static void DebugDrawCapsule(const Vec3& position, const Quat& rotation, float height, float radius, const Vec4& colour);
        static void DebugDrawBone(const Vec3& parent, const Vec3& child, const Vec4& colour = Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        const TDArray<TriangleInfo>& GetTriangles(bool depthTested = false) const { return (depthTested ? m_DrawList.m_DebugTriangles : m_DrawListNDT.m_DebugTriangles); }
        const TDArray<LineInfo>& GetLines(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugLines : m_DrawListNDT.m_DebugLines; }
        const TDArray<LineInfo>& GetThickLines(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugThickLines : m_DrawListNDT.m_DebugThickLines; }
        const TDArray<PointInfo>& GetPoints(bool depthTested = false) const { return depthTested ? m_DrawList.m_DebugPoints : m_DrawListNDT.m_DebugPoints; }

        const TDArray<LogEntry>& GetLogEntries() const { return m_vLogEntries; }
        const TDArray<DebugText>& GetDebugText() const { return m_TextList; }
        const TDArray<DebugText>& GetDebugTextNDT() const { return m_TextListNDT; }
        const TDArray<DebugText>& GetDebugTextCS() const { return m_TextListCS; }

        // const TDArray<Vec4>& GetTextChars() const { return m_vChars; }

        void SetDimensions(uint32_t width, uint32_t height)
        {
            m_Width  = width;
            m_Height = height;
        }
        void SetProjView(const Mat4& projView) { m_ProjViewMtx = projView; }

        static DebugRenderer* GetInstance()
        {
            return s_Instance;
        }

    protected:
        // Actual functions managing data parsing to save code bloat - called by public functions
        static void GenDrawPoint(bool ndt, const Vec3& pos, float point_radius, const Vec4& colour, float time);
        static void GenDrawThickLine(bool ndt, const Vec3& start, const Vec3& end, float line_width, const Vec4& colour, float time);
        static void GenDrawHairLine(bool ndt, const Vec3& start, const Vec3& end, const Vec4& colour, float time);
        static void GenDrawTriangle(bool ndt, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec4& colour, float time);
        static void AddLogEntry(const Vec3& colour, const std::string& text);

    private:
        void ClearInternal();

        static DebugRenderer* s_Instance;

        struct DebugDrawList
        {
            TDArray<TriangleInfo> m_DebugTriangles;
            TDArray<LineInfo> m_DebugLines;
            TDArray<PointInfo> m_DebugPoints;
            TDArray<LineInfo> m_DebugThickLines;
        };

        int m_NumStatusEntries;
        float m_MaxStatusEntryWidth;
        TDArray<LogEntry> m_vLogEntries;
        int m_LogEntriesOffset;

        TDArray<DebugText> m_TextList;
        TDArray<DebugText> m_TextListNDT;
        TDArray<DebugText> m_TextListCS;

        // TDArray<Vec4> m_vChars;
        size_t m_OffsetChars;
        DebugDrawList m_DrawList;
        DebugDrawList m_DrawListNDT;

        void ClearDrawList(DebugDrawList& drawlist, float dt);

        Mat4 m_ProjViewMtx = Mat4(1.0f);
        uint32_t m_Width;
        uint32_t m_Height;
    };
}
