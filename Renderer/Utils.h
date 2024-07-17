#pragma once
#include <optional>
#include <filesystem>
#include "webgpu.h"

#include "ResourceDefs.h"

namespace Utils
{
	std::optional<Object> LoadGeometry(std::filesystem::path const& path);
	std::optional<TextureResource> LoadTexture(std::filesystem::path const& path);
	std::optional<TextureResource> LoadAnimation(std::filesystem::path const& folderPath);
	std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device);
}