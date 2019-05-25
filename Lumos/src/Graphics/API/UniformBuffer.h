#pragma once

namespace lumos
{
	namespace graphics
	{
		class UniformBuffer
		{
		public:
			virtual ~UniformBuffer() = default;
			static UniformBuffer* Create();
			static UniformBuffer* Create(uint32_t size, const void* data);

			virtual void Init(uint32_t size, const void* data) = 0;
			virtual void SetData(uint32_t size, const void* data) = 0;
			virtual void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) = 0;

			virtual byte* GetBuffer() const = 0;

		};
	}
}
