#pragma once
#include <vector>
#include <limits>
#include <cstdint>
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

struct Resolution
{
	uint16_t width;
	uint16_t height;
};

struct Animation
{
	uint32_t startX;
	uint32_t startY;
	uint32_t length;
	Resolution frameRes;
	std::string name;
	TextureResource texture;
};

uint8_t const k_invalidAtlasId = std::numeric_limits<uint8_t>::max();

struct AnimationSet
{
	std::string name;
	std::vector<Animation> animations;
	uint8_t fps;
	uint8_t atlasId = k_invalidAtlasId;
};

// Data expected to be in Json files describing a set of animations to be loaded
struct AnimationDescriptor
{
	std::string name;
	std::vector<std::string> animationDirectories;
	uint32_t fps;
};