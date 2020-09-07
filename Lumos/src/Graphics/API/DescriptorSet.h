#pragma once

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
			R32_FLOAT,
			R32_UINT,
			R8_UINT
		};

		struct VertexInputDescription
		{
			u32 binding;
			u32 location;
			Format format;
			u32 offset;
		};

		struct DescriptorPoolInfo
		{
			DescriptorType type;
			u32 size;
		};

		struct DescriptorLayoutInfo
		{
			DescriptorType type;
			ShaderType stage;
			u32 size;
			u32 count = 1;
		};

		struct DescriptorLayout
		{
			u32 count;
			DescriptorLayoutInfo* layoutInfo;
		};

		struct DescriptorInfo
		{
			Pipeline* pipeline;
			u32 layoutIndex;
			Shader* shader;
			u32 count = 1;
		};

		struct BufferInfo
		{
			UniformBuffer* buffer;
			u32 offset;
			u32 size;
			int binding;
			std::string name = "";
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
			u32 size;
			PushConstantDataType type;
			u8* data;
			ShaderType shaderStage;
		};

		struct ImageInfo
		{
			std::vector<Texture*> texture;
			int count = 1;
			int binding;
			std::string name;
			TextureType type;
		};

		class DescriptorSet
		{
		public:
			virtual ~DescriptorSet() = default;
			static DescriptorSet* Create(const DescriptorInfo& info);

			virtual void Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos) = 0;
			virtual void Update(std::vector<ImageInfo>& imageInfos) = 0;
			virtual void Update(std::vector<BufferInfo>& bufferInfos) = 0;
			virtual void SetPushConstants(std::vector<PushConstant>& pushConstants) = 0;
			virtual void SetDynamicOffset(u32 offset) = 0;
			virtual u32 GetDynamicOffset() const = 0;

		protected:
			static DescriptorSet* (*CreateFunc)(const DescriptorInfo&);
		};
	}
}
