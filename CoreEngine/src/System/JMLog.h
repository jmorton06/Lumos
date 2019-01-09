#pragma once

#include "JM.h"

namespace jm
{

#define JM_LOG_LEVEL_FATAL 0
#define JM_LOG_LEVEL_ERROR 1
#define JM_LOG_LEVEL_WARN  2
#define JM_LOG_LEVEL_INFO  3

#define LOGTOFILE

	static char to_string_buffer[1024 * 10];
	static char sprintf_buffer[1024 * 10];

	namespace JMLog
	{
		JM_EXPORT void PlatformLogMessage(uint level, const char* message);
		
		inline void PrintToFile(const char* buffer, uint level, const String& fileName)
		{
			::std::ofstream log_file(fileName, std::ios_base::out | std::ios_base::app);
			log_file << buffer;
		}

		template <typename T>
		inline const char* to_string_internal(const T& v, const std::true_type& ignored)
		{
			sprintf(to_string_buffer, "Container of size: %d, contents: %s", v.size(), format_iterators(v.begin(), v.end()).c_str());
			return to_string_buffer;
		}

		template <typename T>
		inline const char* to_string_internal(const T& t, const std::false_type& ignored)
		{
			auto x = StringFormat::ToString(t);
			return strcpy(to_string_buffer, x.c_str());
		}

		template <class T>
		struct has_iterator
		{
			template<class U> static char(&test(typename U::iterator const*))[1];
			template<class U> static char(&test(...))[2];
			static const bool value = (sizeof(test<T>(0)) == 1);
		};

		template <typename T>
		inline const char* to_string(const T& t)
		{
			return to_string_internal<T>(t, std::integral_constant<bool, has_iterator<T>::value>());
		}

		template <>
		inline const char* to_string<char>(const char& t)
		{
			return &t;
		}

		template <>
		inline const char* to_string<char*>(char* const& t)
		{
			return t;
		}

		template <>
		inline const char* to_string<unsigned char const*>(unsigned char const* const& t)
		{
			return reinterpret_cast<const char*>(t);
		}

		template <>
		inline const char* to_string<wchar_t*>(wchar_t* const& t)
		{
			wcstombs(sprintf_buffer, t, 1024 * 10);
			return sprintf_buffer;
		}

		template <>
		inline const char* to_string<const wchar_t*>(const wchar_t* const& t)
		{
			wcstombs(sprintf_buffer, t, 1024 * 10);
			return sprintf_buffer;
		}

		template <>
		inline const char* to_string<const char*>(const char* const& t)
		{
			return t;
		}

		template <>
		inline const char* to_string<String>(const String& t)
		{
			return t.c_str();
		}

		template <>
		inline const char* to_string<bool>(const bool& t)
		{
			return t ? "true" : "false";
		}

		template <typename T>
		inline String format_iterators(T& begin, T& end)
		{
			String result;
			bool first = true;
			while (begin != end)
			{
				if (!first)
					result += ", ";
				result += to_string(*begin);
				first = false;
				begin++;
			}
			return result;
		}

		template<>
		inline const char* to_string_internal<const char*>(const char* const& v, const ::std::false_type& ignored)
		{
			return v;
		}

		template <typename First>
		inline void print_log_internal(char* buffer, int32& position, First&& first)
		{
			const char* formatted = jm::JMLog::to_string<First>(first);
			const int32 length = static_cast<int32>(strlen(formatted));
			memcpy(&buffer[position], formatted, length);
			position += length;
		}

		template <typename First, typename... Args>
		inline void print_log_internal(char* buffer, int32& position, First&& first, Args&&... args)
		{
			const char* formatted = jm::JMLog::to_string<First>(first);
			const int32 length = static_cast<int32>(strlen(formatted));
			memcpy(&buffer[position], formatted, length);
			position += length;
			if (sizeof...(Args))
				print_log_internal(buffer, position, std::forward<Args>(args)...);
		}

		template <typename... Args>
		inline void log_message(int32 level, bool newline, Args... args)
		{
			char buffer[1024 * 10];
			int32 position = 0;
			print_log_internal(buffer, position, std::forward<Args>(args)...);

			if (newline)
				buffer[position++] = '\n';

			buffer[position] = 0;

#ifdef LOGTOFILE
			PrintToFile(buffer, level, "../bin/JMEngineLog.txt");
#endif
			PlatformLogMessage(level, buffer);
		}
	}
}

#ifndef JM_LOG_LEVEL
#ifdef JM_DEBUG
#define JM_LOG_LEVEL JM_LOG_LEVEL_INFO
#else
#define JM_LOG_LEVEL JM_LOG_LEVEL_ERROR
#endif
#endif

#if JM_LOG_LEVEL >= JM_LOG_LEVEL_FATAL
#define JM_FATAL(...) jm::JMLog::log_message(JM_LOG_LEVEL_FATAL, true, "JM :    ", __VA_ARGS__)
#define _JM_FATAL(...) jm::JMLog::log_message(JM_LOG_LEVEL_FATAL, false, __VA_ARGS__)
#else
#define JM_FATAL(...)
#define _JM_FATAL(...)
#endif

#if JM_LOG_LEVEL >= JM_LOG_LEVEL_ERROR
#define JM_ERROR(...) jm::JMLog::log_message(JM_LOG_LEVEL_ERROR, true, "JM :    ", __VA_ARGS__);
#define _JM_ERROR(...) jm::JMLog::log_message(JM_LOG_LEVEL_ERROR, false, __VA_ARGS__)
#else
#define JM_ERROR(...)
#define _JM_ERROR(...)
#endif

#if JM_LOG_LEVEL >= JM_LOG_LEVEL_WARN
#define JM_WARN(...) jm::JMLog::log_message(JM_LOG_LEVEL_WARN, true, "JM :    ", __VA_ARGS__)
#define _JM_WARN(...) jm::JMLog::log_message(JM_LOG_LEVEL_WARN, false, __VA_ARGS__)
#else
#define JM_WARN(...)
#define _JM_WARN(...)
#endif

#if JM_LOG_LEVEL >= JM_LOG_LEVEL_INFO
#define JM_INFO(...) jm::JMLog::log_message(JM_LOG_LEVEL_INFO, true, "JM :    ", __VA_ARGS__)
#define _JM_INFO(...) jm::JMLog::log_message(JM_LOG_LEVEL_INFO, false, __VA_ARGS__)
#else
#define JM_INFO(...)
#define _JM_INFO(...)
#endif
