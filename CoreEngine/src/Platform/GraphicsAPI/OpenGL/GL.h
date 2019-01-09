#pragma once
#include "JM.h"
#ifdef JM_PLATFORM_MOBILE
#ifdef JM_PLATFORM_IOS
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif JM_PLATFORM_ANDROID
#include <GLES3/gl3.h>
#endif

#else
#ifdef JM_PLATFORM_WINDOWS
#include <glad/glad.h>
#elif JM_PLATFORM_LINUX
#include <glad/glad.h>
#elif JM_PLATFORM_MACOS
#include <glad/glad.h>
#include <OpenGL/gl.h>
#endif

#endif


