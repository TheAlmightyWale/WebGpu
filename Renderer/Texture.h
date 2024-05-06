#include "webgpu.h"

namespace Gfx
{
	class Texture {
	public:
		Texture(wgpu::TextureDimension dimension, wgpu::Extent3D extents, int usageFlags, wgpu::TextureFormat format, wgpu::Device device);
		~Texture();

		void EnqueueCopy(void* pData, uint32_t size, wgpu::Queue& queue, wgpu::Origin3D targetOffset = { 0, 0, 0 });

		inline wgpu::Texture Get() const { return _handle; }
		inline wgpu::Extent3D Extents() const { return _extents; }
		inline wgpu::TextureFormat Format() const { return _format; }
		inline wgpu::TextureView View() const { return _viewHandle; }

	private:
		wgpu::Texture _handle;
		wgpu::TextureView _viewHandle;
		wgpu::Extent3D _extents;
		wgpu::TextureFormat _format;
	};
}