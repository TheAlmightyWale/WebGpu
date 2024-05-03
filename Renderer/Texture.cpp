#include "Texture.h"

namespace Gfx
{
	Texture::Texture(wgpu::TextureDimension dimension, wgpu::Extent3D extents, int usageFlags, wgpu::TextureFormat format, wgpu::Device device)
		: _handle(nullptr)
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
	}

	Texture::~Texture() {
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}
}