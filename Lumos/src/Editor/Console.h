#pragma once
#include "LM.h"
#include "Utilities/TSingleton.h"

#include <imgui/imgui.h>

namespace Lumos
{
	class Console : public TSingleton<Console>
	{
		friend class TSingleton<Console>;
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
		private:
			struct Colour { float r, g, b, a; };
		public:
			Message(const String& message = "", Level level = Level::Invalid, const String& source = "", int threadID = 0);
			void OnImGUIRender();

			static Level GetLowerLevel(Level level);
			static Level GetHigherLevel(Level level);
			static const char* GetLevelName(Level level);
		private:
			static Colour GetRenderColour(Level level);
		public:
			const std::string m_Message;
			const Level m_Level;
			const String m_Source;
			const int m_ThreadID;
			static std::vector<Level> s_Levels;
		};

        Console();
		~Console() = default;
		void AddMessage(std::shared_ptr<Message> message);
		void Flush();
		void OnImGuiRender(bool* show);

	private:
			void ImGuiRenderHeader();
			void ImGuiRenderMessages();
	private:
		uint16_t m_MessageBufferCapacity;
		uint16_t m_MessageBufferSize;
		uint16_t m_MessageBufferBegin;
		std::vector<std::shared_ptr<Message>> m_MessageBuffer;
		bool m_AllowScrollingToBottom;
		bool m_RequestScrollToBottom;
		static Message::Level s_MessageBufferRenderFilter;
		ImGuiTextFilter Filter;
	};
}
