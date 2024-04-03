#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include "webgpu.h"

struct Object
{
	std::vector<float> points;
	std::vector<uint16_t> indices;
};

namespace Utils
{
	std::optional<Object> LoadGeometry(std::filesystem::path const& path, int dimensions)
	{
		std::ifstream file(path);
		if (!file.is_open()) return std::nullopt;

		Object res;

		enum class Section {
			None,
			Points,
			Indices
		};

		Section currentSection = Section::None;

		float value;
		uint16_t index;
		std::string line;

		while (!file.eof())
		{
			getline(file, line);

			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			if (line == "[points]") {
				currentSection = Section::Points;
			}
			else if (line == "[indices]")
			{
				currentSection = Section::Indices;
			}
			else if (line[0] == '#' || line.empty()) {
				// Do nothing, this is a comment
			}
			else if (currentSection == Section::Points) {
				std::istringstream iss(line);
				// Get x, y, z, r, g, b
				for (int i = 0; i < dimensions + 3/*3 channl color*/; ++i) {
					iss >> value;
					res.points.push_back(value);
				}
			}
			else if (currentSection == Section::Indices) {
				std::istringstream iss(line);
				// Get corners #0 #1 and #2
				for (int i = 0; i < 3; ++i) {
					iss >> index;
					res.indices.push_back(index);
				}
			}
		}

		return res;
	}

	std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
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