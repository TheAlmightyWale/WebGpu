#pragma once
#include <filesystem>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <array>
#include "ResourceDefs.h"
#include "TextureAtlas.h"

namespace Gfx
{

uint8_t const k_maxAtlas = 5;

//Loads and holds memory of all resources in the file paths given
class ResourceManager {
public:
	using AnimationMapKey = std::string;
	using AnimationMap = std::unordered_map<AnimationMapKey, AnimationSet>;
	using AtlasArray = std::array<TextureAtlas, k_maxAtlas>;

	ResourceManager(uint32_t atlasWidth, uint32_t atlasHeight);

	//recurses through the folder given loading animations in sub folders
	std::vector<ResourceManager::AnimationMapKey> LoadAllAnimations(std::filesystem::path const& parentFolder);

	//load up a .json describing a set of animations
	AnimationMapKey LoadAnimationSet(std::filesystem::path const& animationDescriptorPath);

	AnimationSet const& GetAnimation(AnimationMapKey id) const noexcept;

	AnimationMap const& GetAllAnimations() const noexcept;

private:
	AnimationMap m_anims;
	AtlasArray m_atlas;
};

}