#pragma once

namespace Lumos
{
	enum class RenderPriority
	{
		Geometry = 0,
		Lighting = 1,
		Geometry2D = 2,
		PostProcess = 3,
		Debug = 4,
		ImGui = 5,
		Screen = 6,
		Total = 7
	};

	namespace Graphics
	{
		class IRenderer;

		class RenderGraph
		{
		public:
			RenderGraph();
			~RenderGraph();

			void AddRenderer(Graphics::IRenderer* renderer);
			void SortRenderers();

		private:
			std::vector<Graphics::IRenderer*> m_Renderers;
		};
	}
}
