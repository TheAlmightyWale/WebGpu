#include "Utils.h"
#include <filesystem>
#include "ObjLoader.h"
#include "ImageLoader.h"
#include "fstream"
#include <string.h> //memcpy

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

	//Copy a contiguous set of data into a "square" of data
	void CopyIntoSquare(
		std::byte* pDestination,
		uint32_t destinationBytesPerRow,
		uint32_t columnOffsetBytes,
		std::byte const* pSource,
		uint32_t sourceBytesPerRow,
		uint32_t bytesToCopy)
	{
		uint32_t numRows = bytesToCopy / sourceBytesPerRow;
		for(uint32_t row = 0; row < numRows; ++row)
		{
			size_t dstOffset = columnOffsetBytes + row * destinationBytesPerRow;
			size_t srcOffset = row * sourceBytesPerRow;
			memcpy(pDestination + dstOffset, pSource + srcOffset, sourceBytesPerRow);
		}
	}

	std::optional<TextureResource> LoadAnimationTexture(std::filesystem::path const& folderPath)
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

		if (animation.empty()) {
			std::cout << "Failed to load Animation: " << folderPath << "\n";
			return std::nullopt;
		}

		//Pack animation into a single row for now
		//images expected to be in format <name>_<frameId>
		//sort by frame number
		std::sort(animation.begin(), animation.end(), [](TextureResource const& l, TextureResource const& r) {
			size_t lPos = l.label.find('_') + 1; // want everything after _
			std::string lFrame = l.label.substr(lPos);
			int lFrameNum = atoi(lFrame.c_str());

			size_t rPos = r.label.find('_') + 1; //want everything after _
			std::string rFrame = r.label.substr(rPos); 
			int rFrameNum = atoi(rFrame.c_str());

			return lFrameNum < rFrameNum;
		});

		std::string animationName = animation[0].label.substr(0,animation[0].label.find('_'));

		TextureResource animationStrip{
			animation[0].width * (uint32_t)animation.size(),
			animation[0].height,
			animation[0].channelDepthBytes,
			animation[0].numChannels,
			{} /*data*/,
			animationName
		};

		animationStrip.data.resize(animationStrip.SizeBytes());

		//Copy frames into strip
		size_t imageColumnOffset = 0;
		size_t totalRowBytes = animation[0].width * animation[0].channelDepthBytes * animation[0].numChannels * animation.size();
		for (uint32_t i = 0; i < animation.size(); ++i) {
			auto const& frame = animation[i];
			uint32_t imageRowBytes = frame.width * frame.channelDepthBytes * frame.numChannels;

			CopyIntoSquare(
				animationStrip.data.data(),
				static_cast<uint32_t>(totalRowBytes),
				static_cast<uint32_t>(imageColumnOffset),
				frame.data.data(),
				imageRowBytes,
				frame.SizeBytes());

			imageColumnOffset += imageRowBytes;
		}

		std::cout << "Loaded Animation at: " << folderPath << "\n";

		return animationStrip;
	}

	std::optional<wgpu::ShaderModule> LoadShaderModule(std::filesystem::path const& path, wgpu::Device device)
	{
		std::cout << "Attempting to load shader module: " << path << "\n";
		std::ifstream file(path);
		if (file.fail()) {
			std::cerr << "Error Loading Shader: " << strerror(errno) << "\n";
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