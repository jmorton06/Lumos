#include "lmpch.h"
#include "Core/OS/FileSystem.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#	include <Windows.h>
#	include <wtypes.h>

namespace Lumos
{

	/*void CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
	}*/

	static HANDLE OpenFileForReading(const std::string& path)
	{
		return CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	}

	static i64 GetFileSizeInternal(const HANDLE file)
	{
		LARGE_INTEGER size;
		GetFileSizeEx(file, &size);
		return size.QuadPart;
	}

	static bool ReadFileInternal(const HANDLE file, void* buffer, const i64 size)
	{
		OVERLAPPED ol = {0};
		return ReadFileEx(file, buffer, static_cast<DWORD>(size), &ol, nullptr) != 0;
	}

	bool FileSystem::FileExists(const std::string& path)
	{
		auto dwAttr = GetFileAttributes((LPCSTR)path.c_str());
		return (dwAttr != INVALID_FILE_ATTRIBUTES) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	bool FileSystem::FolderExists(const std::string& path)
	{
		DWORD dwAttrib = GetFileAttributes(path.c_str());
		return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	i64 FileSystem::GetFileSize(const std::string& path)
	{
		const HANDLE file = OpenFileForReading(path);
		if(file == INVALID_HANDLE_VALUE)
			return -1;
		i64 result = GetFileSizeInternal(file);
		CloseHandle(file);

		return result;
	}

	bool FileSystem::ReadFile(const std::string& path, void* buffer, i64 size)
	{
		const HANDLE file = OpenFileForReading(path);
		if(file == INVALID_HANDLE_VALUE)
			return false;

		if(size < 0)
			size = GetFileSizeInternal(file);

		bool result = ReadFileInternal(file, buffer, size);
		CloseHandle(file);
		return result;
	}

	u8* FileSystem::ReadFile(const std::string& path)
	{
		const HANDLE file = OpenFileForReading(path);
		const i64 size = GetFileSizeInternal(file);
		u8* buffer = lmnew u8[static_cast<u32>(size)];
		const bool result = ReadFileInternal(file, buffer, size);
		CloseHandle(file);
		if(!result)
			delete[] buffer;
		return result ? buffer : nullptr;
	}

	std::string FileSystem::ReadTextFile(const std::string& path)
	{
		const HANDLE file = OpenFileForReading(path);
		const i64 size = GetFileSizeInternal(file);
		std::string result(static_cast<u32>(size), 0);
		const bool success = ReadFileInternal(file, &result[0], size);
		CloseHandle(file);
		if(success)
		{
			// Strip carriage returns
			result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
		}
		return success ? result : std::string();
	}

	bool FileSystem::WriteFile(const std::string& path, u8* buffer)
	{
		const HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, NULL, nullptr, CREATE_NEW | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file == INVALID_HANDLE_VALUE)
			return false;

		const i64 size = GetFileSizeInternal(file);
		DWORD written;
		const bool result = ::WriteFile(file, buffer, static_cast<DWORD>(size), &written, nullptr) != 0;
		CloseHandle(file);
		return result;
	}

	bool FileSystem::WriteTextFile(const std::string& path, const std::string& text)
	{
		return WriteFile(path, (u8*)&text[0]);
	}
}

#endif