#pragma once
#include "Quad.h"
#include "QuadRenderPipeline.h"
#include "Core/ResourceDefs.h"
#include "Core/Window.h"
#include "Core/webgpu.h"
#include "GraphicsContext.h"

#include <memory>
#include <span>

namespace Gfx
{

class Renderer {
public: 
	Renderer() = default;
	~Renderer() = default;

	void Initialize(Window& window);	
	void Render();
	void UploadTextureAtlas(Gfx::TextureResource const& atlas, uint8_t index);

	GraphicsContext const& GetContext() const {return *_pCtx;}

private:

	std::unique_ptr<GraphicsContext> _pCtx = nullptr;
	
	std::unique_ptr<QuadRenderPipeline> _pQuadPipeline = nullptr;
	Quad _quad;
	std::unique_ptr<GpuBuffer> _pQuadBuffer = nullptr;


	std::unique_ptr<GpuTexture> _pAtlases = nullptr;

	std::unique_ptr<GpuTexture> _pDepthTexture = nullptr;
	wgpu::TextureView _depthTextureView = nullptr;
};

}