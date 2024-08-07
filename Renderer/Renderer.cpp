﻿// Defines the entry point for the application.
#include <iostream>
#include <GLFW/glfw3.h>
#include "webgpu.h"
#include "Utils.h"
#include <glfw3webgpu.h>
#include <array>
#include "MathDefs.h"
#include "MeshDefs.h"
#include "Buffer.h"
#include "Texture.h"
#include "Quad.h"
#include "QuadRenderPipeline.h"
#include "QuadDefs.h"
#include "Terrain.h"
#include "Chrono.h"
#include "ResourceManager.h"

constexpr uint32_t k_screenWidth = 800;
constexpr uint32_t k_screenHeight = 600;

class Window
{
public:
	Window() : pWindow(nullptr)
	{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		pWindow = glfwCreateWindow(k_screenWidth, k_screenHeight, "WebGpu", nullptr, nullptr);
	}

	Window(Window const&) = delete;
	Window& operator=(Window const&) = delete;

	~Window() 
	{
		if(pWindow) glfwDestroyWindow(pWindow);
	}

	inline bool ShouldClose() { return glfwWindowShouldClose(pWindow); }

	inline GLFWwindow* get() { return pWindow; }

private:
	GLFWwindow* pWindow;
};

struct Uniforms
{
	Mat4f projection;
	Mat4f view;
	Mat4f model;
	Vec4f color;
	float time;
	float _pad[3] = {0.f,0.f,0.f}; //struct must be 16byte aligned
};
static_assert(sizeof(Uniforms) % 16 == 0);

constexpr uint32_t k_mbBytes = 1024 * 1024;

