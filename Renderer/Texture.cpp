#include "Texture.h"

namespace
{
	wgpu::TextureViewDimension Convert(wgpu::TextureDimension dimension, wgpu::Extent3D extents)
	{
		switch (dimension)
		{
		case wgpu::TextureDimension::_1D: return wgpu::TextureViewDimension::_1D;
		case wgpu::TextureDimension::_2D: {
			if (extents.depthOrArrayLayers == 1) return wgpu::TextureViewDimension::_2D;
			else return wgpu::TextureViewDimension::_2DArray;
		}
		case wgpu::TextureDimension::_3D: return wgpu::TextureViewDimension::_3D;
		}

		assert("Invalid dimension or extents given");
		return wgpu::TextureViewDimension::_1D;
	}
}


namespace Gfx
{
	Texture::Texture(wgpu::TextureDimension dimension, wgpu::Extent3D extents, int usageFlags,
		uint8_t numChannels, uint8_t bytesPerChannel, wgpu::TextureFormat format, wgpu::Device device, std::string const& label)
		: _handle(nullptr)
		, _viewHandle(nullptr)
		, _extents(extents)
		, _format(format)
		, _numChannels(numChannels)
		, _bytesPerChannel(bytesPerChannel)
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
		desc.label = label.c_str();
		_handle = device.createTexture(desc);

		wgpu::TextureViewDescriptor vDesc;
		vDesc.aspect = wgpu::TextureAspect::All;
		vDesc.baseArrayLayer = 0;
		if (dimension == wgpu::TextureDimension::_2D) vDesc.arrayLayerCount = extents.depthOrArrayLayers;
		else vDesc.arrayLayerCount = 1;
		vDesc.baseMipLevel = 0;
		vDesc.mipLevelCount = 1;
		vDesc.dimension = Convert(dimension, extents);
		vDesc.format = format;
		_viewHandle = _handle.createView(vDesc);
	}

	Texture::~Texture() {
		if (_handle) {
			_handle.destroy();
			_handle.release();
		}
	}
	void Texture::EnqueueCopy(void* pData, wgpu::Extent3D writeSize, wgpu::Queue& queue, wgpu::Origin3D targetOffset)
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
		
		uint32_t sizeBytes = writeSize.width * writeSize.height * writeSize.depthOrArrayLayers * _bytesPerChannel * _numChannels;
		queue.writeTexture(destination, pData, sizeBytes , source, writeSize);
	}

}