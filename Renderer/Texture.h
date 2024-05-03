#include "webgpu.h"

namespace Gfx
{
	class Texture {
	public:
		Texture(wgpu::TextureDimension dimension, wgpu::Extent3D extents, int usageFlags, wgpu::TextureFormat format, wgpu::Device device);
		~Texture();

		inline wgpu::Texture Get() const { return _handle; }
		inline wgpu::Extent3D Extents() const { return _extents; }
		inline wgpu::TextureFormat Format() const { return _format; }

	private:
		wgpu::Texture _handle;
		wgpu::Extent3D _extents;
		wgpu::TextureFormat _format;
	};
}