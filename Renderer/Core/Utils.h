#pragma once
#include <optional>
#include <filesystem>

#include "ResourceDefs.h"
#include "MathDefs.h"

namespace Gfx
{

namespace Utils
{
	std::optional<TextureResource> LoadTexture(std::filesystem::path const& path);
	std::optional<std::pair<Animation, TextureResource>> LoadAnimation(std::filesystem::path const& folderPath);

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

	void Blit(std::byte* pDestination,
		Vec2u destinationOffset,
		uint32_t destinationRowSize,
		std::byte const* pSource,
		uint32_t sourceRowSize,
		Rect sourceLocation,
		uint32_t strideBytes);

	//Fills a buffer with a repeating pattern defined by pSrc. Will truncate if sizes do not fully fit
	void FastFill(std::byte* pDestination, uint32_t destinationSizeBytes, std::byte const* pSrc, uint32_t sourceSizeBytes);
}

}