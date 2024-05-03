#include <cstdint>
#include <string>
#include "webgpu.h"

namespace Gfx
{
	class Buffer
	{
	public:
		Buffer(uint32_t size, int usageFlags, std::string const& label, wgpu::Device device);
		~Buffer();

		void EnqueueCopy(void* pData, uint32_t size, uint32_t bufferOffset, wgpu::Queue& queue);
		void EnqueueCopy(void* pData, uint32_t bufferOffset, wgpu::Queue& queue);

		inline wgpu::Buffer const& Get() const { return _handle; }

	private:
		wgpu::Buffer _handle;
		uint32_t _size;
	};
}

