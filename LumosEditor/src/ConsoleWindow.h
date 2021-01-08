#pragma once

#include "EditorWindow.h"

#include <imgui/imgui.h>
#include "Maths/Colour.h"

namespace Lumos
{
	class ConsoleWindow : public EditorWindow
	{
	public:
		class Message
		{
		public:
			enum Level : u32
			{
				Trace = 1,
				Debug = 2,
                Info = 4,
				Warn = 8,
				Error = 16,
				Critical = 32,
			};

		public:
			Message(const std::string& message = "", Level level = Level::Trace, const std::string& source = "", int threadID = 0);
			void OnImGUIRender();
			void IncreaseCount() { m_Count++; };
			size_t GetMessageID() const { return m_MessageID; }
			
			static const char* GetLevelName(Level level);
			static const char* GetLevelIcon(Level level);
            static Maths::Colour GetRenderColour(Level level);
			
		public:
			const std::string m_Message;
			const Level m_Level;
			const std::string m_Source;
			const int m_ThreadID;
			int m_Count = 1;
			size_t m_MessageID;
		};

		ConsoleWindow();
		~ConsoleWindow() = default;
		static void Flush();
		void OnImGui() override;

		static void AddMessage(const Ref<Message>& message);

	private:
		void ImGuiRenderHeader();
		void ImGuiRenderMessages();

	private:
		static uint16_t s_MessageBufferCapacity;
		static uint16_t s_MessageBufferSize;
		static uint16_t s_MessageBufferBegin;
		static std::vector<Ref<Message>> s_MessageBuffer;
		static bool s_AllowScrollingToBottom;
		static bool s_RequestScrollToBottom;
		static u32 s_MessageBufferRenderFilter;
		ImGuiTextFilter Filter;
	};
}
