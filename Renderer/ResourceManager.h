#pragma once
#include <filesystem>
#include <unordered_map>
#include <cstdint>
#include <string>
#include "ResourceDefs.h"

//Loads and holds memory of all resources in the file paths given
class ResourceManager {
public:
	using AnimationMapKey = std::string;
	using AnimationMap = std::unordered_map<AnimationMapKey, TextureResource>;

	//recurses through the folder given loading animations in sub folders
	void LoadAllAnimations(std::filesystem::path const& parentFolder);

	TextureResource const& GetAnimation(AnimationMapKey id) const noexcept;

	AnimationMap const& GetAllAnimations() const noexcept;

private:
	AnimationMap m_anims;
};