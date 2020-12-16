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
            R32G32B32A32_UINT,
            R32G32B32_UINT,
            R32G32_UINT,
            R32_UINT,
            R8_UINT,
            R32G32B32A32_INT,
            R32G32B32_INT,
            R32G32_INT,
            R32_INT,
			R32G32B32A32_FLOAT,
			R32G32B32_FLOAT,
			R32G32_FLOAT,
			R32_FLOAT
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
            u32 binding = 0;
            u32 setID = 0;
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
			ShaderType shaderStage;
            u8* data;
            u32 offset = 0;
        };

		struct ImageInfo
		{
			Texture** textures;
            Texture* texture;

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
