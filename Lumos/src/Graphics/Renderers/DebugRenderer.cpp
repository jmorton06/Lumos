#include "LM.h"
#include "DebugRenderer.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "App/Window.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/Material.h"
#include "Graphics/API/VertexArray.h"

#include "Maths/MathsUtilities.h"
#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"

#include <cstdarg>
#include <cstdio>

namespace Lumos
{
	Maths::Vector3	DebugRenderer::m_CameraPosition;
	Maths::Matrix4	DebugRenderer::m_ProjMtx;
	Maths::Matrix4	DebugRenderer::m_ViewMtx;
	Maths::Matrix4	DebugRenderer::m_ProjViewMtx;

	int DebugRenderer::m_NumStatusEntries = 0;
	float DebugRenderer::m_MaxStatusEntryWidth = 0.0f;
	std::vector<LogEntry> DebugRenderer::m_vLogEntries;
	int DebugRenderer::m_LogEntriesOffset = 0;
	size_t	DebugRenderer::m_OffsetChars = 0;

	std::vector<Maths::Vector4> DebugRenderer::m_vChars;
	DebugRenderer::DebugDrawList DebugRenderer::m_DrawList;
	DebugRenderer::DebugDrawList DebugRenderer::m_DrawListNDT;

	Material* DebugRenderer::m_pShaderPoints = nullptr;
	Material* DebugRenderer::m_pShaderLines = nullptr;
	Material* DebugRenderer::m_pShaderHairLines = nullptr;
	Material* DebugRenderer::m_pShaderText = nullptr;

	VertexArray* DebugRenderer::m_VertexArray = nullptr;

	Texture2D* DebugRenderer::m_FontTex = nullptr;

	uint DebugRenderer::m_Width = 0;
	uint DebugRenderer::m_Height = 0;

#ifdef LUMOS_PLATFORM_WINDOWS
#define VSNPRINTF( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList ) vsnprintf_s( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList )
#elif LUMOS_PLATFORM_MACOS
#define VSNPRINTF( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList ) vsnprintf_l( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList )
#elif LUMOS_PLATFORM_LINUX
#define VSNPRINTF( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList ) vsnprintf( _DstBuf, _DstSize, _Format, _ArgList )
#elif LUMOS_PLATFORM_MOBILE
#define VSNPRINTF( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList ) 0
#else
#define VSNPRINTF( _DstBuf, _DstSize, _MaxCount, _Format, _ArgList ) 0
#endif

#ifndef LUMOS_PLATFORM_WINDOWS
#define _TRUNCATE 0
#endif

