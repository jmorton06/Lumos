#pragma once
#include "lmpch.h"
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
			enum class Level
			{
				Invalid = -1,
				Trace = 0,
				Debug = 1,
				Info = 2,
				Warn = 3,
				Error = 4,
				Critical = 5,
				Off = 6, // Display nothing
			};
		public:
			Message(const String& message = "", Level level = Level::Invalid, const String& source = "", int threadID = 0);
			void OnImGUIRender();

			static Level GetLowerLevel(Level level);
			static Level GetHigherLevel(Level level);
			static const char* GetLevelName(Level level);
		private:
			static Maths::Colour GetRenderColour(Level level);
		public:
			const std::string m_Message;
			const Level m_Level;
			const String m_Source;
			const int m_ThreadID;
			static std::vector<Level> s_Levels;
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
		static Message::Level s_MessageBufferRenderFilter;
		ImGuiTextFilter Filter;
	};
}
