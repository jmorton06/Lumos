#include "LM.h"
#include "Shader.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLShader.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXShader.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKShader.h"
#endif

#include "Graphics/API/Context.h"

namespace Lumos
{

	const Shader* Shader::s_CurrentlyBound = nullptr;

	Shader* Shader::CreateFromFile(const String& name, const String& filepath, void* address)
	{
        String filePath;
#ifdef LUMOS_PLATFORM_MOBILE
        filePath = name;
#else
        filePath = filepath;
#endif

		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:
		{
			const String source = Lumos::VFS::Get()->ReadTextFile(filePath + name + ".glsl");
			GLShader* result = address ? new(address)GLShader(name, source) : new GLShader(name, source);
			result->m_Path = filePath;
			return result;
		}
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:
		{
			std::string physicalPath;
			Lumos::VFS::Get()->ResolvePhysicalPath(filepath, physicalPath);
			graphics::VKShader* result = new graphics::VKShader(name, physicalPath);
			return result;
		}
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:
		{
			const String source = Lumos::VFS::Get()->ReadTextFile(filepath + ".hlsl");
			D3DShader* result = address ? new(address)D3DShader(name, source) : new D3DShader(name, source);
			result->m_FilePath = filepath;
			return result;
		}
#endif
		}
		return nullptr;
	}

	bool Shader::TryCompile(const String& source, String& error, const String& name)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return GLShader::TryCompile(source, error);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return D3DShader::TryCompile(source, error);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
        case RenderAPI::VULKAN:     return false; //VKShader::TryCompile(source, error);
#endif
		}
		return false;
	}

	bool Shader::TryCompileFromFile(const String& filepath, String& error)
	{
		String source = Lumos::VFS::Get()->ReadTextFile(filepath);
		return TryCompile(source, error, filepath);
	}
}
