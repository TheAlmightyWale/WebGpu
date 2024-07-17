#pragma once
#include <vector>
#include "MeshDefs.h"
#include "webgpu.h"


struct Shape
{
	std::vector<InterleavedVertex> points;
};

struct Object
{
	std::vector<Shape> shapes;
};

struct TextureResource
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t channelDepthBytes = 0;
	uint8_t numChannels = 0;
	std::vector<std::byte> data;
	std::string label = "Undefined";

	uint32_t SizeBytes() const noexcept{ return numChannels * width * height * channelDepthBytes; }
	wgpu::Extent3D Extents() const noexcept { return { width, height, 1 }; }
};