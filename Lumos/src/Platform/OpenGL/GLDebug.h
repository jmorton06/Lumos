#pragma once
#include "lmpch.h"
#include "GLRenderer.h"
#include "GL.h"

namespace Lumos
{
#ifdef LUMOS_DEBUG
#ifdef glDebugMessageCallback1
#define GL_DEBUD_CALLBACK 1
#else
#define GL_DEBUG 1
#endif
#endif

#define OPENGLLOG "[OPENGL] - "

#ifdef GL_DEBUG
	static bool GLLogCall(const char* function, const char* file, const i32 line)
	{
		GLenum err(glGetError());
		bool Error = true;
		while (err != GL_NO_ERROR)
		{
			std::string error;

			switch (err) {
				case GL_INVALID_OPERATION:
					error = "INVALID_OPERATION";
					break;
				case GL_INVALID_ENUM:
					error = "INVALID_ENUM";
					break;
				case GL_INVALID_VALUE:
					error = "INVALID_VALUE";
					break;
				case GL_OUT_OF_MEMORY:
					error = "OUT_OF_MEMORY";
					break;
				case GL_INVALID_FRAMEBUFFER_OPERATION:
					error = "INVALID_FRAMEBUFFER_OPERATION";
					break;
				default:;
			}

			std::cerr << "GL_" << error.c_str() << " - " << file << " - " << function << ":" << line << std::endl;
			Error = false;
			err = glGetError();
		}
			return Error;
	}

	static void GLClearError()
	{
		while (glGetError() != GL_NO_ERROR);
	}
#endif

}

#if GL_DEBUG
#define GLCall(x) GLClearError();\
		x; \
		if (!GLLogCall(#x, __FILE__, __LINE__)) LUMOS_BREAK();
#else
#define GLCall(x) x
#endif


