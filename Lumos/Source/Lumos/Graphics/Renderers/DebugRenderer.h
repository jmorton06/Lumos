#pragma once
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace Lumos
{
#define MAX_LOG_SIZE 25
#define LOG_TEXT_SIZE 14.0f
#define STATUS_TEXT_SIZE 16.0f

    struct LogEntry
    {
        glm::vec4 colour;
        std::string text;
    };

    struct DebugText
    {
        glm::vec4 colour;
        std::string text;
        float Size;
        glm::vec4 Position;
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
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec4 col;

        float time = 0.0f;

        LineInfo(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec4& colour, float t = 0.0f)
        {
            p1   = pos1;
            p2   = pos2;
            col  = colour;
            time = t;
        }
    };

    struct PointInfo
    {
        glm::vec3 p1;
        glm::vec4 col;
        float size;
        float time = 0.0f;

        PointInfo(const glm::vec3& pos1, float s, const glm::vec4& colour, float t = 0.0f)
        {
            p1   = pos1;
            size = s;
            col  = colour;
            time = t;
        }
    };

    struct TriangleInfo
    {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
        glm::vec4 col;
        float time = 0.0f;

        TriangleInfo(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3, const glm::vec4& colour, float t = 0.0f)
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
        friend class RenderPasses;

    public:
        static void Init();
        static void Release();
        static void Reset(float dt);

        DebugRenderer();
        ~DebugRenderer();

        // Note: Functions appended with 'NDT' (no depth testing) will always be rendered in the foreground. This can be useful for debugging things inside objects.

        // Draw Point (circle)
        static void DrawPoint(const glm::vec3& pos, float point_radius, bool depthTested = false, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Line with a given thickness
        static void DrawThickLine(const glm::vec3& start, const glm::vec3& end, float line_width, bool depthTested = false, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw line with thickness of 1 screen pixel regardless of distance from camera
        static void DrawHairLine(const glm::vec3& start, const glm::vec3& end, bool depthTested = false, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Matrix (x,y,z axis at pos)
        static void DrawMatrix(const glm::mat4& transform_mtx, bool depthTested = false, float time = 0.0f);
        static void DrawMatrix(const glm::mat3& rotation_mtx, const glm::vec3& position, bool depthTested = false, float time = 0.0f);

        // Draw Triangle
        static void DrawTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, bool depthTested = false, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Polygon (Renders as a triangle fan, so verts must be arranged in order)
        static void DrawPolygon(int n_verts, const glm::vec3* verts, bool depthTested = false, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float time = 0.0f);

        // Draw Text WorldSpace (pos given here in worldspace)
        static void DrawTextWs(const glm::vec3& pos, const float font_size, bool depthTested, const glm::vec4& colour, float time, const std::string text, ...); /// See "printf" for usage manual

        // Draw Text (pos is assumed to be pre-multiplied by projMtx * viewMtx at this point)
        static void DrawTextCs(const glm::vec4& pos, const float font_size, const std::string& text, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Add a status entry at the top left of the screen (Cleared each frame)
        static void AddStatusEntry(const glm::vec4& colour, const std::string text, ...); /// See "printf" for usuage manual

        // Add a log entry at the bottom left - persistent until scene reset
        static void Log(const glm::vec3& colour, const std::string text, ...); /// See "printf" for usuage manual
        static void Log(const std::string text, ...);                          // Default Text Colour
        static void LogE(const char* filename, int linenumber, const std::string text, ...);

        static void SortLists();
        static void ClearLogEntries();

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

        const std::vector<LogEntry>& GetLogEntries() const { return m_vLogEntries; }
        const std::vector<DebugText>& GetDebugText() const { return m_TextList; }
        const std::vector<DebugText>& GetDebugTextNDT() const { return m_TextListNDT; }
        const std::vector<DebugText>& GetDebugTextCS() const { return m_TextListCS; }

        // const std::vector<glm::vec4>& GetTextChars() const { return m_vChars; }

        void SetDimensions(uint32_t width, uint32_t height)
        {
            m_Width  = width;
            m_Height = height;
        }
        void SetProjView(const glm::mat4& projView) { m_ProjViewMtx = projView; }

        static DebugRenderer* GetInstance()
        {
            return s_Instance;
        }

    protected:
        // Actual functions managing data parsing to save code bloat - called by public functions
        static void GenDrawPoint(bool ndt, const glm::vec3& pos, float point_radius, const glm::vec4& colour, float time);
        static void GenDrawThickLine(bool ndt, const glm::vec3& start, const glm::vec3& end, float line_width, const glm::vec4& colour, float time);
        static void GenDrawHairLine(bool ndt, const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour, float time);
        static void GenDrawTriangle(bool ndt, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour, float time);
        static void AddLogEntry(const glm::vec3& colour, const std::string& text);

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

        int m_NumStatusEntries;
        float m_MaxStatusEntryWidth;
        std::vector<LogEntry> m_vLogEntries;
        int m_LogEntriesOffset;

        std::vector<DebugText> m_TextList;
        std::vector<DebugText> m_TextListNDT;
        std::vector<DebugText> m_TextListCS;

        // std::vector<glm::vec4> m_vChars;
        size_t m_OffsetChars;
        DebugDrawList m_DrawList;
        DebugDrawList m_DrawListNDT;

        void ClearDrawList(DebugDrawList& drawlist, float dt);

        glm::mat4 m_ProjViewMtx = glm::mat4(1.0f);
        uint32_t m_Width;
        uint32_t m_Height;
    };
}
