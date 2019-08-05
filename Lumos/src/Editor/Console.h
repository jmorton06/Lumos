#pragma once
#include "LM.h"

namespace Lumos
{
	class Console 
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
		private:
			struct Color { float r, g, b, a; };
		public:
			Message(const std::string message = "", Level level = Level::Invalid);
			void OnImGUIRender();

			static Level GetLowerLevel(Level level);
			static Level GetHigherLevel(Level level);
			static const char* GetLevelName(Level level);
		private:
			static Color GetRenderColor(Level level);
		public:
			const std::string m_Message;
			const Level m_Level;
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
	};
}