	//Draw Point (circle)
	void DebugRenderer::GenDrawPoint(bool ndt, const Maths::Vector3& pos, float point_radius, const Maths::Vector4& colour)
	{
		auto list = ndt ? &m_DrawListNDT : &m_DrawList;
		list->_vPoints.emplace_back(pos, point_radius);
		list->_vPoints.push_back(colour);
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
		auto list = ndt ? &m_DrawListNDT : &m_DrawList;

		//For Depth Sorting
		Maths::Vector3 midPoint = (start + end) * 0.5f;
		float camDist = Maths::Vector3::Dot(midPoint - m_CameraPosition, midPoint - m_CameraPosition);

		//Add to Data Structures
		list->_vThickLines.emplace_back(start, line_width);
		list->_vThickLines.push_back(colour);

		list->_vThickLines.emplace_back(end, camDist);
		list->_vThickLines.push_back(colour);

		GenDrawPoint(ndt, start, line_width * 0.5f, colour);
		GenDrawPoint(ndt, end, line_width * 0.5f, colour);
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
		auto list = ndt ? &m_DrawListNDT : &m_DrawList;
		list->_vHairLines.emplace_back(start, 1.0f);
		list->_vHairLines.push_back(colour);

		list->_vHairLines.emplace_back(end, 1.0f);
		list->_vHairLines.push_back(colour);
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
		Maths::Vector3 position = mtx.GetPositionVector();
		GenDrawHairLine(false, position, position + Maths::Vector3(mtx.values[0], mtx.values[1], mtx.values[2]), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		GenDrawHairLine(false, position, position + Maths::Vector3(mtx.values[4], mtx.values[5], mtx.values[6]), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		GenDrawHairLine(false, position, position + Maths::Vector3(mtx.values[8], mtx.values[9], mtx.values[10]), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
	void DebugRenderer::DrawMatrix(const Maths::Matrix3& mtx, const Maths::Vector3& position)
	{
		GenDrawHairLine(false, position, position + mtx.GetCol(0), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		GenDrawHairLine(false, position, position + mtx.GetCol(1), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		GenDrawHairLine(false, position, position + mtx.GetCol(2), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
	void DebugRenderer::DrawMatrixNDT(const Maths::Matrix4& mtx)
	{
		Maths::Vector3 position = mtx.GetPositionVector();
		GenDrawHairLine(true, position, position + Maths::Vector3(mtx.values[0], mtx.values[1], mtx.values[2]), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		GenDrawHairLine(true, position, position + Maths::Vector3(mtx.values[4], mtx.values[5], mtx.values[6]), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		GenDrawHairLine(true, position, position + Maths::Vector3(mtx.values[8], mtx.values[9], mtx.values[10]), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
	void DebugRenderer::DrawMatrixNDT(const Maths::Matrix3& mtx, const Maths::Vector3& position)
	{
		GenDrawHairLine(true, position, position + mtx.GetCol(0), Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		GenDrawHairLine(true, position, position + mtx.GetCol(1), Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		GenDrawHairLine(true, position, position + mtx.GetCol(2), Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}

	//Draw Triangle
	void DebugRenderer::GenDrawTriangle(bool ndt, const Maths::Vector3& v0, const Maths::Vector3& v1, const Maths::Vector3& v2, const Maths::Vector4& colour)
	{
		auto list = ndt ? &m_DrawListNDT : &m_DrawList;

		//For Depth Sorting
		Maths::Vector3 midPoint = (v0 + v1 + v2) * (1.0f / 3.0f);
		float camDist = Maths::Vector3::Dot(midPoint - m_CameraPosition, midPoint - m_CameraPosition);

		//Add to data structures
		list->_vTris.emplace_back(v0, camDist);
		list->_vTris.push_back(colour);

		list->_vTris.emplace_back(v1, 1.0f);
		list->_vTris.push_back(colour);

		list->_vTris.emplace_back(v2, 1.0f);
		list->_vTris.push_back(colour);
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

	void DebugRenderer::DrawTextCs(const Maths::Vector4& cs_pos, const float font_size, const std::string& text, const TextAlignment alignment, const Maths::Vector4& colour)
	{
		Maths::Vector3 cs_size = Maths::Vector3(font_size / m_Width, font_size / m_Height, 0.0f);
		cs_size = cs_size * cs_pos.GetW();

		//Work out the starting position of text based off desired alignment
		float x_offset = 0.0f;
		const auto text_len = static_cast<int>(text.length());

		switch (alignment)
		{
		case TEXTALIGN_RIGHT:
			x_offset = -text_len * cs_size.GetX() * 1.2f;
			break;

		case TEXTALIGN_CENTRE:
			x_offset = -text_len * cs_size.GetX() * 0.6f;
			break;
		default:;

			//   case TEXTALIGN_LEFT:
			//     x_offset = -text_len * cs_size.GetX() * 0.6f;
		}

		//Add each characters to the draw list individually
		for (int i = 0; i < text_len; ++i)
		{
			Maths::Vector4 char_pos = Maths::Vector4(cs_pos.GetX() + x_offset, cs_pos.GetY(), cs_pos.GetZ(), cs_pos.GetW());
			Maths::Vector4 char_data = Maths::Vector4(cs_size.GetX(), cs_size.GetY(), static_cast<float>(text[i]), 0.0f);

			m_vChars.push_back(char_pos);
			m_vChars.push_back(char_data);
			m_vChars.push_back(colour);
			m_vChars.push_back(colour);	//We dont really need this, but we need the padding to match the same vertex format as all the other debug drawables

			x_offset += cs_size.GetX() * 1.2f;
		}
	}

	//Draw Text WorldSpace
	void DebugRenderer::DrawTextWs(const Maths::Vector3& pos, const float font_size, const TextAlignment alignment, const Maths::Vector4& colour, const std::string text, ...)
	{
		va_list args;
		va_start(args, text);

		char buf[1024];

		int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

		va_end(args);

		int length = (needed < 0) ? 1024 : needed;

		std::string formatted_text = std::string(buf, static_cast<size_t>(length));

		Maths::Vector4 cs_pos = m_ProjViewMtx * Maths::Vector4(pos, 1.0f);
		DrawTextCs(cs_pos, font_size, formatted_text, alignment, colour);
	}

	void DebugRenderer::DrawTextWsNDT(const Maths::Vector3& pos, const float font_size, const TextAlignment alignment, const Maths::Vector4& colour, const std::string text, ...)
	{
		va_list args;
		va_start(args, text);

		char buf[1024];

		int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

		va_end(args);

		int length = (needed < 0) ? 1024 : needed;

		String formatted_text = String(buf, static_cast<size_t>(length));

		Maths::Vector4 cs_pos = m_ProjViewMtx * Maths::Vector4(pos, 1.0f);
		cs_pos.SetZ(1.0f * cs_pos.GetW());
		DrawTextCs(cs_pos, font_size, formatted_text, alignment, colour);
	}

	//Status Entry
	void DebugRenderer::AddStatusEntry(const Maths::Vector4& colour, const std::string text, ...)
	{
		float cs_size_x = STATUS_TEXT_SIZE / m_Width * 2.0f;
		float cs_size_y = STATUS_TEXT_SIZE / m_Height * 2.0f;

		va_list args;
		va_start(args, text);

		char buf[1024];

		int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

		va_end(args);

		int length = (needed < 0) ? 1024 : needed;

		std::string formatted_text = std::string(buf, static_cast<size_t>(length));

		DrawTextCs(Maths::Vector4(-1.0f + cs_size_x * 0.5f, 1.0f - (m_NumStatusEntries * cs_size_y) - cs_size_y, -1.0f, 1.0f), STATUS_TEXT_SIZE, formatted_text, TEXTALIGN_LEFT, colour);
		m_NumStatusEntries++;
		m_MaxStatusEntryWidth = Maths::Max(m_MaxStatusEntryWidth, cs_size_x * 0.6f * length);
	}

	//Log

	void DebugRenderer::AddLogEntry(const Maths::Vector3& colour, const std::string& text)
	{
		/*	time_t now = time(0);
		tm ltm;
		localtime_s(&ltm, &now);*/

		std::stringstream ss;
		//ss << "[" << ltm.tm_hour << ":" << ltm.tm_min << ":" << ltm.tm_sec << "] ";

		LogEntry le;
		le.text = ss.str() + text;
		le.colour = Maths::Vector4(colour.GetX(), colour.GetY(), colour.GetZ(), 1.0f);

		if (m_vLogEntries.size() < MAX_LOG_SIZE)
			m_vLogEntries.push_back(le);
		else
		{
			m_vLogEntries[m_LogEntriesOffset] = le;
			m_LogEntriesOffset = (m_LogEntriesOffset + 1) % MAX_LOG_SIZE;
		}

		LUMOS_CORE_WARN(text);
	}

	void DebugRenderer::Log(const Maths::Vector3& colour, const std::string text, ...)
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
		AddLogEntry(Maths::Vector3(0.4f, 1.0f, 0.6f), std::string(buf, static_cast<size_t>(length)));
	}

	void DebugRenderer::LogE(const char* filename, int linenumber, const std::string text, ...)
	{
		//Error Format:
		//<text>
		//		-> <line number> : <file name>

		va_list args;
		va_start(args, text);

		char buf[1024];

		int needed = VSNPRINTF(buf, 1023, _TRUNCATE, text.c_str(), args);

		va_end(args);

		int length = (needed < 0) ? 1024 : needed;

		Log(Maths::Vector3(1.0f, 0.25f, 0.25f), "[ERROR] %s:%d", filename, linenumber);
		AddLogEntry(Maths::Vector3(1.0f, 0.5f, 0.5f), "\t \x01 \"" + std::string(buf, static_cast<size_t>(length)) + "\"");

		std::cout << std::endl;
	}

	void DebugRenderer::DebugDraw(Maths::BoundingBox* box, const Maths::Vector4 &edgeColour, float width)
	{
		Maths::Vector3 uuu = box->Upper();
		Maths::Vector3 lll = box->Lower();

		Maths::Vector3 ull(uuu.GetX(), lll.GetY(), lll.GetZ());
		Maths::Vector3 uul(uuu.GetX(), uuu.GetY(), lll.GetZ());
		Maths::Vector3 ulu(uuu.GetX(), lll.GetY(), uuu.GetZ());

		Maths::Vector3 luu(lll.GetX(), uuu.GetY(), uuu.GetZ());
		Maths::Vector3 llu(lll.GetX(), lll.GetY(), uuu.GetZ());
		Maths::Vector3 lul(lll.GetX(), uuu.GetY(), lll.GetZ());

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

	void DebugRenderer::DebugDraw(Maths::BoundingSphere* sphere, const Maths::Vector4 &colour)
	{
		Lumos::DebugRenderer::DrawPointNDT(sphere->Centre(), sphere->SphereRadius(), colour);
	}

	void DebugRenderer::ClearDebugLists()
	{
		m_vChars.clear();

		const auto clear_list = [](DebugRenderer::DebugDrawList& list)
		{
			list._vPoints.clear();
			list._vThickLines.clear();
			list._vHairLines.clear();
			list._vTris.clear();
		};
		clear_list(m_DrawList);
		clear_list(m_DrawListNDT);

		m_NumStatusEntries = 0;
		m_MaxStatusEntryWidth = 0.0f;
	}

	void DebugRenderer::ClearLog()
	{
		m_vLogEntries.clear();
		m_LogEntriesOffset = 0;
	}

	struct PointVertex
	{
		Maths::Vector4 pos;
		Maths::Vector4 col;
	};

	struct LineVertex
	{
		PointVertex p0;
		PointVertex p1;
	};

	struct TriVertex
	{
		PointVertex p0;
		PointVertex p1;
		PointVertex p2;
	};

	void DebugRenderer::SortDebugLists()
	{
		//Draw log text
		float cs_size_x = LOG_TEXT_SIZE / m_Width * 2.0f;
		float cs_size_y = LOG_TEXT_SIZE / m_Height * 2.0f;
		size_t log_len = m_vLogEntries.size();

		float max_x = 0.0f;
		for (size_t i = 0; i < log_len; ++i)
		{
			max_x = Maths::Max(max_x, m_vLogEntries[i].text.length() * cs_size_x * 0.6f);

			size_t idx = (i + m_LogEntriesOffset) % MAX_LOG_SIZE;

			DrawTextCs(Maths::Vector4(-1.0f + cs_size_x * 0.5f, -1.0f + ((log_len - i - 1) * cs_size_y) + cs_size_y, 0.0f, 1.0f), LOG_TEXT_SIZE, m_vLogEntries[idx].text, TEXTALIGN_LEFT, m_vLogEntries[idx].colour);
		}

		auto sort_lists = [](DebugRenderer::DebugDrawList& list)
		{
			//Sort Points
			if (!list._vPoints.empty())
			{
				auto * points = reinterpret_cast<PointVertex*>(&list._vPoints[0]);
				std::sort(points, points + list._vPoints.size() / 2, [&](const PointVertex& a, const PointVertex& b)
				{
					float a2 = Maths::Vector3::Dot(a.pos.ToVector3() - m_CameraPosition, a.pos.ToVector3() - m_CameraPosition);
					float b2 = Maths::Vector3::Dot(b.pos.ToVector3() - m_CameraPosition, b.pos.ToVector3() - m_CameraPosition);
					return (a2 > b2);
				});
			}

			//Sort Lines
			if (!list._vThickLines.empty())
			{
				auto * lines = reinterpret_cast<LineVertex*>(&list._vThickLines[0]);
				std::sort(lines, lines + list._vThickLines.size() / 4, [](const LineVertex& a, const LineVertex& b)
				{
					return (a.p1.pos.GetW() > b.p1.pos.GetW());
				});
			}

			//Sort Triangles
			if (!list._vTris.empty())
			{
				auto * tris = reinterpret_cast<TriVertex*>(&list._vTris[0]);
				std::sort(tris, tris + list._vTris.size() / 6, [](const TriVertex& a, const TriVertex& b)
				{
					return (a.p0.pos.GetW() > b.p0.pos.GetW());
				});
			}
			return false;
		};

		sort_lists(m_DrawList);
		sort_lists(m_DrawListNDT);

		//Draw background to text areas
		// - This is done last as to avoid additional triangle-sorting
		Maths::Matrix4 invProjView = Maths::Matrix4::Inverse(m_ProjViewMtx);
		float rounded_offset_x = 10.f / m_Width * 2.0f;
		float rounded_offset_y = 10.f / m_Height * 2.0f;
		const Maths::Vector4 log_background_col(0.1f, 0.1f, 0.1f, 0.5f);

		Maths::Vector3 centre;
		Maths::Vector3 last;
		const auto NextTri = [&](const Maths::Vector3& point)
		{
			DrawTriangleNDT(centre, last, point, log_background_col);
			DrawHairLineNDT(last, point);
			last = point;
		};

		//Draw Log Background
		if (!m_vLogEntries.empty())
		{
			float top_y = -1 + m_vLogEntries.size() * cs_size_y + cs_size_y;
			max_x = max_x - 1 + cs_size_x;

			centre = invProjView * Maths::Vector3(-1, -1, 0);
			last = invProjView * Maths::Vector3(max_x, -1, 0);
			NextTri(invProjView * Maths::Vector3(max_x, top_y - rounded_offset_y, 0.0f));
			for (int i = 0; i < 5; ++i)
			{
				Maths::Vector3 round_offset = Maths::Vector3(
						cos(Maths::DegreesToRadians(i * 22.5f)) * rounded_offset_x,
						sin(Maths::DegreesToRadians(i * 22.5f)) * rounded_offset_y,
					0.0f);
				NextTri(invProjView * Maths::Vector3(max_x + round_offset.GetX() - rounded_offset_x, top_y + round_offset.GetY() - rounded_offset_y, 0.0f));
			}
			NextTri(invProjView * Maths::Vector3(-1, top_y, 0.0f));
		}

		//Draw Status Background
		if (m_NumStatusEntries > 0)
		{
			cs_size_x = STATUS_TEXT_SIZE / m_Width * 2.0f;
			cs_size_y = STATUS_TEXT_SIZE / m_Height * 2.0f;

			const float btm_y = 1 - m_NumStatusEntries * cs_size_y - cs_size_y;
			max_x = -1 + cs_size_x + m_MaxStatusEntryWidth;

			centre = invProjView * Maths::Vector3(-1, 1, 0);
			last = invProjView * Maths::Vector3(-1, btm_y, 0);

			NextTri(invProjView * Maths::Vector3(max_x - rounded_offset_x, btm_y, 0.0f));
			for (int i = 4; i >= 0; --i)
			{
				const Maths::Vector3 round_offset = Maths::Vector3(
					cos(Maths::DegreesToRadians(i * 22.5f)) * rounded_offset_x,
					sin(Maths::DegreesToRadians(i * 22.5f)) * rounded_offset_y,
					0.0f);
				NextTri(invProjView * Maths::Vector3(max_x + round_offset.GetX() - rounded_offset_x, btm_y - round_offset.GetY() + rounded_offset_y, 0.0f));
			}
			NextTri(invProjView * Maths::Vector3(max_x, 1, 0.0f));
		}
	}

	void DebugRenderer::DrawDebugLists()
	{
	}

	//This Function has been abstracted from previous set of draw calls as to avoid supersampling the font bitmap.. which is bad enough as it is.
	void DebugRenderer::DrawDebubHUD()
	{

	}

	void DebugRenderer::Init()
	{
	}

	void DebugRenderer::Release()
	{
	}
}
