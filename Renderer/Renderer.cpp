// Defines the entry point for the application.
#include <iostream>
#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

const char* k_shaderSource = R"(
	@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

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

		wgpu::DeviceDescriptor deviceDescriptor{};
		deviceDescriptor.label = "Default Device";
		deviceDescriptor.defaultQueue.label = "Default Queue";
		wgpu::Device device = adapter.requestDevice(deviceDescriptor);

		auto onDeviceError = [](wgpu::ErrorType type, char const* message) {
			std::cout << "Uncaptured Device error: type-" << type;
			if (message) std::cout << " (" << message << ")";
			std::cout << "/n";

			assert(true);
		};
		device.setUncapturedErrorCallback(onDeviceError);

		wgpu::Queue queue = device.getQueue();

		auto onQueueWorkDone = [](wgpu::QueueWorkDoneStatus status) {
			std::cout << "Queued work completed with status: " << status << "\n";
		};
		queue.onSubmittedWorkDone(onQueueWorkDone);

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

		wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc{};
		shaderCodeDesc.chain.next = nullptr;
		shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
		shaderCodeDesc.code = k_shaderSource;

		wgpu::ShaderModuleDescriptor shaderDesc{};
		shaderDesc.nextInChain = &shaderCodeDesc.chain;
#ifdef WEBGPU_BACKEND_WGPU
		shaderDesc.hintCount = 0;
		shaderDesc.hints = nullptr;
#endif
		wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);

		wgpu::FragmentState fragmentState{};
		fragmentState.module = shaderModule;
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;

		wgpu::RenderPipelineDescriptor pipelineDesc{};
		pipelineDesc.vertex.bufferCount = 0;
		pipelineDesc.vertex.buffers = nullptr;
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

		pipelineDesc.layout = nullptr;

		wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

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

		auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* /*userData*/) {
			std::cout << "Buffer 2 Mapped with status: " << status << "\n";
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
			renderPassEncoder.draw(3, 1, 0, 0);
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
		}

		//TODO raii webgpu generator
		//buffer1.destroy();
		//buffer1.release();
		//buffer2.destroy();
		//buffer2.release();
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
