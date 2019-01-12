#include "JM.h"
#include "GLContext.h"

#include "GLVertexArray.h"
#include "GL.h"
#include "GLDebug.h"
#ifndef JM_PLATFORM_MOBILE
#include "GLFW/glfw3.h"
#endif

#ifdef GL_DEBUD_CALLBACK
static std::string GetStringForType(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		return"Error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "Deprecated behavior";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "Undefined behavior";
	case GL_DEBUG_TYPE_PORTABILITY:
		return "Portability issue";
	case GL_DEBUG_TYPE_PERFORMANCE:
		return "Performance issue";
	case GL_DEBUG_TYPE_MARKER:
		return "Stream annotation";
	case GL_DEBUG_TYPE_OTHER_ARB:
		return "Other";
	default:
		return "";
	}
}

static bool PrintMessage(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		return true;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return true;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return true;
	case GL_DEBUG_TYPE_PORTABILITY:
		return true;
	case GL_DEBUG_TYPE_PERFORMANCE:
		return false;
	case GL_DEBUG_TYPE_MARKER:
		return false;
	case GL_DEBUG_TYPE_OTHER_ARB:
		return false;
	default:
		return false;
	}
}

static std::string GetStringForSource(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return "Window system";
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return "Shader compiler";
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return "Third party";
	case GL_DEBUG_SOURCE_APPLICATION:
		return "Application";
	case GL_DEBUG_SOURCE_OTHER:
		return "Other";
	default:
		return "";
	}
}

static std::string GetStringForSeverity(GLenum severity)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		JM_CORE_ASSERT(0,"");
		return "High";
	case GL_DEBUG_SEVERITY_MEDIUM:
		JM_CORE_ASSERT(0,"");
		return "Medium";
	case GL_DEBUG_SEVERITY_LOW:
		JM_CORE_ASSERT(0,"");
		return "Low";
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return "Notification";
	default:
		return("");
	}

}
void APIENTRY openglCallbackFunction(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) 
{
	if(!PrintMessage(type))
		return;

	JM_CORE_INFO(OPENGLLOG"Message: {0}" , message);
	JM_CORE_INFO(OPENGLLOG"Type: {0}"	   , GetStringForType(type));
	JM_CORE_INFO(OPENGLLOG"Source: {0}"  , GetStringForSource(source));
	JM_CORE_INFO(OPENGLLOG"ID: {0}"      , id);
	JM_CORE_INFO(OPENGLLOG"Severity: {0}", GetStringForSeverity(source));
}

#endif

namespace jm
{
	namespace graphics
	{
		GLContext::GLContext(WindowProperties properties, void* deviceContext)
		{
#ifndef JM_PLATFORM_MOBILE
			if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
			{
				JM_CORE_ERROR("Failed to initialize OpenGL context");
			}
#endif
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			JM_CORE_INFO("----------------------------------");
			JM_CORE_INFO(OPENGLLOG);
			JM_CORE_INFO(glGetString(GL_VERSION));
			JM_CORE_INFO(glGetString(GL_VENDOR));
			JM_CORE_INFO(glGetString(GL_RENDERER));
			JM_CORE_INFO("----------------------------------");

#if JM_DEBUG
#ifdef GL_DEBUD_CALLBACK
				JM_CORE_INFO(OPENGLLOG"Registering OpenGL debug callback");

				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(openglCallbackFunction, nullptr);
				GLuint unusedIds = 0;
				glDebugMessageControl(GL_DONT_CARE,
					GL_DONT_CARE,
					GL_DONT_CARE,
					0,
					&unusedIds,
					true);
#else
			JM_CORE_INFO(OPENGLLOG"glDebugMessageCallback not available");
#endif
#endif
		}

		GLContext::~GLContext() = default;

		void GLContext::Present()
		{
		}
	}
}
