#include "Utils.h"
#include <filesystem>
#include "ObjLoader.h"
#include "ImageLoader.h"
#include <fstream>
#include <string.h> //memcpy
#include "MathDefs.h"

namespace
{

	std::optional<Gfx::Object> LoadGeometryObj_(std::filesystem::path const& path)
	{
		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(path.string()))
		{
			if (!reader.Error().empty())
				std::cout << "TinyObjReader: " << reader.Error() << std::endl;
			return std::nullopt;
		}

		if (!reader.Warning().empty())
			std::cout << "TinyObjReader: " << reader.Warning() << "\n";

		auto const& attrib = reader.GetAttrib();
		auto const& shapes = reader.GetShapes();

		Gfx::Object result;
		// Loop over shapes
		for (size_t shapeIndex = 0; shapeIndex < shapes.size(); shapeIndex++)
		{
			Gfx::Shape shape;
			size_t indexOffset = 0;
			// Loop over faces
			for (size_t faceIndex = 0; faceIndex < shapes[shapeIndex].mesh.num_face_vertices.size(); faceIndex++)
			{
				size_t numVerticesInFace = (size_t)(shapes[shapeIndex].mesh.num_face_vertices[faceIndex]);

				// Loop over vertices in the face
				for (size_t vertexIndex = 0; vertexIndex < numVerticesInFace; vertexIndex++)
				{
					tinyobj::index_t idx = shapes[shapeIndex].mesh.indices[indexOffset + vertexIndex];

					InterleavedVertex vertex;
					vertex.x = attrib.vertices[3 * (size_t)(idx.vertex_index) + 0];
					vertex.y = -attrib.vertices[3 * (size_t)(idx.vertex_index) + 2];
					vertex.z = attrib.vertices[3 * (size_t)(idx.vertex_index) + 1]; // obj file format specifies +Y as up, we use +Z

					if (idx.normal_index >= 0)
					{
						vertex.nx = attrib.normals[3 * (size_t)(idx.normal_index) + 0];
						vertex.ny = -attrib.normals[3 * (size_t)(idx.normal_index) + 2]; // obj file format specifies +Y as up, we use +Z
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

} // Anonymous namespace

namespace Gfx
{

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
			std::cout << "Failed to Load Texture Resource at" << path << "\n"
					  << "Reason: " << stbi_failure_reason() << "\n";
			return std::nullopt;
		}

		stbi_uc* pData = stbi_load(path.generic_string().c_str(), &x, &y, &n, 0);
		res.height = (uint32_t)y;
		res.width = (uint32_t)x;
		res.channelDepthBytes = 1; // stbi uses 8bit channels
		res.numChannels = (uint8_t)n;
		uint32_t dataSizeBytes = (uint32_t)(x * y * n);
		res.data.resize(dataSizeBytes);
		res.label = path.stem().string();
		std::copy((std::byte*)pData, (std::byte*)pData + dataSizeBytes, res.data.data());

		stbi_image_free(pData);

		return res;
	}

	// Copy a contiguous set of data into a "square" of data
	void CopyIntoRect(
		std::byte* pDestination,
		uint32_t destinationBytesPerRow,
		uint32_t columnOffsetBytes,
		std::byte const* pSource,
		uint32_t sourceBytesPerRow,
		uint32_t bytesToCopy)
	{
		uint32_t numRows = bytesToCopy / sourceBytesPerRow;
		for (uint32_t row = 0; row < numRows; ++row)
		{
			size_t dstOffset = columnOffsetBytes + row * destinationBytesPerRow;
			size_t srcOffset = row * sourceBytesPerRow;
			memcpy(pDestination + dstOffset, pSource + srcOffset, sourceBytesPerRow);
		}
	}

	//it is assumed offsets do not take in to account the stride in bytes
	// i.e. if a texture has 4 bytes per pixel we assume offset is in pixel count, not total bytes
	// if it is in byte count then strideBytes must be set to 1
	void Blit(
		std::byte* pDestination,
		Vec2u destinationOffset,
		uint32_t destinationRowSize,
		std::byte const* pSource,
		uint32_t sourceRowSize,
		Rect sourceLocation,
		uint32_t strideBytes)
	{
		//verify we can actually fit source row into remaining destination row (with offset)
		assert(destinationRowSize >= sourceLocation.pos.x + sourceLocation.dims.x);
		
		//move destination pointer to it's offset
		uint32_t destOffset = ((destinationOffset.y * destinationRowSize) + destinationOffset.x) * strideBytes;
		pDestination += destOffset;

		//move source pointer to it's offset
		uint32_t srcOffset = ((sourceLocation.pos.y * sourceRowSize) + sourceLocation.pos.x) * strideBytes;
		pSource += srcOffset;

		//copy row by row
		for(uint32_t i = 0; i < sourceLocation.dims.y; i++)
		{
			//copy row of memory
			memcpy(pDestination, pSource, sourceLocation.dims.x * strideBytes);
			//advance dest by destRow
			pDestination += destinationRowSize * strideBytes;
			//advance src by srcRow	
			pSource += sourceRowSize * strideBytes;
		}
	}

	std::optional<std::pair<Animation, TextureResource>> LoadAnimation(std::filesystem::path const& folderPath)
	{
		std::vector<TextureResource> textures;

		for (auto const& directoryEntry : std::filesystem::directory_iterator(folderPath))
		{
			auto path = directoryEntry.path();
			if (!path.has_filename())
				continue;

			if (path.extension() == ".png")
			{
				auto oTexture = LoadTexture(path);
				if (oTexture)
					textures.push_back(std::move(*oTexture));
			}
		}

		if (textures.empty())
		{
			std::cout << "Failed to load Animation: " << folderPath << "\n";
			return std::nullopt;
		}

		// Pack animation into a single row for now
		// images expected to be in format <name>_<frameId>
		// sort by frame number
		std::sort(textures.begin(), textures.end(), [](TextureResource const& l, TextureResource const& r)
				  {
			size_t lPos = l.label.find('_') + 1; // want everything after _
			std::string lFrame = l.label.substr(lPos);
			int lFrameNum = atoi(lFrame.c_str());

			size_t rPos = r.label.find('_') + 1; //want everything after _
			std::string rFrame = r.label.substr(rPos); 
			int rFrameNum = atoi(rFrame.c_str());

			return lFrameNum < rFrameNum; });

		std::string animationName = textures[0].label.substr(0, textures[0].label.find('_'));

		Animation animation;
		animation.startX = 0;
		animation.startY = 0;
		animation.length = (uint32_t)textures.size();
		animation.frameRes.width = (uint16_t)textures[0].width;
		animation.frameRes.height = (uint16_t)textures[0].height;
		animation.name = animationName;

		TextureResource textureResource{
			textures[0].width * (uint32_t)textures.size(),
			textures[0].height,
			textures[0].channelDepthBytes,
			textures[0].numChannels,
			{} /*data*/,
			animationName};

		textureResource.data.resize(textureResource.SizeBytes());

		// Copy frames into strip
		size_t imageColumnOffset = 0;
		size_t totalRowBytes = textures[0].width * textures[0].channelDepthBytes * textures[0].numChannels * textures.size();
		for (uint32_t i = 0; i < textures.size(); ++i)
		{
			auto const& frame = textures[i];
			uint32_t imageRowBytes = frame.width * frame.channelDepthBytes * frame.numChannels;

			CopyIntoRect(
				textureResource.data.data(),
				static_cast<uint32_t>(totalRowBytes),
				static_cast<uint32_t>(imageColumnOffset),
				frame.data.data(),
				imageRowBytes,
				frame.SizeBytes());

			imageColumnOffset += imageRowBytes;
		}

		std::cout << "Loaded Animation at: " << folderPath << "\n";

		return {{animation, textureResource}};
	}

	//Rectangle packs textures. Right now we just stack textures on top of each other
	std::optional<PackResult> PackTextures(std::string const& atlasName, std::vector<TextureResource*> const& textures){
		if(textures.empty()){
			std::cout << "Attempting to pack an empty list of textures" << std::endl;
			return std::nullopt;
		}
		
		PackResult result;

		uint16_t maxWidth = 0;
		uint16_t totalHeight = 0;
		
		std::for_each(textures.begin(), textures.end(), [&maxWidth, &totalHeight](TextureResource const* const tex){
			assert(tex->width < std::numeric_limits<uint16_t>::max());
			assert((uint32_t)totalHeight + tex->height < std::numeric_limits<uint16_t>::max());

			maxWidth = std::max(maxWidth, (uint16_t)tex->width);
			totalHeight += (uint16_t)tex->height; 
		});

		TextureResource atlas{
			maxWidth,
			totalHeight,
			textures[0]->channelDepthBytes,
			textures[0]->numChannels,
			{},
			atlasName
		};

		atlas.data.resize(atlas.SizeBytes());

		uint16_t yPos = 0;
		uint16_t xPos = 0;
		
		for(auto const& tex : textures)
		{
			StartLocation loc;
			loc.x = xPos;
			loc.y = yPos;

			//copy individual texture into atlas
			CopyIntoRect(atlas.data.data(),
				atlas.width,
				xPos,
				tex->data.data(),
				tex->width,
				(uint32_t)tex->data.size() 
			);

			//insert result
			result.labelledStartLocations.insert({tex->label, loc});

			//update start positions
			yPos += (uint16_t)tex->height;
		}

		result.textureAtlas = atlas;

		return result;
	}
}
}