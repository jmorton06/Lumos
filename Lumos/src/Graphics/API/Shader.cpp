#include "LM.h"
#include "Shader.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLShader.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXShader.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKShader.h"
#endif

#include "Graphics/API/GraphicsContext.h"

namespace lumos
{
	namespace graphics
	{
		const Shader* Shader::s_CurrentlyBound = nullptr;

		Shader* Shader::CreateFromFile(const String& name, const String& filepath)
		{
			String filePath;
#ifdef LUMOS_PLATFORM_MOBILE
			filePath = name;
#else
			filePath = filepath;
#endif

			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
			{
				const String source = lumos::VFS::Get()->ReadTextFile(filePath + name + ".glsl");
				GLShader* result = new GLShader(name, source);
				result->m_Path = filePath;
				return result;
			}
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
			{
				std::string physicalPath;
				lumos::VFS::Get()->ResolvePhysicalPath(filepath, physicalPath);
				graphics::VKShader* result = new graphics::VKShader(name, physicalPath);
				return result;
			}
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
			{
				const String source = lumos::VFS::Get()->ReadTextFile(filepath + ".hlsl");
				D3DShader* result = new D3DShader(name, source);
				result->m_FilePath = filepath;
				return result;
			}
#endif
			}
			return nullptr;
		}

		bool Shader::TryCompile(const String& source, String& error, const String& name)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
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
			String source = lumos::VFS::Get()->ReadTextFile(filepath);
			return TryCompile(source, error, filepath);
		}
	}
}
