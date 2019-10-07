#pragma once
#include "lmpch.h"
#include "EditorWindow.h"
#include "Core/Profiler.h"
#include "Maths/Maths.h"
#include <imgui/imgui.h>

namespace Lumos
{
	class ProfilerGraph
	{
	public:
		ProfilerGraph();

		void Update();
		static void Rect(ImDrawList *drawList, const Maths::Vector2& minPoint, const Maths::Vector2& maxPoint, uint32_t col, bool filled = true);
		static void Text(ImDrawList *drawList, const Maths::Vector2& point, uint32_t col, const char *text);
		static void Triangle(ImDrawList *drawList, const std::array<Maths::Vector2, 3>& points, uint32_t col, bool filled = true);
		static void RenderTaskMarker(ImDrawList *drawList, const Maths::Vector2& leftMinPoint, const Maths::Vector2& leftMaxPoint, const Maths::Vector2& rightMinPoint, const Maths::Vector2& rightMaxPoint, uint32_t col);

        void FindMaxFrameTime();
		void RenderGraph(ImDrawList *drawList, const Maths::Vector2& graphPos, const Maths::Vector2& graphSize, size_t frameIndexOffset);
		void RenderLegend(ImDrawList *drawList, const Maths::Vector2& legendPos, const Maths::Vector2& legendSize, size_t frameIndexOffset);
		void RenderTimings(int graphWidth, int legendWidth, int height, int frameIndexOffset);

		void RebuildTaskStats(size_t endFrame, size_t framesCount);
        
        uint32_t GetColour(const String& name);

		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;
		size_t currFrameIndex = 0;
        float maxFrameTime = 0.0f;

		struct TaskStats
		{
			double maxTime;
			size_t priorityOrder;
			size_t onScreenIndex;
		};
		std::vector<TaskStats> taskStats;
		std::map<std::string, size_t> taskNameToStatsIndex;

		std::vector<ProfilerReport> m_Reports;
        
        int colourIndex = 0;
        std::unordered_map<String, uint32_t> m_ColourMap;
	};
	class ProfilerWindow : public EditorWindow
	{
	public:
		ProfilerWindow();
		~ProfilerWindow() = default;

		void OnImGui() override;
	private:
		float m_UpdateFrequency;
		float m_UpdateTimer;

		ProfilerGraph m_CPUGraph;

		bool stopProfiling;
		int frameOffset;
		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
		TimePoint prevFpsFrameTime;
		size_t fpsFramesCount;
		float avgFrameTime;
	};
}
