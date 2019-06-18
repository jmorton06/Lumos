#pragma once
#include "LM.h"

namespace Lumos
{
	namespace Graphics
	{
		class Pipeline;
		class Shader;
		class Texture;
		class UniformBuffer;
		enum class TextureType : int;
		enum class ShaderType : int;

		enum class DescriptorType
		{
			UNIFORM_BUFFER,
			UNIFORM_BUFFER_DYNAMIC,
			IMAGE_SAMPLER
		};

		enum class Format
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
			ShaderType stage;
			uint size;
			uint count = 1;
		};

		struct DescriptorLayout
		{
			uint count;
			DescriptorLayoutInfo* layoutInfo;
		};

		struct DescriptorInfo
		{
			Pipeline* pipeline;
			uint layoutIndex;
            Shader* shader;
			uint count = 1;
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
            ShaderType shaderStage;
		};

		struct ImageInfo
		{
            std::vector<Texture*> texture;
			int count = 1;
			int binding;
			String name;
			TextureType type;
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
			virtual void SetDynamicOffset(uint offset) = 0;
			virtual uint GetDynamicOffset() const = 0;
		};
	}
}
