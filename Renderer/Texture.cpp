#include "Texture.h"

namespace
{
	wgpu::TextureViewDimension Convert(wgpu::TextureDimension dimension)
	{
		switch (dimension)
		{
		case wgpu::TextureDimension::_1D: return wgpu::TextureViewDimension::_1D;
		case wgpu::TextureDimension::_2D: return wgpu::TextureViewDimension::_2D;
		case wgpu::TextureDimension::_3D: return wgpu::TextureViewDimension::_3D;
		}

		assert("Invalid dimension given");
		return wgpu::TextureViewDimension::_1D;
	}
}


namespace Gfx
{
	Texture::Texture(wgpu::TextureDimension dimension, wgpu::Extent3D extents, int usageFlags, wgpu::TextureFormat format, wgpu::Device device)
		: _handle(nullptr)
		, _viewHandle(nullptr)
		, _extents(extents)
		, _format(format)
	{
		wgpu::TextureDescriptor desc;
		desc.dimension = dimension;
		desc.size = extents;
		desc.mipLevelCount = 1;
		desc.sampleCount = 1;
		desc.format = format;
		desc.usage = usageFlags;
		desc.viewFormatCount = 1;
		desc.viewFormats = (WGPUTextureFormat*)&_format;
		_handle = device.createTexture(desc);

		wgpu::TextureViewDescriptor vDesc;
		vDesc.aspect = wgpu::TextureAspect::All;
		vDesc.baseArrayLayer = 0;
		vDesc.arrayLayerCount = 1;
		vDesc.baseMipLevel = 0;
		vDesc.mipLevelCount = 1;
		vDesc.dimension = Convert(dimension);
		vDesc.format = format;
		_viewHandle = _handle.createView(vDesc);
	}

	Texture::~Texture() {
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}
	void Texture::EnqueueCopy(void* pData, uint32_t size, wgpu::Queue& queue, wgpu::Origin3D targetOffset)
	{
		wgpu::ImageCopyTexture destination;
		destination.texture = _handle;
		destination.mipLevel = 0;
		destination.origin = targetOffset;
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::TextureDataLayout source;
		source.offset = 0;
		source.bytesPerRow = 4 /*4 byte channels, rgba*/ * _extents.width;
		source.rowsPerImage = _extents.height;

		queue.writeTexture(destination, pData, size, source, _extents);
	}

}