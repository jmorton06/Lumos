#include "Precompiled.h"
#include "GLDebug.h"

#ifdef GL_DEBUG

bool Lumos::GLLogCall(const char* function, const char* file, const int32_t line)
{
    GLenum err(glGetError());
    bool Error = true;
    while(err != GL_NO_ERROR)
    {
        std::string error;

        switch(err)
        {
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

void Lumos::GLClearError()
{
    while(glGetError() != GL_NO_ERROR)
        ;
}

#endif
