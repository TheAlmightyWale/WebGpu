#include "Utils.h"

namespace
{

	std::optional<Object> LoadGeometryObj_(std::filesystem::path const& path)
	{
		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(path.string()))
		{
			if (!reader.Error().empty())	std::cout << "TinyObjReader: " << reader.Error() << std::endl;
			return std::nullopt;
		}

		if (!reader.Warning().empty()) std::cout << "TinyObjReader: " << reader.Warning() << "\n";

		auto const& attrib = reader.GetAttrib();
		auto const& shapes = reader.GetShapes();
		//auto const& materials = reader.GetMaterials();

		size_t numIndices = 0;
		for (auto const& shape : shapes)
		{
			numIndices += shape.mesh.indices.size();
		}

		Object result;
		//Loop over shapes
		for (size_t shapeIndex = 0; shapeIndex < shapes.size(); shapeIndex++) {
			Shape shape;
			size_t indexOffset = 0;
			//Loop over faces
			for (size_t faceIndex = 0; faceIndex < shapes[shapeIndex].mesh.num_face_vertices.size(); faceIndex++) {
				size_t numVerticesInFace = (size_t)(shapes[shapeIndex].mesh.num_face_vertices[faceIndex]);

				//Loop over vertices in the face
				for (size_t vertexIndex = 0; vertexIndex < numVerticesInFace; vertexIndex++) {
					tinyobj::index_t idx = shapes[shapeIndex].mesh.indices[indexOffset + vertexIndex];

					InterleavedVertex vertex;
					vertex.x = attrib.vertices[3 * (size_t)(idx.vertex_index) + 0];
					vertex.y = -attrib.vertices[3 * (size_t)(idx.vertex_index) + 2];
					vertex.z = attrib.vertices[3 * (size_t)(idx.vertex_index) + 1]; //obj file format specifies +Y as up, we use +Z

					if (idx.normal_index >= 0) {
						vertex.nx = attrib.normals[3 * (size_t)(idx.normal_index) + 0];
						vertex.ny = -attrib.normals[3 * (size_t)(idx.normal_index) + 2]; //obj file format specifies +Y as up, we use +Z
						vertex.nz = attrib.normals[3 * (size_t)(idx.normal_index) + 1];
					}

					vertex.r = attrib.colors[3 * (size_t)(idx.vertex_index) + 0];
					vertex.g = attrib.colors[3 * (size_t)(idx.vertex_index) + 1];
					vertex.b = attrib.colors[3 * (size_t)(idx.vertex_index) + 2];
					shape.points.push_back(vertex);
				}
				indexOffset += numVerticesInFace;
			}

			result.shapes.push_back(shape);
		}

		return result;
	}

}//Anonymous namespace

namespace Utils
{
	std::optional<Object> LoadGeometry(std::filesystem::path const& path)
	{
		if (path.extension() == ".obj")
		{
			return LoadGeometryObj_(path);
		}
		else
		{
			std::cout << "Unhandled file type: " << path << std::endl;
			return std::nullopt;
		}
	}

	std::optional<TextureResource> LoadTexture(std::filesystem::path const& path)
	{
		TextureResource res;
		int x, y, n, ok;
		ok = stbi_info(path.generic_string().c_str(), &x, &y, &n);

		if (!ok)
		{
			std::cout << "Failed to Load Texture Resource at" << path << "\n" << "Reason: " << stbi_failure_reason() << "\n";
			return std::nullopt;
		}

		stbi_uc* pData = stbi_load(path.generic_string().c_str(), &x, &y, &n, 0);
		res.height = (uint32_t)y;
		res.width = (uint32_t)x;
		res.channelDepthBytes = 1;// stbi uses 8bit channels
		res.numChannels = (uint8_t)n;
		uint32_t dataSizeBytes = (uint32_t)(x * y * n);
		res.data.resize(dataSizeBytes);
		res.label = path.stem().string();
		std::copy((std::byte*)pData, (std::byte*)pData + dataSizeBytes, res.data.data());

		stbi_image_free(pData);

		return res;
	}

	std::vector<TextureResource> LoadAnimation(std::filesystem::path const& folderPath)
	{
		std::vector<TextureResource> animation;

		for (auto const& directoryEntry : std::filesystem::directory_iterator(folderPath))
		{
			auto path = directoryEntry.path();
			if (!path.has_filename()) continue;

			if (path.extension() == ".png")
			{
				auto oTexture = LoadTexture(path);
				if (oTexture) animation.push_back(std::move(*oTexture));
			}
		}

		return animation;
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