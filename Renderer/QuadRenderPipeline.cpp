#include "QuadRenderPipeline.h"
#include "Utils.h"
#include "Quad.h"

namespace Gfx
{

	QuadRenderPipeline::QuadRenderPipeline(
		wgpu::Device device,
		wgpu::ShaderModule shaders,
		wgpu::ColorTargetState outputTarget,
		wgpu::DepthStencilState depthStencil)
		: _pipeline(nullptr)
		, _bindLayout(nullptr)
		, _bindGroup(nullptr)
		, _sampler(nullptr)
	{
		std::vector<wgpu::VertexAttribute> quadVertexAttributes{ 2 };
		quadVertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
		quadVertexAttributes[0].offset = 0;
		quadVertexAttributes[0].shaderLocation = 0;
		quadVertexAttributes[1].format = wgpu::VertexFormat::Float32x2;
		quadVertexAttributes[1].offset = offsetof(Gfx::QuadVertex, u);
		quadVertexAttributes[1].shaderLocation = 1;

		wgpu::VertexBufferLayout quadVertexBufferLayout;
		quadVertexBufferLayout.attributeCount = (uint32_t)quadVertexAttributes.size();
		quadVertexBufferLayout.attributes = quadVertexAttributes.data();
		quadVertexBufferLayout.arrayStride = sizeof(Gfx::QuadVertex);
		quadVertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

		wgpu::VertexState vertexState{};
		vertexState.bufferCount = 1;
		vertexState.buffers = &quadVertexBufferLayout;
		vertexState.module = shaders;
		vertexState.entryPoint = "vs_main";
		vertexState.constantCount = 0;
		vertexState.constants = nullptr;

		wgpu::FragmentState fragmentState{};
		fragmentState.module = shaders;
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;
		fragmentState.entryPoint = "fs_main";
		fragmentState.targetCount = 1;
		fragmentState.targets = &outputTarget;

		wgpu::BindGroupLayoutEntry& transformBinding = _bindLayouts[0];
		transformBinding.binding = 0; //Slot id
		transformBinding.visibility = wgpu::ShaderStage::Vertex;
		transformBinding.buffer.type = wgpu::BufferBindingType::Uniform;
		transformBinding.buffer.minBindingSize = sizeof(QuadTransform);
		transformBinding.buffer.hasDynamicOffset = false;

		wgpu::BindGroupLayoutEntry& textureBinding = _bindLayouts[1];
		textureBinding.binding = 1;
		textureBinding.visibility = wgpu::ShaderStage::Fragment;
		textureBinding.texture.sampleType = wgpu::TextureSampleType::Float;
		textureBinding.texture.viewDimension = wgpu::TextureViewDimension::_2D;

		wgpu::BindGroupLayoutEntry& samplerBinding = _bindLayouts[2];
		samplerBinding.binding = 2;
		samplerBinding.visibility = wgpu::ShaderStage::Fragment;
		samplerBinding.sampler.type = wgpu::SamplerBindingType::Filtering;

		wgpu::BindGroupLayoutEntry& cameraUniformBinding = _bindLayouts[3];
		cameraUniformBinding.binding = 3;
		cameraUniformBinding.visibility = wgpu::ShaderStage::Vertex;
		cameraUniformBinding.buffer.type = wgpu::BufferBindingType::Uniform;
		cameraUniformBinding.buffer.minBindingSize = sizeof(CamUniforms);
		cameraUniformBinding.buffer.hasDynamicOffset = false;

		wgpu::BindGroupLayoutDescriptor bindLayoutDesc;
		bindLayoutDesc.entryCount = k_QuadPipelineBindingCount;
		bindLayoutDesc.entries = _bindLayouts.data();
		_bindLayout = device.createBindGroupLayout(bindLayoutDesc);

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
		_sampler = device.createSampler(spriteSamplerDesc);

		wgpu::PipelineLayoutDescriptor quadLayoutDescriptor;
		quadLayoutDescriptor.bindGroupLayoutCount = 1;
		quadLayoutDescriptor.bindGroupLayouts = (WGPUBindGroupLayout*)&_bindLayout;
		quadLayoutDescriptor.label = "Quad layout";
		wgpu::PipelineLayout quadPipelineLayout = device.createPipelineLayout(quadLayoutDescriptor);

		wgpu::RenderPipelineDescriptor quadPipelineDesc;
		quadPipelineDesc.layout = quadPipelineLayout;
		quadPipelineDesc.depthStencil = &depthStencil;
		quadPipelineDesc.vertex = vertexState;
		quadPipelineDesc.fragment = &fragmentState;

		quadPipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
		quadPipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
		quadPipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
		quadPipelineDesc.primitive.cullMode = wgpu::CullMode::None;

		quadPipelineDesc.multisample.count = 1;
		quadPipelineDesc.multisample.mask = ~0u; //all bits on
		quadPipelineDesc.multisample.alphaToCoverageEnabled = false;

		quadPipelineDesc.label = "Quad Pipeline";
		_pipeline = device.createRenderPipeline(quadPipelineDesc);
	}

	QuadRenderPipeline::~QuadRenderPipeline()
	{
		_bindGroup.release();
		_bindLayout.release();
		_sampler.release();
		_pipeline.release();
	}

	void QuadRenderPipeline::BindData(Gfx::Buffer const& transformData, Gfx::Texture const& texture, Gfx::Buffer const& cameraData, wgpu::Device device)
	{
		wgpu::BindGroupEntry& quadUniformBind = _bindEntries[0];
		quadUniformBind.binding = 0;
		quadUniformBind.buffer = transformData.Get();
		quadUniformBind.offset = 0;
		quadUniformBind.size = sizeof(QuadTransform);

		wgpu::BindGroupEntry& quadTextureBind = _bindEntries[1];
		quadTextureBind.binding = 1;
		quadTextureBind.textureView = texture.View();

		wgpu::BindGroupEntry& quadSamplerBind = _bindEntries[2];
		quadSamplerBind.binding = 2;
		quadSamplerBind.sampler = _sampler;

		wgpu::BindGroupEntry& quadCamBind = _bindEntries[3];
		quadCamBind.binding = 3;
		quadCamBind.buffer = cameraData.Get();
		quadCamBind.offset = 0;
		quadCamBind.size = sizeof(CamUniforms);

		wgpu::BindGroupDescriptor bindingDesc{};
		bindingDesc.layout = _bindLayout;
		bindingDesc.entryCount = k_QuadPipelineBindingCount;
		bindingDesc.entries = _bindEntries.data();
		_bindGroup = device.createBindGroup(bindingDesc);
	}
}