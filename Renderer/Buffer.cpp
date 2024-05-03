#include "Buffer.h"

namespace Gfx
{
	void Buffer::EnqueueCopy(void* pData, uint32_t size, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		assert(size <= _size);
		queue.writeBuffer(_handle, bufferOffset, pData, size);
	}

	void Buffer::EnqueueCopy(void* pData, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		queue.writeBuffer(_handle, bufferOffset, pData, _size);
	}

	Buffer::~Buffer()
	{
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}

	Buffer::Buffer(uint32_t size, int usageFlags, std::string const& label, wgpu::Device device)
		: _size(size)
		, _handle(nullptr)
	{
		wgpu::BufferDescriptor desc;
		desc.label = label.c_str();
		desc.mappedAtCreation = false;
		desc.nextInChain = nullptr;
		desc.size = size;
		desc.usage = usageFlags;

		_handle = device.createBuffer(desc);
	}

}