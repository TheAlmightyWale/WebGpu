#include "GpuBuffer.h"

namespace Gfx
{
	void GpuBuffer::EnqueueCopy(void const* pData, uint32_t size, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		assert(size <= _size);
		queue.writeBuffer(_handle, bufferOffset, pData, size);
	}

	void GpuBuffer::EnqueueCopy(void const* pData, uint32_t bufferOffset, wgpu::Queue& queue)
	{
		EnqueueCopy(pData, _size, bufferOffset, queue);
	}

	GpuBuffer::~GpuBuffer()
	{
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}

	GpuBuffer::GpuBuffer(uint32_t size, int usageFlags, std::string const& label, wgpu::Device device)
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