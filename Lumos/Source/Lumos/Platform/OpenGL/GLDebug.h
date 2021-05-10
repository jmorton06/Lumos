#pragma once

#include "GLRenderer.h"
#include "GL.h"

namespace Lumos
{
#ifdef LUMOS_DEBUG
#ifdef glDebugMessageCallback
#define GL_DEBUD_CALLBACK 1
#else
#define GL_DEBUG 1
#endif
#endif

#define OPENGLLOG "[OPENGL] - "

#ifdef GL_DEBUG
    bool GLLogCall(const char* function, const char* file, const int32_t line);
    void GLClearError();
#endif

}

#if GL_DEBUG
#define GLCall(x)                          \
    GLClearError();                        \
    x;                                     \
    if(!GLLogCall(#x, __FILE__, __LINE__)) \
        LUMOS_BREAK();
#else
#define GLCall(x) x
#endif
