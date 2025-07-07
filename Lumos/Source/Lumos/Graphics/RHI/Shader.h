#pragma once
#include "RHIDefinitions.h"
#include "Core/Profiler.h"
#include "Core/Asset/Asset.h"

namespace spirv_cross
{
    struct SPIRType;
}
namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT Shader : public Asset
        {
        public:
            static const Shader* s_CurrentlyBound;

        public:
            virtual void Bind() const   = 0;
            virtual void Unbind() const = 0;

            virtual ~Shader() = default;

            virtual const TDArray<ShaderType> GetShaderTypes() const = 0;
            virtual const char* GetName() const               = 0;
            virtual const char* GetFilePath() const           = 0;

            virtual void* GetHandle() const = 0;
            virtual bool IsCompiled() const { return true; }

            virtual TDArray<PushConstant>& GetPushConstants() = 0;
            virtual PushConstant* GetPushConstant(uint32_t index) { return nullptr; }
            virtual void BindPushConstants(Graphics::CommandBuffer* commandBuffer, Graphics::Pipeline* pipeline) = 0;
            virtual DescriptorSetInfo GetDescriptorInfo(uint32_t index) { return DescriptorSetInfo(); }
            virtual uint64_t GetHash() const { return 0; };

            ShaderDataType SPIRVTypeToLumosDataType(const spirv_cross::SPIRType type);

            SET_ASSET_TYPE(AssetType::Shader);

        public:
            static Shader* CreateFromFile(const char* filepath);
            static Shader* CreateFromEmbeddedArray(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize);
            static Shader* CreateCompFromEmbeddedArray(const uint32_t* compData, uint32_t compDataSize);

        protected:
            static Shader* (*CreateFunc)(const char*);
            static Shader* (*CreateFuncFromEmbedded)(const uint32_t*, uint32_t, const uint32_t*, uint32_t);
            static Shader* (*CreateCompFuncFromEmbedded)(const uint32_t*, uint32_t);
        };
    }
}
