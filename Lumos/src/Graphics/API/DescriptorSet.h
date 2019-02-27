#pragma once
#include "UniformBuffer.h"

#include "Textures/Texture.h"

namespace Lumos
{
    class Shader;
	enum ShaderType : int;

	namespace graphics
	{
		namespace api
		{
			class Pipeline;

			enum class DescriptorType
			{
				UNIFORM_BUFFER,
				UNIFORM_BUFFER_DYNAMIC,
				IMAGE_SAMPLER
			};

			enum class ShaderStage
			{
				VERTEX,
				FRAGMENT,
				GEOMETRY
			};

			enum Format
			{
				R32G32B32A32_FLOAT,
				R32G32B32_FLOAT,
				R32G32_FLOAT,
                R32_FLOAT
			};

			struct VertexInputDescription
			{
				uint binding;
				uint location;
				Format format;
				uint offset;
			};

			struct DescriptorPoolInfo
			{
				DescriptorType type;
				uint size;
			};

			struct DescriptorLayoutInfo
			{
				DescriptorType type;
				ShaderStage stage;
				uint size;
			};

			struct DescriptorLayout
			{
				uint count;
				api::DescriptorLayoutInfo* layoutInfo;
			};

			struct DescriptorInfo
			{
				Pipeline* pipeline;
				uint layoutIndex;
                Shader* shader;
			};

			struct BufferInfo
			{
				UniformBuffer* buffer;
				uint offset;
				uint size;
				int binding;
				String name = "";
				DescriptorType type;
				ShaderType shaderType;
				bool systemUniforms = false;
			};

			enum class PushConstantDataType
			{
				UINT,
				FLOAT,
				VEC4
			};

			struct PushConstant
			{
				uint size;
				PushConstantDataType type;
				byte* data;
			};

			struct ImageInfo
			{
				Texture* texture;
				int binding;
				String name;
				TextureType type = TextureType::COLOUR;
			};

			class DescriptorSet
			{
			public:
				virtual ~DescriptorSet() = default;
				static DescriptorSet* Create(DescriptorInfo info);

				virtual void Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos) = 0;
				virtual void Update(std::vector<ImageInfo>& imageInfos) = 0;
				virtual void Update(std::vector<BufferInfo>& bufferInfos) = 0;
				virtual void SetPushConstants(std::vector<PushConstant>& pushConstants) = 0;

				void SetDynamicOffset(uint offset) { m_DynamicOffset = offset; }
				uint GetDynamicOffset() const { return m_DynamicOffset; }

			protected:
				uint m_DynamicOffset = 0;
                Shader* m_Shader = nullptr;


			};
		}
	}
}
