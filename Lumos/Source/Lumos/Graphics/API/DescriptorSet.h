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
			uint32_t binding;
			uint32_t location;
			Format format;
			uint32_t offset;
		};

		struct DescriptorPoolInfo
		{
			DescriptorType type;
			uint32_t size;
		};

		struct DescriptorLayoutInfo
		{
			DescriptorType type;
			ShaderType stage;
            uint32_t binding = 0;
            uint32_t setID = 0;
            uint32_t count = 1;
		};

		struct DescriptorLayout
		{
			uint32_t count;
			DescriptorLayoutInfo* layoutInfo;
		};

		struct DescriptorInfo
		{
			Pipeline* pipeline;
			uint32_t layoutIndex;
			Shader* shader;
			uint32_t count = 1;
		};

		struct BufferInfo
		{
			UniformBuffer* buffer;
			uint32_t offset;
			uint32_t size;
			int binding;
			std::string name = "";
			DescriptorType type;
			ShaderType shaderType;
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
			virtual void SetDynamicOffset(uint32_t offset) = 0;
			virtual uint32_t GetDynamicOffset() const = 0;

		protected:
			static DescriptorSet* (*CreateFunc)(const DescriptorInfo&);
		};
	}
}
