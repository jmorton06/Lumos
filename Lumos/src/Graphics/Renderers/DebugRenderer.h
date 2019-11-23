#pragma once

#include "lmpch.h"
#include "Maths/Maths.h"

#ifndef LUMOS_PLATFORM_WINDOWS
#include <signal.h>
#endif

namespace Lumos
{

#define MAX_LOG_SIZE		25
#define LOG_TEXT_SIZE  		14.0f
#define STATUS_TEXT_SIZE	16.0f

	enum TextAlignment
	{
		TEXTALIGN_LEFT,
		TEXTALIGN_RIGHT,
		TEXTALIGN_CENTRE
	};

	struct LogEntry
	{
		Maths::Vector4 colour;
		std::string text;
	};

	class Texture2D;
	class Material;
	class VertexArray;
	class IndexBuffer;

	namespace Maths
	{
		class Sphere;
		class BoundingBox;
	}

	class LUMOS_EXPORT DebugRenderer
	{
		friend class Scene;
		friend class GraphicsPipeline;
		friend class Application;
	public:
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

		//Draw Text WorldSpace (pos given here in worldspace)
		static void DrawTextWs(const Maths::Vector3& pos, const float font_size, const TextAlignment alignment, const Maths::Vector4& colour, const std::string text, ...); ///See "printf" for usage manual
		static void DrawTextWsNDT(const Maths::Vector3& pos, const float font_size, const TextAlignment alignment, const Maths::Vector4& colour, const std::string text, ...); ///See "printf" for usage manual

																																								 //Draw Text (pos is assumed to be pre-multiplied by projMtx * viewMtx at this point)
		static void DrawTextCs(const Maths::Vector4& pos, const float font_size, const std::string& text, const TextAlignment alignment = TEXTALIGN_LEFT, const Maths::Vector4& colour = Maths::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		//Add a status entry at the top left of the screen (Cleared each frame)
		static void AddStatusEntry(const Maths::Vector4& colour, const std::string text, ...); ///See "printf" for usuage manual

																						//Add a log entry at the bottom left - persistent until scene reset
		static void Log(const Maths::Vector3& colour, const std::string text, ...); ///See "printf" for usuage manual
		static void Log(const std::string text, ...); //Default Text Colour
		static void LogE(const char* filename, int linenumber, const std::string text, ...);

		static void DebugDraw(Maths::BoundingBox* box, const Maths::Vector4& edgeColour, float width = 0.02f);
		static void DebugDraw(Maths::Sphere* sphere, const Maths::Vector4& colour);

	protected:
		//Actual functions managing data parsing to save code bloat - called by public functions
		static void GenDrawPoint(bool ndt, const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour);
		static void GenDrawThickLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, float line_width, const Maths::Vector4& colour);
		static void GenDrawHairLine(bool ndt, const Maths::Vector3& start, const Maths::Vector3& end, const Maths::Vector4& colour);
		static void GenDrawTriangle(bool ndt, const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour);

		static void AddLogEntry(const Maths::Vector3& colour, const std::string& text);

		//Called by Scene Renderer class
		static void ClearDebugLists();
		static void SortDebugLists();
		static void DrawDebugLists();
		static void DrawDebubHUD();

		static void Init();
		static void Release();

		static void ClearLog();

		static void SetDebugDrawData(const Maths::Matrix4& projMtx, const Maths::Matrix4& viewMtx, const Maths::Vector3& camera_pos)
		{
			m_ProjMtx = projMtx;
			m_ViewMtx = viewMtx;
			m_ProjViewMtx = projMtx * viewMtx;
			m_CameraPosition = camera_pos;
		}

	protected:
		static Maths::Vector3  m_CameraPosition;
		static Maths::Matrix4  m_ProjMtx;
		static Maths::Matrix4  m_ViewMtx;
		static Maths::Matrix4  m_ProjViewMtx;
		static int m_NumStatusEntries;
		static float m_MaxStatusEntryWidth;
		static std::vector<LogEntry> m_vLogEntries;
		static int m_LogEntriesOffset;
		static u32 m_Width;
		static u32 m_Height;

		static std::vector<Maths::Vector4> m_vChars;
		struct DebugDrawList
		{
			std::vector<Maths::Vector4> _vPoints;
			std::vector<Maths::Vector4> _vThickLines;
			std::vector<Maths::Vector4> _vHairLines;
			std::vector<Maths::Vector4> _vTris;
		};
		static DebugDrawList m_DrawList;			//Depth-Tested
		static DebugDrawList m_DrawListNDT;			//Not Depth-Tested

		static VertexArray* m_VertexArray;
		static Texture2D*	m_FontTex;
		static size_t	m_OffsetChars;
	};
}
