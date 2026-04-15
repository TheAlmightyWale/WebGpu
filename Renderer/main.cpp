// Defines the entry point for the application.

#include <iostream>
#include <glfw3webgpu.h>
#include <array>

#include "Core/webgpu.h"
#include "Core/Utils.h"
#include "CameraDefs.h"
#include "Core/MathDefs.h"
#include "GpuBuffer.h"
#include "GpuTexture.h"
#include "Terrain.h"
#include "Core/Chrono.h"
#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "GlobalBuffers.h"
#include "Renderer.h"

uint32_t CeilToNextMultiple(uint32_t value, uint32_t multiple)
{
	uint32_t divideAndCeil = value / multiple + (value % multiple == 0 ? 0 : 1);
	return multiple * divideAndCeil;
}

int main()
{
	Gfx::Window window;
	Gfx::Renderer renderer;
	renderer.Initialize(window);

	// Quad cam uniforms, to be updated when swap chain is resized
	std::span<Gfx::CamUniform> quadCams = Gfx::g_cameraUniformBuffer.RegisterData(1);
	Gfx::CamUniform& quadCam = quadCams[0];
	quadCam.position = Vec2f{0.5f, 0.5f};
	quadCam.extents = Vec2f{(float)Gfx::k_screenWidth, (float)Gfx::k_screenHeight};

	// Temp animation load
	uint32_t const k_atlasWidth = renderer.GetContext().GetDeviceLimits().limits.maxTextureDimension1D;
	uint32_t const k_atlasHeight = renderer.GetContext().GetDeviceLimits().limits.maxTextureDimension2D;
	Gfx::ResourceManager resources(k_atlasWidth, k_atlasHeight);
	resources.LoadAllAnimations(ASSETS_DIR);

	// Upload all texture atlases
	for (uint8_t i = 0; i < Gfx::k_maxAtlas; i++)
	{
		auto const& atlas = resources.GetAtlas(i);
		renderer.UploadTextureAtlas(atlas, i);
	}

	Gfx::Terrain terrain(10, 10, 50, resources);
	std::span<Gfx::QuadTransform> debugAtlas = Gfx::g_transformBuffer.RegisterData(Gfx::k_maxAtlas);
	std::span<Gfx::AnimUniform> debugAtlasAnims = Gfx::g_animationBuffer.RegisterData(Gfx::k_maxAtlas);

	// Fill in debug atlas data
	float k_debugAtlasSize = 500.f;
	for (uint32_t i = 0; i < Gfx::k_maxAtlas; i++)
	{
		debugAtlas[i] = Gfx::QuadTransform{
			Vec3f(-10.f - k_debugAtlasSize, (i * -5.0f) - (i * k_debugAtlasSize), 0.0f),
			0.0f, // pad
			Vec2f(k_debugAtlasSize, k_debugAtlasSize)};

		// cover entierty of atlas
		debugAtlasAnims[i] = Gfx::AnimUniform{
			Vec2f(0.f, 0.f),
			Vec2f(k_atlasWidth, k_atlasHeight),
			0,
			i};
	}

	while (!window.ShouldClose())
	{
		Clock::Tick();
		float deltaTime = Clock::GetDelta();

		// Update animations
		terrain.Animate(deltaTime);

		renderer.Render();
	}
	return 0;
}
