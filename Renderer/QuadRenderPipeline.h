#pragma once
#include <array>
#include "Core/webgpu.h"
#include "Core/MathDefs.h"
#include "GpuBuffer.h"
#include "GpuTexture.h"

namespace Gfx
{
	constexpr uint32_t k_QuadPipelineBindingCount = 5;

	class QuadRenderPipeline {
	public:
		QuadRenderPipeline(wgpu::Device device, wgpu::ShaderModule shaders, wgpu::ColorTargetState outputTarget, wgpu::DepthStencilState depthStencil);
		~QuadRenderPipeline();

		void BindData(Gfx::GpuBuffer const& transformData, Gfx::GpuTexture const& texture,
			Gfx::GpuBuffer const& cameraData, Gfx::GpuBuffer const& animationData, wgpu::Device device);

		inline wgpu::RenderPipeline Get() const noexcept {
			return _pipeline;
		};

		inline wgpu::BindGroup BindGroup() const noexcept {
			return _bindGroup;
		};

	private:
		//No copy, move
		QuadRenderPipeline(QuadRenderPipeline const& other) = delete;
		QuadRenderPipeline(QuadRenderPipeline&& other) = delete;
		QuadRenderPipeline& operator=(QuadRenderPipeline const& other) = delete;
		QuadRenderPipeline& operator=(QuadRenderPipeline&& other) = delete;

		wgpu::RenderPipeline _pipeline;
		std::array<wgpu::BindGroupEntry, k_QuadPipelineBindingCount> _bindEntries;
		std::array<wgpu::BindGroupLayoutEntry, k_QuadPipelineBindingCount> _bindLayouts;
		wgpu::BindGroupLayout _bindLayout;
		wgpu::BindGroup _bindGroup;
		wgpu::Sampler _sampler;
	};
}