uint32_t CeilToNextMultiple(uint32_t value, uint32_t multiple)
{
	uint32_t divideAndCeil = value / multiple + (value % multiple == 0 ? 0 : 1);
	return multiple * divideAndCeil;
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Could not initialize GLFW \n";
		return 1;
	}

	//We want to destruct everything in here before calling glfwTerminate
	{
		Window window;

		wgpu::InstanceDescriptor desc{};
		wgpu::Instance instance = wgpu::createInstance(desc);

		if (!instance)
		{
			std::cerr << "Could not initialize WebGPU /n";
			return 1;
		}

		std::cout << "WGPU Instance: " << instance << "/n";

		wgpu::Surface surface{ glfwGetWGPUSurface(instance, window.get()) };
		wgpu::RequestAdapterOptions adapterOptions{};
		adapterOptions.compatibleSurface = surface;
		wgpu::Adapter adapter = instance.requestAdapter(adapterOptions);

		std::vector<wgpu::FeatureName> features;
		size_t featureCount = adapter.enumerateFeatures(nullptr);
		features.resize(featureCount, wgpu::FeatureName::Undefined);
		adapter.enumerateFeatures(features.data());

		std::cout << "Adapter Features: \n";
		for (auto const& feature : features)
		{
			std::cout << " - " << feature << "\n";
		}

		wgpu::SupportedLimits adapterLimits;
		adapter.getLimits(&adapterLimits);
		std::cout << "adapter.maxVertexAttributes: " << adapterLimits.limits.maxVertexAttributes << "\n";

		//It's best practice to set these as low as possible to alert you when you are using more resources than you wants
		wgpu::RequiredLimits requiredDeviceLimits = wgpu::Default;
		requiredDeviceLimits.limits.maxVertexAttributes = 3;
		requiredDeviceLimits.limits.maxVertexBuffers = 1;
		requiredDeviceLimits.limits.maxBufferSize = k_mbBytes;
		requiredDeviceLimits.limits.maxVertexBufferArrayStride = sizeof(Gfx::QuadVertex);
		requiredDeviceLimits.limits.maxInterStageShaderComponents = 6; // everything other than default position needs to be under this max
		requiredDeviceLimits.limits.maxBindGroups = 1;
		requiredDeviceLimits.limits.maxBindingsPerBindGroup = 10;
		requiredDeviceLimits.limits.maxUniformBuffersPerShaderStage = 3;
		requiredDeviceLimits.limits.maxUniformBufferBindingSize = 100 * sizeof(AnimUniform);
		requiredDeviceLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
		requiredDeviceLimits.limits.maxTextureDimension1D = k_screenHeight;
		requiredDeviceLimits.limits.maxTextureDimension2D = k_screenWidth;
		requiredDeviceLimits.limits.maxTextureArrayLayers = 2;
		requiredDeviceLimits.limits.maxSampledTexturesPerShaderStage = 1;
		requiredDeviceLimits.limits.maxSamplersPerShaderStage = 1;

		//Must be set even if we don't use em yet
		requiredDeviceLimits.limits.minStorageBufferOffsetAlignment = adapterLimits.limits.minStorageBufferOffsetAlignment;
		requiredDeviceLimits.limits.minUniformBufferOffsetAlignment = adapterLimits.limits.minUniformBufferOffsetAlignment;

		wgpu::DeviceDescriptor deviceDescriptor{};
		deviceDescriptor.label = "Default Device";
		deviceDescriptor.defaultQueue.label = "Default Queue";
		deviceDescriptor.requiredLimits = &requiredDeviceLimits;
		wgpu::Device device = adapter.requestDevice(deviceDescriptor);

		wgpu::SupportedLimits deviceLimits;
		device.getLimits(&deviceLimits);
		std::cout << "device.maxVertexAttributes: " << deviceLimits.limits.maxVertexAttributes << "\n";

		uint32_t uniformStride = CeilToNextMultiple((uint32_t)sizeof(Uniforms), (uint32_t)deviceLimits.limits.minUniformBufferOffsetAlignment);

		auto onDeviceError = [](wgpu::ErrorType type, char const* message) {
			std::cout << "Uncaptured Device error: type-" << type;
			if (message) std::cout << " (" << message << ")";
			std::cout << "\n";

			assert(true);
		};
		auto pErrorCallback = device.setUncapturedErrorCallback(onDeviceError);

		wgpu::Queue queue = device.getQueue();

		//disabled for now due to causing crashes on surface.configure
		//auto onQueueWorkDone = [](wgpu::QueueWorkDoneStatus status) {
		//	std::cout << "Queued work completed with status: " << status << "\n";
		//};
		//queue.onSubmittedWorkDone(onQueueWorkDone);

		float angle1 = 2.0f; //arbitrary
		float angle2 = 3.0f * PI / 4.0f;
		Vec3f focalPoint = { 0.0f, 0.0f, -2.0f };

		Mat4f scale = glm::scale(Mat4f(1.0f), Vec3f(0.3f));
		Mat4f translation1 = glm::translate(Mat4f(1.0f), Vec3f(0.5f, 0.f, 0.f));
		Mat4f rotation1 = glm::rotate(Mat4f(1.0f), angle1, Vec3f(0.0f, 0.0f, 1.0f));

		Mat4f translation2 = glm::translate(Mat4f(1.0f), -focalPoint);
		Mat4f rotation2 = glm::rotate(Mat4f(1.0f), angle2, Vec3f(1.0f, 0.0f, 0.0f));

		float focalLength = 2.0f;
		float fov = 2.0f * glm::atan(1.0f, focalLength);
		float ratio = (float)k_screenWidth / (float)k_screenHeight;
		float nearPlane = 0.01f;
		float farPlane = 100.0f;

		Uniforms uniform
		{
			.projection = glm::perspective(fov, ratio, nearPlane, farPlane),
			.view = translation2 * rotation2,
			.model = rotation1 * translation1 * scale, //rotation after translation to orbit
			.color{0.0f, 1.0f, 0.4f, 1.0f},
			.time{1.0f}
		};

		Gfx::Buffer uniformBuffer(
			uniformStride + sizeof(Uniforms),
			wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
			"Uniform Buffer",
			device);

		wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
		if (swapChainFormat == wgpu::TextureFormat::Undefined) swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;
		
		//Swapchain is configured through surface
		wgpu::SurfaceConfiguration surfaceConfig = wgpu::Default;
		surfaceConfig.nextInChain = nullptr;
		surfaceConfig.width = k_screenWidth;
		surfaceConfig.height = k_screenHeight;
		surfaceConfig.format = swapChainFormat;
		surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
		surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
		surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
		surfaceConfig.viewFormatCount = 0;
		surfaceConfig.viewFormats = nullptr;
		surfaceConfig.device = device;
		surface.configure(surfaceConfig);

		//Quad cam uniforms, to be updated when swap chain is resized
		CamUniforms quadCam;
		quadCam.position = Vec2f{ 0.5f, 0.5f };
		quadCam.extents = Vec2f{ (float)k_screenWidth, (float)k_screenHeight };

		Gfx::Buffer camBuffer{ sizeof(CamUniforms), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, "Camera Uniforms", device};
		camBuffer.EnqueueCopy(&quadCam, 0, queue);

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
		auto oQuadShaderModule = Utils::LoadShaderModule(assetsBasePath / "quadShader.wgsl", device);
		if (!oQuadShaderModule)
		{
			std::cout << "Failed to create Quad Shader Module" << std::endl;
			return -1;
		}
		wgpu::ShaderModule quadShaderModule = *oQuadShaderModule;

		wgpu::DepthStencilState depthStencilState = wgpu::Default;
		depthStencilState.depthCompare = wgpu::CompareFunction::Less;
		depthStencilState.depthWriteEnabled = true;
		wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
		depthStencilState.format = depthTextureFormat;
		//No stencil ability
		depthStencilState.stencilReadMask = 0;
		depthStencilState.stencilWriteMask = 0;

		//Temp animation load
		ResourceManager resources;
		resources.LoadAllAnimations(assetsBasePath);
		auto anim = resources.GetAnimation("cell1");
		//Upload anim strip
		Gfx::Texture animTex{
			wgpu::TextureDimension::_2D,
			{anim.width, anim.height, 2},
			wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
			anim.numChannels,
			anim.channelDepthBytes,
			wgpu::TextureFormat::RGBA8Unorm,
			device,
			"animation"
		};
		animTex.EnqueueCopy(anim.data.data(), anim.Extents(), queue);

		//Load other animations and offset copy
		TextureResource const& texture2 = resources.GetAnimation("cell2");
		animTex.EnqueueCopy(texture2.data.data(), texture2.Extents(), queue, {0,0,1});

		//wgpu::SamplerDescriptor spriteSamplerDesc;
		//spriteSamplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
		//spriteSamplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
		//spriteSamplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
		//spriteSamplerDesc.magFilter = wgpu::FilterMode::Linear;
		//spriteSamplerDesc.minFilter = wgpu::FilterMode::Linear;
		//spriteSamplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
		//spriteSamplerDesc.lodMinClamp = 0.0f;
		//spriteSamplerDesc.lodMaxClamp = 1.0f;
		//spriteSamplerDesc.compare = wgpu::CompareFunction::Undefined;
		//spriteSamplerDesc.maxAnisotropy = 1;
		//wgpu::Sampler spriteSampler = device.createSampler(spriteSamplerDesc);

		Gfx::QuadRenderPipeline quadPipeline(device, quadShaderModule, colorTarget, depthStencilState);

		Terrain terrain(10, 10, 50);

		Gfx::Buffer transformBuffer{(uint32_t)(terrain.Cells().size() * sizeof(QuadTransform)), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
			"Transform Buffer", device};
		transformBuffer.EnqueueCopy(terrain.Cells().data(), 0, queue);

		Gfx::Buffer animationBuffer{(uint32_t)(terrain.CellAnimations().size() * sizeof(AnimUniform)), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
			"Animations", device };
		animationBuffer.EnqueueCopy(terrain.CellAnimations().data(), 0, queue);

		quadPipeline.BindData(transformBuffer, animTex, camBuffer, animationBuffer, device);

		//Create depth texture and depth texture view
		Gfx::Texture depthTexture(wgpu::TextureDimension::_2D, { surfaceConfig.width, surfaceConfig.height, 1 },
			wgpu::TextureUsage::RenderAttachment, 1, 3/*24 bit depth*/, depthTextureFormat, device, "depth");

		wgpu::TextureViewDescriptor depthTextureViewDesc;
		depthTextureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
		depthTextureViewDesc.baseArrayLayer = 0;
		depthTextureViewDesc.arrayLayerCount = 1;
		depthTextureViewDesc.baseMipLevel = 0;
		depthTextureViewDesc.mipLevelCount = 1;
		depthTextureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
		depthTextureViewDesc.format = depthTextureFormat;
		wgpu::TextureView depthTextureView = depthTexture.Get().createView(depthTextureViewDesc);

		//Temp buffer data
		uint32_t k_bufferSize = 16;
		std::vector<uint8_t> numbers(k_bufferSize);
		for (uint8_t i = 0; i < k_bufferSize; ++i) numbers[i] = i;

		//Create Buffer
		Gfx::Buffer buffer1{ k_bufferSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc, "buffer1", device };
		Gfx::Buffer buffer2{ k_bufferSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, "buffer2", device };

		//Add instruction to copy to buffer
		buffer1.EnqueueCopy(numbers.data(), 0, queue);

		std::cout << "Sending buffer copy operation... " << std::endl;

		//Copy buffer1 to buffer2
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

		auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* pUserData)
		{
			BufferMappedContext* pContext = reinterpret_cast<BufferMappedContext*>(pUserData);
			std::cout << "Buffer 2 Mapped with status: " << status << "\n";

			if (status != wgpu::BufferMapAsyncStatus::Success || pContext == nullptr) return;
			//uint8_t* pBufferData = (uint8_t*)pContext->buffer.getConstMappedRange(0, pContext->bufferSize);
			
			//Once we are done with this data, unmap the buffer
			pContext->buffer.unmap();
		};
		wgpuBufferMapAsync(buffer2.Get(), wgpu::MapMode::Read, 0, k_bufferSize, onBuffer2Mapped, nullptr);

		//Quad upload
		Gfx::Quad quad;
		Gfx::Buffer quadBuffer{
			sizeof(Gfx::Quad), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex, "Quad Vertices", device
		};
		quadBuffer.EnqueueCopy(quad.vertices.data(), 0, queue);

		while (!window.ShouldClose())
		{
			Clock::Tick();
			float deltaTime = Clock::GetDelta();

			glfwPollEvents();

			wgpu::SurfaceTexture surfaceTexture;
			surface.getCurrentTexture(&surfaceTexture);
			if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
				std::cerr << "Failed to get surfaceTexture, status code: " << surfaceTexture.status << "\n";
				break;
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
				break;
			}

			wgpu::CommandEncoderDescriptor encoderDesc{};
			encoderDesc.label = "Default Command Encoder";
			wgpu::CommandEncoder encoder = device.createCommandEncoder(encoderDesc);

			//Update view matrix
			angle1 = uniform.time;
			rotation1 = glm::rotate(Mat4f(1.0f), angle1, Vec3f(0.0f, 0.0f, 1.0f));
			uniform.model = rotation1 * translation1 * scale;

			//EnqueueCopy uniforms
			uniform.time = static_cast<float>(glfwGetTime());
			uniformBuffer.EnqueueCopy(&uniform, sizeof(Uniforms), 0, queue);

			//Update animations
			terrain.Animate(deltaTime);
			animationBuffer.EnqueueCopy(terrain.CellAnimations().data(), 0, queue);

			wgpu::RenderPassColorAttachment rpColorAttachment{};
			rpColorAttachment.view = toDisplay;
			rpColorAttachment.resolveTarget = nullptr;
			rpColorAttachment.loadOp = wgpu::LoadOp::Clear;
			rpColorAttachment.storeOp = wgpu::StoreOp::Store;
			rpColorAttachment.clearValue = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

			wgpu::RenderPassDepthStencilAttachment rpDepthAttachment;
			rpDepthAttachment.view = depthTextureView;
			rpDepthAttachment.depthClearValue = 1.0f; //Maximum distance possible
			rpDepthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
			rpDepthAttachment.depthStoreOp = wgpu::StoreOp::Store;
			rpDepthAttachment.depthReadOnly = false;
			//current unused stencil params, but need to be filled out
			rpDepthAttachment.stencilClearValue = 0;
			rpDepthAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
			rpDepthAttachment.stencilStoreOp = wgpu::StoreOp::Store;
			rpDepthAttachment.stencilReadOnly = true;

			wgpu::RenderPassDescriptor renderPassDesc{};
			renderPassDesc.colorAttachmentCount = 1;
			renderPassDesc.colorAttachments = &rpColorAttachment;
			renderPassDesc.timestampWrites = nullptr;
			renderPassDesc.depthStencilAttachment = &rpDepthAttachment;
			renderPassDesc.nextInChain = nullptr; //TODO ensure this is set to nullptr for all descriptor constructors

			wgpu::RenderPassEncoder quadPassEncoder = encoder.beginRenderPass(renderPassDesc);
			quadPassEncoder.setPipeline(quadPipeline.Get());
			quadPassEncoder.setBindGroup(0, quadPipeline.BindGroup(), 0, nullptr);
			quadPassEncoder.setVertexBuffer(0, quadBuffer.Get(), 0, quadBuffer.Size());
			quadPassEncoder.draw((uint32_t)quad.vertices.size(), (uint32_t)terrain.Cells().size() /*instance count*/, 0, 0);
			quadPassEncoder.end();
			quadPassEncoder.release();

			wgpu::CommandBufferDescriptor commandBufferDescriptor{};
			commandBufferDescriptor.label = "Default Command Buffer";
			wgpu::CommandBuffer commands = encoder.finish(commandBufferDescriptor);

			queue.submit(commands);
			
			commands.release();
			encoder.release();
			toDisplay.release();
			surface.present();

			//Wait until queue is finished processing
#ifdef WEBGPU_BACKEND_WGPU
			queue.submit(0, nullptr);
#else
			device.tick();
#endif
		}

		//TODO raii webgpu generator
		depthTextureView.release();
		queue.release();
		device.release();
		adapter.release();
		surface.release();
		instance.release();

	}

	glfwTerminate();
	return 0;
}
