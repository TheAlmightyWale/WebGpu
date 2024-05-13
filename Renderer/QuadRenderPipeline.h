#pragma once
#include <array>
#include "webgpu.h"
#include "MathDefs.h"
#include "Buffer.h"
#include "Texture.h"

namespace Gfx
{
	constexpr uint32_t k_QuadPipelineBindingCount = 4;

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

	class QuadRenderPipeline {
	public:
		QuadRenderPipeline(wgpu::Device device, wgpu::ShaderModule shaders, wgpu::ColorTargetState outputTarget, wgpu::DepthStencilState depthStencil);
		~QuadRenderPipeline();

		void BindData(Gfx::Buffer const& transformData, Gfx::Texture const& texture, Gfx::Buffer const& cameraData, wgpu::Device device);

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