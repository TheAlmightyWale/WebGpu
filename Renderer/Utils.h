#pragma once
#include <optional>
#include <filesystem>
#include "webgpu.h"

#include "ResourceDefs.h"

namespace Utils
{
	std::optional<Object> LoadGeometry(std::filesystem::path const& path);
	std::optional<TextureResource> LoadTexture(std::filesystem::path const& path);
	std::optional<std::pair<Animation,TextureResource>> LoadAnimation(std::filesystem::path const& folderPath);
	std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device);

	struct StartLocation
	{
		uint16_t x = 0;
		uint16_t y = 0;
	};

	struct PackResult
	{
		std::unordered_map<std::string, StartLocation> labelledStartLocations;
		TextureResource textureAtlas; 
	};

	std::optional<PackResult> PackTextures(std::string const& atlasName, std::vector<TextureResource*> const& textures);
}