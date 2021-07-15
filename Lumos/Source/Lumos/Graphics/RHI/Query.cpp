#include "Precompiled.h"
#include "Query.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLQuery.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/directx/DXQuery.h"
#endif

#include "Graphics/RHI/GraphicsContext.h"

namespace Lumos
{
    namespace Graphics
    {
        Query* (*Query::CreateFunc)(QueryType) = nullptr;

        Query* Query::Create(QueryType type)
        {
            LUMOS_ASSERT(CreateFunc, "No Query Create Function");

            return CreateFunc(type);
        }
    }
}
