#include "Buffer.h"

namespace Gfx
{
	void Buffer::EnqueueCopy(void const* pData, uint32_t size, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		assert(size <= _size);
		queue.writeBuffer(_handle, bufferOffset, pData, size);
	}

	void Buffer::EnqueueCopy(void const* pData, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		EnqueueCopy(pData, _size, bufferOffset, queue);
	}

	Buffer::~Buffer()
	{
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}

	Buffer::Buffer(uint32_t size, int usageFlags, std::string const& label, wgpu::Device device)
		: _handle(nullptr)
		, _size(size)
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