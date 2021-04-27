#include "Precompiled.h"
#include "Shader.h"

#include "Graphics/API/GraphicsContext.h"
#include "Core/VFS.h"

namespace Lumos
{
    namespace Graphics
    {
        Shader* (*Shader::CreateFunc)(const std::string&) = nullptr;

        const Shader* Shader::s_CurrentlyBound = nullptr;

        Shader* Shader::CreateFromFile(const std::string& filepath)
        {
            LUMOS_ASSERT(CreateFunc, "No Shader Create Function");
            return CreateFunc(filepath);
        }
    }
}
