#pragma once
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include "webgpu.h"
#include "ObjLoader.h"
#include "MeshDefs.h"
#include "ImageLoader.h"

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
	uint32_t width;
	uint32_t height;
	uint8_t channelDepthBytes;
	uint8_t numChannels;
	std::vector<std::byte> data;
	std::string label;

	uint32_t SizeBytes() { return numChannels * width * height * channelDepthBytes; }
};

namespace Utils
{
	std::optional<Object> LoadGeometry(std::filesystem::path const& path);
	std::optional<TextureResource> LoadTexture(std::filesystem::path const& path);
	std::vector<TextureResource> LoadAnimation(std::filesystem::path const& folderPath);
	std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device);
}