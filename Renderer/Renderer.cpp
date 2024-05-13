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

		auto oShaderModule = Utils::LoadShaderModule(ASSETS_DIR "/shader.wgsl", device);
		if (!oShaderModule)
		{
			std::cout << "Failed to create Shader Module" << std::endl;
			return -1;
		}
		wgpu::ShaderModule shaderModule = *oShaderModule;

		auto oQuadShaderModule = Utils::LoadShaderModule(ASSETS_DIR "/quadShader.wgsl", device);
		if (!oQuadShaderModule)
		{
			std::cout << "Failed to create Quad Shader Module" << std::endl;
			return -1;
		}
		wgpu::ShaderModule quadShaderModule = *oQuadShaderModule;

		wgpu::FragmentState quadFragmentState{};
		quadFragmentState.module = quadShaderModule;
		quadFragmentState.constantCount = 0;
		quadFragmentState.constants = nullptr;
		quadFragmentState.entryPoint = "fs_main";
		quadFragmentState.targetCount = 1;
		quadFragmentState.targets = &colorTarget;


		wgpu::FragmentState fragmentState{};
		fragmentState.module = shaderModule;
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;

		Gfx::Texture texture(wgpu::TextureDimension::_2D, { 256,256,1 },
			wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
			wgpu::TextureFormat::RGBA8Unorm,
			device);

		//Fake image data
		std::vector<uint8_t> pixels(4 * texture.Extents().width * texture.Extents().height);
		for (uint32_t i = 0; i < texture.Extents().width; i++)
		{
			for (uint32_t j = 0; j < texture.Extents().height; j++)
			{
				uint8_t* p = &pixels[4 * (j * texture.Extents().width + i)];
				//rgba
				p[0] = (uint8_t)i;
				p[1] = (uint8_t)j;
				p[2] = 128;
				p[3] = 255;
			}
		}

		//Describe and upload to gpu
		wgpu::ImageCopyTexture destination;
		destination.texture = texture.Get();
		destination.mipLevel = 0;
		destination.origin = { 0, 0, 0 };
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::TextureDataLayout source;
		source.offset = 0;
		source.bytesPerRow = 4 * texture.Extents().width;
		source.rowsPerImage = texture.Extents().height;
		queue.writeTexture(destination, pixels.data(), pixels.size(), source, texture.Extents());

		//Textureview
		wgpu::TextureViewDescriptor textureViewDesc;
		textureViewDesc.aspect = wgpu::TextureAspect::All;
		textureViewDesc.baseArrayLayer = 0;
		textureViewDesc.arrayLayerCount = 1;
		textureViewDesc.baseMipLevel = 0;
		textureViewDesc.mipLevelCount = 1;
		textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
		textureViewDesc.format = texture.Format();
		wgpu::TextureView textureView = texture.Get().createView(textureViewDesc);

		//Binding layouts
		std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(2, wgpu::Default);

		wgpu::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
		uniformBindingLayout.binding = 0; //slot id
		uniformBindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
		uniformBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
		uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);
		uniformBindingLayout.buffer.hasDynamicOffset = true; //Dynamic Buffer

		wgpu::BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
		textureBindingLayout.binding = 1;
		textureBindingLayout.visibility = wgpu::ShaderStage::Fragment;
		textureBindingLayout.texture.sampleType = wgpu::TextureSampleType::Float;
		textureBindingLayout.texture.viewDimension = wgpu::TextureViewDimension::_2D;

		//bind group layout
		wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
		bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
		bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
		wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

		//actual bind group
		std::vector<wgpu::BindGroupEntry> bindings{ 2 };
		wgpu::BindGroupEntry& uniformBinding = bindings[0];
		uniformBinding.binding = 0; //Slot id
		uniformBinding.buffer = uniformBuffer.Get();
		uniformBinding.offset = 0;
		uniformBinding.size = sizeof(Uniforms);

		wgpu::BindGroupEntry& textureBinding = bindings[1];
		textureBinding.binding = 1;
		textureBinding.textureView = textureView;

		wgpu::BindGroupDescriptor bindingDesc{};
		bindingDesc.layout = bindGroupLayout;
		bindingDesc.entryCount = bindGroupLayoutDesc.entryCount;
		bindingDesc.entries = bindings.data();
		wgpu::BindGroup bindGroup = device.createBindGroup(bindingDesc);

		//InterleavedVertex attributes
		std::vector<wgpu::VertexAttribute> vertexAttributes(3);
		vertexAttributes[0].shaderLocation = 0;
		vertexAttributes[0].format = wgpu::VertexFormat::Float32x3; //TODO handle 2d
		vertexAttributes[0].offset = 0;
		//Normal
		vertexAttributes[1].shaderLocation = 1;
		vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
		vertexAttributes[1].offset = offsetof(InterleavedVertex, nx);
		//Color
		vertexAttributes[2].shaderLocation = 2;
		vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
		vertexAttributes[2].offset = offsetof(InterleavedVertex, r);

		//InterleavedVertex buffer layouts
		wgpu::VertexBufferLayout vertexBufferLayout;
		vertexBufferLayout.attributeCount = (uint32_t)vertexAttributes.size();
		vertexBufferLayout.attributes = vertexAttributes.data();
		vertexBufferLayout.arrayStride = sizeof(InterleavedVertex);
		vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

		wgpu::RenderPipelineDescriptor pipelineDesc{};
		pipelineDesc.vertex.bufferCount = 1;
		pipelineDesc.vertex.buffers = &vertexBufferLayout;
		pipelineDesc.vertex.module = shaderModule;
		pipelineDesc.vertex.entryPoint = "vs_main";
		pipelineDesc.vertex.constantCount = 0;
		pipelineDesc.vertex.constants = nullptr;

		pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
		pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
		pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
		pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

		pipelineDesc.fragment = &fragmentState;


		wgpu::DepthStencilState depthStencilState = wgpu::Default;
		depthStencilState.depthCompare = wgpu::CompareFunction::Less;
		depthStencilState.depthWriteEnabled = true;
		wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
		depthStencilState.format = depthTextureFormat;
		//No stencil ability
		depthStencilState.stencilReadMask = 0;
		depthStencilState.stencilWriteMask = 0;

		pipelineDesc.depthStencil = &depthStencilState;

		pipelineDesc.multisample.count = 1;
		pipelineDesc.multisample.mask = ~0u; //all bits on
		pipelineDesc.multisample.alphaToCoverageEnabled = false;

		//Pipeline layout
		wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{};
		pipelineLayoutDesc.bindGroupLayoutCount = 1;
		pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
		wgpu::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutDesc);

		pipelineDesc.layout = pipelineLayout;

		wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

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
		

		//Quad Pipeline start
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

		//Commented for now until we support both 2d and 3d
		////Load object
		//auto oObject = Utils::LoadGeometry(ASSETS_DIR "/object.txt", 2);

		//if (!oObject)
		//{
		//	std::cout << "Failed to load object" << std::endl;
		//	return -1;
		//}
		//Object object = *oObject;

		////Create vertex buffer
		//wgpu::BufferDescriptor vertexBufferDesc;
		//vertexBufferDesc.size = object.points.size() * sizeof(float);
		//vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		//vertexBufferDesc.mappedAtCreation = false;
		//wgpu::Buffer vertexBuffer = device.createBuffer(vertexBufferDesc);

		////upload vertex data to gpu
		//queue.writeBuffer(vertexBuffer, 0, object.points.data(), vertexBufferDesc.size);

		////Create index buffer
		//wgpu::BufferDescriptor indexBufferDesc;
		//indexBufferDesc.size = object.indices.size() * sizeof(uint16_t);
		////wgpu has the requirement for data to be aligned to 4 bytes, so round up to the nearest 4
		//indexBufferDesc.size = (indexBufferDesc.size + 3) & ~3;

		//indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
		//wgpu::Buffer indexBuffer = device.createBuffer(indexBufferDesc);

		////upload index buffer to gpu
		//queue.writeBuffer(indexBuffer, 0, object.indices.data(), indexBufferDesc.size);

		//Load pyramid
		auto oPyramid = Utils::LoadGeometry(ASSETS_DIR "/mammoth.obj");
		if (!oPyramid)
		{
			std::cout << "Failed to load pyramid" << std::endl;
			return -1;
		}

		Object pyramid = *oPyramid;
		//Create pyramid vertex buffer
		Gfx::Buffer vertexBuffer2(
			(uint32_t)pyramid.shapes[0].points.size() * sizeof(InterleavedVertex),
			wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
			"vertexBuffer2",
			device);
		//upload vertex data to gpu
		vertexBuffer2.EnqueueCopy(pyramid.shapes[0].points.data(), 0, queue);

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
			//wgpu::RenderPassEncoder renderPassEncoder = encoder.beginRenderPass(renderPassDesc);
			//renderPassEncoder.setPipeline(pipeline);

			//uint32_t dynamicOffset = 0;
			//renderPassEncoder.setBindGroup(0, bindGroup, 1, &dynamicOffset);

			////renderPassEncoder.setVertexBuffer(0, vertexBuffer, 0, object.points.size() * sizeof(float));
			////renderPassEncoder.setIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16, 0, object.indices.size() * sizeof(uint16_t));
			////renderPassEncoder.drawIndexed((uint32_t)object.indices.size(), 1, 0, 0, 0);

			//////Second draw with differnt uniforms
			////dynamicOffset = uniformStride;
			////renderPassEncoder.setBindGroup(0, bindGroup, 1, &dynamicOffset);
			////renderPassEncoder.drawIndexed((uint32_t)object.indices.size(), 1, 0, 0, 0);

			////pyramid draw
			//renderPassEncoder.setVertexBuffer(0, vertexBuffer2.Get(), 0, pyramid.shapes[0].points.size() * sizeof(InterleavedVertex));
			//renderPassEncoder.draw((uint32_t)pyramid.shapes[0].points.size(), 1, 0, 0);

			//renderPassEncoder.end(); // clears screen
			//renderPassEncoder.release();

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

			//std::cout << "Submitting Upload Commands \n";
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
