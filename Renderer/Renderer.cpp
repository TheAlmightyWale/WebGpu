// Defines the entry point for the application.
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
	float _pad[3]; //struct must be 16byte aligned
};
static_assert(sizeof(Uniforms) % 16 == 0);

struct CamUniforms
{
	Vec2f position;
	Vec2f extents;
};
static_assert(sizeof(CamUniforms) % 16 == 0);

struct QuadTransform
{
	Vec3f position;
	float _pad;
	Vec2f scale;
	float _pad2[2]; //Each member must be 16 byte aligned, min 32 bytes
};
static_assert(sizeof(QuadTransform) % 16 == 0);

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
		requiredDeviceLimits.limits.maxBufferSize = 1'000'000 * sizeof(InterleavedVertex);
		requiredDeviceLimits.limits.maxVertexBufferArrayStride = sizeof(InterleavedVertex);
		requiredDeviceLimits.limits.maxInterStageShaderComponents = 6; // everything other than default position needs to be under this max
		requiredDeviceLimits.limits.maxBindGroups = 1;
		requiredDeviceLimits.limits.maxUniformBuffersPerShaderStage = 2;
		requiredDeviceLimits.limits.maxUniformBufferBindingSize = sizeof(Uniforms);
		requiredDeviceLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
		requiredDeviceLimits.limits.maxTextureDimension1D = k_screenHeight;
		requiredDeviceLimits.limits.maxTextureDimension2D = k_screenWidth;
		requiredDeviceLimits.limits.maxTextureArrayLayers = 1;
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
			std::cout << "/n";

			assert(true);
		};
		auto pErrorCallback = device.setUncapturedErrorCallback(onDeviceError);

		wgpu::Queue queue = device.getQueue();

		auto onQueueWorkDone = [](wgpu::QueueWorkDoneStatus status) {
			std::cout << "Queued work completed with status: " << status << "\n";
		};
		queue.onSubmittedWorkDone(onQueueWorkDone);

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
		wgpu::SwapChainDescriptor swapChainDesc{};
		swapChainDesc.width = k_screenWidth;
		swapChainDesc.height = k_screenHeight;
		swapChainDesc.format = swapChainFormat;
		swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
		swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
		swapChainDesc.label = "Swapchain";
		wgpu::SwapChain swapchain = device.createSwapChain(surface, swapChainDesc);

		//Quad cam uniforms, to be updated when swap chain is resized
		CamUniforms quadCam;
		quadCam.position = Vec2f{ 0.5f, 0.5f };
		quadCam.extents = Vec2f{ (float)k_screenWidth, (float)k_screenHeight };

		Gfx::Buffer camBuffer{ sizeof(CamUniforms), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, "Camera Uniforms", device};
		camBuffer.EnqueueCopy(&quadCam, 0, queue);

		std::cout << "Swapchain: " << swapchain << "\n";

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

		auto oQuadShaderModule = Utils::LoadShaderModule(ASSETS_DIR "/quadShader.wgsl", device);
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

		//Default texture load
		TextureResource defaultRes = *Utils::LoadTexture(ASSETS_DIR "/default.png");
		Gfx::Texture defaultSprite{ wgpu::TextureDimension::_2D,
			{defaultRes.width, defaultRes.height, 1},
			wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
			wgpu::TextureFormat::RGBA8Unorm,
			device };
		defaultSprite.EnqueueCopy(defaultRes.data.data(), defaultRes.SizeBytes(), queue);

		wgpu::SamplerDescriptor spriteSamplerDesc;
		spriteSamplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
		spriteSamplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
		spriteSamplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
		spriteSamplerDesc.magFilter = wgpu::FilterMode::Linear;
		spriteSamplerDesc.minFilter = wgpu::FilterMode::Linear;
		spriteSamplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
		spriteSamplerDesc.lodMinClamp = 0.0f;
		spriteSamplerDesc.lodMaxClamp = 1.0f;
		spriteSamplerDesc.compare = wgpu::CompareFunction::Undefined;
		spriteSamplerDesc.maxAnisotropy = 1;
		wgpu::Sampler spriteSampler = device.createSampler(spriteSamplerDesc);

		Gfx::QuadRenderPipeline quadPipeline(device, quadShaderModule, colorTarget, depthStencilState);

		//Placeholder Quad transform
		QuadTransform transform
		{
			{0.f, 0.f, 0.f}, {0},
			{50.0f, 50.0f}
		};
		Gfx::Buffer transformBuffer{ sizeof(QuadTransform), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, "Transform Buffer", device };
		transformBuffer.EnqueueCopy(&transform, 0, queue);

		quadPipeline.BindData(transformBuffer, defaultSprite, camBuffer, device);

		//Create depth texture and depth texture view
		Gfx::Texture depthTexture(wgpu::TextureDimension::_2D, { swapChainDesc.width, swapChainDesc.height, 1 },
			wgpu::TextureUsage::RenderAttachment, depthTextureFormat, device);

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
			glfwPollEvents();

			wgpu::TextureView toDisplay = swapchain.getCurrentTextureView();
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
			renderPassDesc.timestampWriteCount = 0;
			renderPassDesc.timestampWrites = nullptr;
			renderPassDesc.depthStencilAttachment = &rpDepthAttachment;
			renderPassDesc.nextInChain = nullptr; //TODO ensure this is set to nullptr for all descriptor constructors

			wgpu::RenderPassEncoder quadPassEncoder = encoder.beginRenderPass(renderPassDesc);
			quadPassEncoder.setPipeline(quadPipeline.Get());
			quadPassEncoder.setBindGroup(0, quadPipeline.BindGroup(), 0, nullptr);
			quadPassEncoder.setVertexBuffer(0, quadBuffer.Get(), 0, quadBuffer.Size());
			quadPassEncoder.draw((uint32_t)quad.vertices.size(), 1, 0, 0);
			quadPassEncoder.end();
			quadPassEncoder.release();

			wgpu::CommandBufferDescriptor commandBufferDescriptor{};
			commandBufferDescriptor.label = "Default Command Buffer";
			wgpu::CommandBuffer commands = encoder.finish(commandBufferDescriptor);

			queue.submit(commands);
			
			commands.release();
			encoder.release();
			toDisplay.release();
			swapchain.present();

			//Wait until queue is finished processing
#ifdef WEBGPU_BACKEND_WGPU
			queue.submit(0, nullptr);
#else
			device.tick();
#endif
		}

		//TODO raii webgpu generator
		depthTextureView.release();

		swapchain.release();
		queue.release();
		device.release();
		adapter.release();
		surface.release();
		instance.release();

	}

	glfwTerminate();
	return 0;
}
