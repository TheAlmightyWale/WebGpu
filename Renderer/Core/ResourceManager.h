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
	using AnimationMap = std::unordered_map<AnimationMapKey, AnimationSet>;

	//recurses through the folder given loading animations in sub folders
	std::vector<ResourceManager::AnimationMapKey> LoadAllAnimations(std::filesystem::path const& parentFolder);

	//load up a .json describing a set of animations
	AnimationMapKey LoadAnimationSet(std::filesystem::path const& animationDescriptorPath);

	AnimationSet const& GetAnimation(AnimationMapKey id) const noexcept;

	AnimationMap const& GetAllAnimations() const noexcept;

private:
	AnimationMap m_anims;
};