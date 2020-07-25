#include "lmpch.h"
#include "Shader.h"

#include "Graphics/API/GraphicsContext.h"
#include "Core/VFS.h"

namespace Lumos
{
	namespace Graphics
	{
		Shader* (*Shader::CreateFunc)(const std::string&, const std::string&) = nullptr;

		const Shader* Shader::s_CurrentlyBound = nullptr;

		Shader* Shader::CreateFromFile(const std::string& name, const std::string& filepath)
		{
			std::string filePath;
#ifdef LUMOS_PLATFORM_MOBILE
			filePath = name;
#else
			filePath = filepath;
#endif

			LUMOS_ASSERT(CreateFunc, "No Shader Create Function");

			return CreateFunc(name, filepath);
		}

		bool Shader::TryCompile(const std::string& source, std::string& error, const std::string& name)
		{
			LUMOS_ASSERT(CreateFunc, "No Shader TryCompile Function");

			return false; // CreateFunc(framebufferInfo);
		}

		bool Shader::TryCompileFromFile(const std::string& filepath, std::string& error)
		{
			std::string source = Lumos::VFS::Get()->ReadTextFile(filepath);
			return TryCompile(source, error, filepath);
		}
	}
}
