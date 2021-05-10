#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class ShaderResourceDeclaration
        {
        public:
            virtual ~ShaderResourceDeclaration() = default;
            virtual const std::string& GetName() const = 0;
            virtual uint32_t GetRegister() const = 0;
            virtual uint32_t GetCount() const = 0;
        };

        typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;
    }
}
