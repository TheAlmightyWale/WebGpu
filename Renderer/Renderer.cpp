#include "Renderer.h"

#include "CameraDefs.h"
#include "Core/ResourceDefs.h"
#include "GpuTexture.h"
#include "ShaderLoader.h"
#include "QuadDefs.h"
#include "Quad.h"
#include "CpuBuffer.hpp"
#include "GlobalBuffers.h"

#include <iostream>
#include <filesystem>
#include <span>

namespace Gfx
{
	uint32_t CeilToNextMultiple(uint32_t value, uint32_t multiple)
	{
		uint32_t divideAndCeil = value / multiple + (value % multiple == 0 ? 0 : 1);
		return multiple * divideAndCeil;
	}

    void Renderer::Initialize(Window& window)
    {
        _pCtx = std::make_unique<GraphicsContext>(window);
		wgpu::Device device = _pCtx->GetDevice();
		wgpu::Instance instance = _pCtx->GetInstance();
		wgpu::Queue queue = _pCtx->GetQueue();
		wgpu::Surface surface = _pCtx->GetSurface();
		wgpu::Adapter adapter = _pCtx->GetAdapter();

        // disabled for now due to causing crashes on surface.configure
		// auto onQueueWorkDone = [](wgpu::QueueWorkDoneStatus status) {
		//	std::cout << "Queued work completed with status: " << status << "\n";
		// };
		// queue.onSubmittedWorkDone(onQueueWorkDone);

        wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
		if (swapChainFormat == wgpu::TextureFormat::Undefined)
			swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

		// Swapchain is configured through surface
		wgpu::SurfaceConfiguration surfaceConfig = wgpu::Default;
		surfaceConfig.nextInChain = nullptr;
		surfaceConfig.width = Gfx::k_screenWidth;
		surfaceConfig.height = Gfx::k_screenHeight;
		surfaceConfig.format = swapChainFormat;
		surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
		surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
		surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
		surfaceConfig.viewFormatCount = 0;
		surfaceConfig.viewFormats = nullptr;
		surfaceConfig.device = device;
		surface.configure(surfaceConfig);

		std::cout << "Configured Surface\n";

        wgpu::BlendState blendState{};
		blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
		blendState.color.dstFactor = wgpu::BlendFactor::OneMinusDstAlpha;
		blendState.color.operation = wgpu::BlendOperation::Add;
		blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
		blendState.alpha.dstFactor = wgpu::BlendFactor::One;
		blendState.alpha.operation = wgpu::BlendOperation::Add;

        wgpu::ColorTargetState colorTarget{};
		colorTarget.format = swapChainFormat;
		colorTarget.blend = &blendState;
		colorTarget.writeMask = wgpu::ColorWriteMask::All;

        std::filesystem::path const assetsBasePath(ASSETS_DIR);
        auto oQuadShaderModule = Gfx::LoadShaderModule(assetsBasePath / "quadShader.wgsl", device);
		if (!oQuadShaderModule)
		{
			std::cout << "Failed to create Quad Shader Module" << std::endl;
			return;
		}
		wgpu::ShaderModule quadShaderModule = *oQuadShaderModule;
    
        wgpu::DepthStencilState depthStencilState = wgpu::Default;
		depthStencilState.depthCompare = wgpu::CompareFunction::Less;
		depthStencilState.depthWriteEnabled = true;
		wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
		depthStencilState.format = depthTextureFormat;
		// No stencil ability
		depthStencilState.stencilReadMask = 0;
		depthStencilState.stencilWriteMask = 0;

        _pQuadPipeline = std::make_unique<QuadRenderPipeline>
            (device, quadShaderModule, colorTarget, depthStencilState);

		//Upload data for quad
		_pQuadBuffer = std::make_unique<GpuBuffer>(
			(uint32_t)(sizeof(Quad)), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex, "Quad Vertices", device);
		_pQuadBuffer->EnqueueCopy(_quad.vertices.data(), 0, queue);

        // Create depth texture and depth texture view
		_pDepthTexture = std::make_unique<GpuTexture>(
			wgpu::TextureDimension::_2D,
			wgpu::Extent3D{surfaceConfig.width, surfaceConfig.height, 1},
			wgpu::TextureUsage::RenderAttachment,
			1,
			3 /*24 bit depth*/,
			depthTextureFormat,
			device,
			"depth");

		wgpu::TextureViewDescriptor depthTextureViewDesc;
		depthTextureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
		depthTextureViewDesc.baseArrayLayer = 0;
		depthTextureViewDesc.arrayLayerCount = 1;
		depthTextureViewDesc.baseMipLevel = 0;
		depthTextureViewDesc.mipLevelCount = 1;
		depthTextureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
		depthTextureViewDesc.format = depthTextureFormat;
		_depthTextureView = _pDepthTexture->Get().createView(depthTextureViewDesc);

        // Temp buffer data, everything used here is dummy data to show an example of copying data between buffers on GPU
		uint32_t k_bufferSize = 16;
		std::vector<uint8_t> numbers(k_bufferSize);
		for (uint8_t i = 0; i < k_bufferSize; ++i)
			numbers[i] = i;

		// Create Buffer
		Gfx::GpuBuffer buffer1{k_bufferSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc, "buffer1", device};
		Gfx::GpuBuffer buffer2{k_bufferSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, "buffer2", device};

		// Add instruction to copy to buffer
		buffer1.EnqueueCopy(numbers.data(), 0, queue);

		std::cout << "Sending buffer copy operation... " << std::endl;

		// Copy buffer1 to buffer2
		wgpu::CommandEncoderDescriptor encoderDesc1{};
		encoderDesc1.label = "Default Command Encoder";
		wgpu::CommandEncoder copyEncoder = device.createCommandEncoder(encoderDesc1);
		copyEncoder.copyBufferToBuffer(buffer1.Get(), 0, buffer2.Get(), 0, k_bufferSize);

		wgpu::CommandBufferDescriptor commandBufferDescriptor1{};
		commandBufferDescriptor1.label = "Default Command Buffer";
		wgpu::CommandBuffer copyCommands = copyEncoder.finish(commandBufferDescriptor1);
		queue.submit(copyCommands);

		copyCommands.release();

        struct BufferMappedContext
		{
			wgpu::Buffer buffer;
			uint32_t bufferSize;
		};

		auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void *pUserData)
		{
			BufferMappedContext *pContext = reinterpret_cast<BufferMappedContext *>(pUserData);
			std::cout << "Buffer 2 Mapped with status: " << status << "\n";

			if (status != wgpu::BufferMapAsyncStatus::Success || pContext == nullptr)
				return;
			// uint8_t* pBufferData = (uint8_t*)pContext->buffer.getConstMappedRange(0, pContext->bufferSize);

			// Once we are done with this data, unmap the buffer
			pContext->buffer.unmap();
		};
		wgpuBufferMapAsync(buffer2.Get(), wgpu::MapMode::Read, 0, k_bufferSize, onBuffer2Mapped, nullptr);
    }

    void Renderer::Render()
    {
		glfwPollEvents();

		wgpu::Surface surface = _pCtx->GetSurface();

		wgpu::SurfaceTexture surfaceTexture;
		surface.getCurrentTexture(&surfaceTexture);
		if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
		{
			std::cerr << "Failed to get surfaceTexture, status code: " << surfaceTexture.status << "\n";
			return;
		}

		wgpu::TextureViewDescriptor surfaceViewDesc = wgpu::Default;
		surfaceViewDesc.nextInChain = nullptr;
		surfaceViewDesc.label = "Display Surface Texture View";
		surfaceViewDesc.format = wgpuTextureGetFormat(surfaceTexture.texture);
		surfaceViewDesc.dimension = wgpu::TextureViewDimension::_2D;
		surfaceViewDesc.baseMipLevel = 0;
		surfaceViewDesc.mipLevelCount = 1;
		surfaceViewDesc.baseArrayLayer = 0;
		surfaceViewDesc.arrayLayerCount = 1;
		surfaceViewDesc.aspect = wgpu::TextureAspect::All;

		wgpu::TextureView toDisplay = wgpuTextureCreateView(surfaceTexture.texture, &surfaceViewDesc);
		if (!toDisplay)
		{
			std::cerr << "Failed to acquire next swap chain texture\n";
			return;
		}

		wgpu::Device device = _pCtx->GetDevice();

		wgpu::CommandEncoderDescriptor encoderDesc{};
		encoderDesc.label = "Default Command Encoder";
		wgpu::CommandEncoder encoder = device.createCommandEncoder(encoderDesc);

		wgpu::Queue queue = _pCtx->GetQueue();

		//Upload various global buffers to GPU

		//Camera
		Gfx::GpuBuffer camBuffer{sizeof(Gfx::CamUniform), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, "Camera Uniforms", device};
		g_cameraUniformBuffer.CopyTo(camBuffer, queue);

		//Quad positions
		Gfx::GpuBuffer transformBuffer{g_transformBuffer.SizeBytes(),
									wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
									"Transform Buffer", device};
		g_transformBuffer.CopyTo(transformBuffer, queue);

		//Animation states
		Gfx::GpuBuffer animationBuffer{g_animationBuffer.SizeBytes(),
									 wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
									"Animations", device};
		g_animationBuffer.CopyTo(animationBuffer, queue);

        // then draws it

		//bind data
		_pQuadPipeline->BindData(transformBuffer, *_pAtlases, camBuffer, animationBuffer, device);

		//construct render pass
		wgpu::RenderPassColorAttachment rpColorAttachment{};
		rpColorAttachment.view = toDisplay;
		rpColorAttachment.resolveTarget = nullptr;
		rpColorAttachment.loadOp = wgpu::LoadOp::Clear;
		rpColorAttachment.storeOp = wgpu::StoreOp::Store;
		rpColorAttachment.clearValue = wgpu::Color{0.9, 0.1, 0.2, 1.0};

		wgpu::RenderPassDepthStencilAttachment rpDepthAttachment;
		rpDepthAttachment.view = _depthTextureView;
		rpDepthAttachment.depthClearValue = 1.0f; // Maximum distance possible
		rpDepthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
		rpDepthAttachment.depthStoreOp = wgpu::StoreOp::Store;
		rpDepthAttachment.depthReadOnly = false;
		// current unused stencil params, but need to be filled out
		rpDepthAttachment.stencilClearValue = 0;
		rpDepthAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
		rpDepthAttachment.stencilStoreOp = wgpu::StoreOp::Store;
		rpDepthAttachment.stencilReadOnly = true;

		wgpu::RenderPassDescriptor renderPassDesc{};
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &rpColorAttachment;
		renderPassDesc.timestampWrites = nullptr;
		renderPassDesc.depthStencilAttachment = &rpDepthAttachment;
		renderPassDesc.nextInChain = nullptr; // TODO ensure this is set to nullptr for all descriptor constructors

		//Encode render commands
		wgpu::RenderPassEncoder quadPassEncoder = encoder.beginRenderPass(renderPassDesc);
		quadPassEncoder.setPipeline(_pQuadPipeline->Get());
		quadPassEncoder.setBindGroup(0, _pQuadPipeline->BindGroup(), 0, nullptr);
		quadPassEncoder.setVertexBuffer(0, _pQuadBuffer->Get(), 0, _pQuadBuffer->Size());

		quadPassEncoder.draw((uint32_t)_quad.vertices.size(), g_animationBuffer.Size()/*instance count*/, 0, 0);
		quadPassEncoder.end();
		quadPassEncoder.release();

		wgpu::CommandBufferDescriptor commandBufferDescriptor{};
		commandBufferDescriptor.label = "Default Command Buffer";
		wgpu::CommandBuffer commands = encoder.finish(commandBufferDescriptor);

		//submit
		queue.submit(commands);

		commands.release();
		encoder.release();
		toDisplay.release();
		surface.present();

					// Wait until queue is finished processing
#ifdef WEBGPU_BACKEND_WGPU
			queue.submit(0, nullptr);
#else
			device.tick();
#endif
    }

	//Uploads a texture resource to the given texture index used to store out atlases
	// will construct GPU atlas texture if it is not already constructed
	void Renderer::UploadTextureAtlas(Gfx::TextureResource const& atlas, uint8_t index)
	{
		assert(index < k_maxAtlas);

		if(!_pAtlases){
			_pAtlases = std::make_unique<GpuTexture>(
				wgpu::TextureDimension::_2D,
				wgpu::Extent3D{atlas.width, atlas.height, Gfx::k_maxAtlas},
				wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
				atlas.numChannels,
				atlas.channelDepthBytes,
				wgpu::TextureFormat::RGBA8Unorm,
				_pCtx->GetDevice(),
				"GpuAtlas");
		}

		wgpu::Queue queue = _pCtx->GetQueue();
		_pAtlases->EnqueueCopy(atlas.data.data(), atlas.Extents(), queue, {0,0,index});
	}


}