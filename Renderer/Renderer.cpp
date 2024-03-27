// Defines the entry point for the application.
#include <iostream>
#include <GLFW/glfw3.h>
#include "webgpu.h"
#include "Utils.h"
#include <glfw3webgpu.h>

class Window
{
public:
	Window() : pWindow(nullptr)
	{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		pWindow = glfwCreateWindow(800, 600, "WebGpu", nullptr, nullptr);
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

struct InterleavedVertex
{
	float x, y;
	float r, g, b;
};

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
		requiredDeviceLimits.limits.maxVertexAttributes = 2;
		requiredDeviceLimits.limits.maxVertexBuffers = 1;
		requiredDeviceLimits.limits.maxBufferSize = 15 * sizeof(InterleavedVertex); //2 2d tris
		requiredDeviceLimits.limits.maxVertexBufferArrayStride = sizeof(InterleavedVertex);
		requiredDeviceLimits.limits.maxInterStageShaderComponents = 3; // 3 extra floats transferred between vertex and fragment shader
		requiredDeviceLimits.limits.maxBindGroups = 1;
		requiredDeviceLimits.limits.maxUniformBuffersPerShaderStage = 1;
		requiredDeviceLimits.limits.maxUniformBufferBindingSize = 16 * sizeof(float); //uniform structs have a max size of 16 floats
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

		//Create uniform buffer
		wgpu::BufferDescriptor uniformBufferDesc{};
		uniformBufferDesc.size = sizeof(float);
		uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
		uniformBufferDesc.mappedAtCreation = false;
		wgpu::Buffer uniformBuffer = device.createBuffer(uniformBufferDesc);

		wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
		if (swapChainFormat == wgpu::TextureFormat::Undefined) swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;
		wgpu::SwapChainDescriptor swapChainDesc{};
		swapChainDesc.width = 800;
		swapChainDesc.height = 600;
		swapChainDesc.format = swapChainFormat;
		swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
		swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
		swapChainDesc.label = "Swapchain";
		wgpu::SwapChain swapchain = device.createSwapChain(surface, swapChainDesc);

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

		wgpu::FragmentState fragmentState{};
		fragmentState.module = shaderModule;
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;

		//binding layout
		wgpu::BindGroupLayoutEntry bindingLayout = wgpu::Default;
		bindingLayout.binding = 0; //slot id
		bindingLayout.visibility = wgpu::ShaderStage::Vertex;
		bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
		bindingLayout.buffer.minBindingSize = sizeof(float);

		//bind group layout
		wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
		bindGroupLayoutDesc.entryCount = 1;
		bindGroupLayoutDesc.entries = &bindingLayout;
		wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

		//actual bind group
		wgpu::BindGroupEntry binding{};
		binding.binding = 0; //Slot id
		binding.buffer = uniformBuffer;
		binding.offset = 0;
		binding.size = sizeof(float);

		wgpu::BindGroupDescriptor bindingDesc{};
		bindingDesc.layout = bindGroupLayout;
		bindingDesc.entryCount = bindGroupLayoutDesc.entryCount;
		bindingDesc.entries = &binding;
		wgpu::BindGroup bindGroup = device.createBindGroup(bindingDesc);

		//InterleavedVertex attributes
		std::vector<wgpu::VertexAttribute> vertexAttributes(2);
		vertexAttributes[0].shaderLocation = 0;
		vertexAttributes[0].format = wgpu::VertexFormat::Float32x2;
		vertexAttributes[0].offset = 0;

		vertexAttributes[1].shaderLocation = 1;
		vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
		vertexAttributes[1].offset = 2 * sizeof(float);

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

		pipelineDesc.depthStencil = nullptr;

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

		//Load object
		auto oObject = Utils::LoadGeometry(ASSETS_DIR "/object.txt");

		if (!oObject)
		{
			std::cout << "Failed to load object" << std::endl;
			return -1;
		}
		Object object = *oObject;

		//Create vertex buffer
		wgpu::BufferDescriptor vertexBufferDesc;
		vertexBufferDesc.size = object.points.size() * sizeof(float);
		vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		vertexBufferDesc.mappedAtCreation = false;
		wgpu::Buffer vertexBuffer = device.createBuffer(vertexBufferDesc);

		//upload vertex data to gpu
		queue.writeBuffer(vertexBuffer, 0, object.points.data(), vertexBufferDesc.size);

		//Create index buffer
		wgpu::BufferDescriptor indexBufferDesc;
		indexBufferDesc.size = object.indices.size() * sizeof(uint16_t);
		//wgpu has the requirement for data to be aligned to 4 bytes, so round up to the nearest 4
		indexBufferDesc.size = (indexBufferDesc.size + 3) & ~3;

		indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
		wgpu::Buffer indexBuffer = device.createBuffer(indexBufferDesc);

		//upload index buffer to gpu
		queue.writeBuffer(indexBuffer, 0, object.indices.data(), indexBufferDesc.size);

		//Temp buffer data
		uint32_t k_bufferSize = 16;
		std::vector<uint8_t> numbers(k_bufferSize);
		for (uint8_t i = 0; i < k_bufferSize; ++i) numbers[i] = i;

		//Create Buffer
		wgpu::BufferDescriptor bufferDesc1;
		bufferDesc1.nextInChain = nullptr;
		bufferDesc1.label = "buffer1";
		bufferDesc1.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
		bufferDesc1.size = k_bufferSize;
		bufferDesc1.mappedAtCreation = false;
		wgpu::Buffer buffer1 = device.createBuffer(bufferDesc1);

		wgpu::BufferDescriptor bufferDesc2;
		bufferDesc2.nextInChain = nullptr;
		bufferDesc2.label = "buffer2";
		bufferDesc2.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
		bufferDesc2.size = k_bufferSize;
		bufferDesc2.mappedAtCreation = false;
		wgpu::Buffer buffer2 = device.createBuffer(bufferDesc2);

		//Add instruction to copy to buffer
		queue.writeBuffer(buffer1, 0, numbers.data(), numbers.size());

		std::cout << "Sending buffer copy operation... " << std::endl;

		//Copy buffer1 to buffer2
		wgpu::CommandEncoderDescriptor encoderDesc1{};
		encoderDesc1.label = "Default Command Encoder";
		wgpu::CommandEncoder copyEncoder = device.createCommandEncoder(encoderDesc1);
		copyEncoder.copyBufferToBuffer(buffer1, 0, buffer2, 0, k_bufferSize);

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
		wgpuBufferMapAsync(buffer2, wgpu::MapMode::Read, 0, k_bufferSize, onBuffer2Mapped, nullptr);

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

			//Upload uniforms
			float currentTime = static_cast<float>(glfwGetTime());
			queue.writeBuffer(uniformBuffer, 0, &currentTime, sizeof(currentTime));

			wgpu::RenderPassColorAttachment rpColorAttachment{};
			rpColorAttachment.view = toDisplay;
			rpColorAttachment.resolveTarget = nullptr;
			rpColorAttachment.loadOp = wgpu::LoadOp::Clear;
			rpColorAttachment.storeOp = wgpu::StoreOp::Store;
			rpColorAttachment.clearValue = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

			wgpu::RenderPassDescriptor renderPassDesc{};
			renderPassDesc.colorAttachmentCount = 1;
			renderPassDesc.colorAttachments = &rpColorAttachment;
			renderPassDesc.timestampWriteCount = 0;
			renderPassDesc.timestampWrites = nullptr;
			renderPassDesc.depthStencilAttachment = nullptr;
			renderPassDesc.nextInChain = nullptr; //TODO ensure this is set to nullptr for all descriptor constructors
			wgpu::RenderPassEncoder renderPassEncoder = encoder.beginRenderPass(renderPassDesc);
			renderPassEncoder.setPipeline(pipeline);
			renderPassEncoder.setBindGroup(0, bindGroup, 0, nullptr);
			renderPassEncoder.setVertexBuffer(0, vertexBuffer, 0, object.points.size() * sizeof(float));
			renderPassEncoder.setIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16, 0, object.indices.size() * sizeof(uint16_t));
			renderPassEncoder.drawIndexed((uint32_t)object.indices.size(), 1, 0, 0, 0);
			renderPassEncoder.end(); // clears screen
			renderPassEncoder.release();

			wgpu::CommandBufferDescriptor commandBufferDescriptor{};
			commandBufferDescriptor.label = "Default Command Buffer";
			wgpu::CommandBuffer commands = encoder.finish(commandBufferDescriptor);

			//std::cout << "Submitting Render Commands \n";
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
		buffer1.destroy();
		buffer1.release();
		buffer2.destroy();
		buffer2.release();
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
