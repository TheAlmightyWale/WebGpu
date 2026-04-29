// Defines the entry point for the application.

#include <iostream>
#include <glfw3webgpu.h>
#include <array>
#include <functional>

#include "Core/webgpu.h"
#include "Core/Utils.h"
#include "CameraDefs.h"
#include "Core/MathDefs.h"
#include "Terrain.h"
#include "Core/Chrono.h"
#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "GlobalBuffers.h"
#include "Renderer.h"
#include "DebugAtlasViewer.h"

uint32_t CeilToNextMultiple(uint32_t value, uint32_t multiple)
{
	uint32_t divideAndCeil = value / multiple + (value % multiple == 0 ? 0 : 1);
	return multiple * divideAndCeil;
}

struct InputCtx{
	Gfx::Debug::DebugAtlasViewer* pAtlasViewer;
};
static InputCtx g_inputCtx;

void KeyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
	//if key is 'X'
	if(key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		static bool bVis = true;
		bVis = !bVis;
		g_inputCtx.pAtlasViewer->SetVisibility(bVis);
	}
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
	float const k_atlasViewerSize = 500.0f;
	Gfx::Debug::DebugAtlasViewer atlasViewer(k_atlasViewerSize, k_atlasWidth, k_atlasHeight);

	//input for viewing and hiding debug atlas
	g_inputCtx.pAtlasViewer = &atlasViewer;
	glfwSetKeyCallback(window.get(), KeyCallback);

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
