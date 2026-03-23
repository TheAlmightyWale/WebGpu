#pragma once
#include "Core/webgpu.h"
#include <optional>
#include <filesystem>

namespace Gfx
{
    std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device);
}