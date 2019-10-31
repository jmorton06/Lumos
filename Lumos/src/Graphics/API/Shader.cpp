#include "lmpch.h"
#include "Shader.h"

#include "Graphics/API/GraphicsContext.h"
#include "Core/VFS.h"

namespace Lumos
{
	namespace Graphics
	{
        Shader*(*Shader::CreateFunc)(const String&, const String&) = nullptr;

		const Shader* Shader::s_CurrentlyBound = nullptr;

		Shader* Shader::CreateFromFile(const String& name, const String& filepath)
		{
			String filePath;
#ifdef LUMOS_PLATFORM_MOBILE
			filePath = name;
#else
			filePath = filepath;
#endif

            LUMOS_ASSERT(CreateFunc, "No Shader Create Function");
            
            return CreateFunc(name,filepath);
		}

		bool Shader::TryCompile(const String& source, String& error, const String& name)
		{
            LUMOS_ASSERT(CreateFunc, "No Shader TryCompile Function");
            
            return false;// CreateFunc(framebufferInfo);
		}

		bool Shader::TryCompileFromFile(const String& filepath, String& error)
		{
			String source = Lumos::VFS::Get()->ReadTextFile(filepath);
			return TryCompile(source, error, filepath);
		}
	}
}
