#include "ShaderLoader.h"
#include "Core/webgpu.h"
#include <iostream>
#include <fstream>

namespace Gfx
{
    std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device)
	{
		std::cout << "Attempting to load shader module: " << path << "\n";
		std::ifstream file(path);
		if (file.fail())
		{
			std::string errMsg;
			errMsg.reserve(256);
			strerror_s(errMsg.data(), errMsg.capacity(), errno);
			std::cerr << "Error Loading Shader: " << errMsg << "\n";
			return std::nullopt;
		}

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::string shaderSource(size, ' ');
		file.seekg(0);

		file.read(shaderSource.data(), size);

		wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
		wgslDesc.chain.next = nullptr;
		wgslDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
		wgslDesc.code = shaderSource.c_str();

		wgpu::ShaderModuleDescriptor desc{};
		desc.hintCount = 0;
		desc.hints = nullptr;
		desc.nextInChain = &wgslDesc.chain;

		wgpu::ShaderModule module = device.createShaderModule(desc);
		if (module)
		{
			return module;
		}
		else
		{
			return std::nullopt;
		}
	}
}