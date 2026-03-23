#include "ResourceManager.h"
#include "Utils.h"

#include "glaze/glaze.hpp"

namespace Detail
{
	std::optional<std::filesystem::path> FindDescriptorFile(std::filesystem::path directory)
	{
		for(auto const& entry : std::filesystem::directory_iterator{directory})
		{
			if(entry.is_regular_file())
			{
				std::filesystem::path path{entry};
				if(path.extension() == ".json")
				{
					//return first one found
					std::cout << "Found animation descriptor at " << path << std::endl;
					return path;
				} 
			}
		}

		return std::nullopt;
	}
}

std::vector<ResourceManager::AnimationMapKey> ResourceManager::LoadAllAnimations(std::filesystem::path const& parentFolder)
{
	std::cout << "Attempting to load Animations in " << parentFolder << "\n";
	std::vector<ResourceManager::AnimationMapKey> animationsLoaded;
	for (auto const& dir : std::filesystem::directory_iterator{ parentFolder }) {
		if (dir.is_directory()) {
			//find json descriptor in directory
			//then load animationSet described by that
			std::optional<std::filesystem::path> animationDescriptorFile = Detail::FindDescriptorFile(dir.path().lexically_normal());
			if(!animationDescriptorFile){
				std::cout << "Could not find descriptor file in " << dir << std::endl;
				continue;
			}

			animationsLoaded.push_back(LoadAnimationSet(*animationDescriptorFile));
		}
	}

	return animationsLoaded;
}

ResourceManager::AnimationMapKey ResourceManager::LoadAnimationSet(std::filesystem::path const& animationDescriptorPath)
{
	std::cout << "Loading animations defined at " << animationDescriptorPath << std::endl; 
	//load and parse in to AnimationSet struct
	AnimationDescriptor ad;
	auto readError = glz::read_file_json(ad, animationDescriptorPath.string(), std::string{});

	if (readError) {
       std::string error_msg = glz::format_error(readError);
       std::cout << error_msg << std::endl;
	   return "";
    }

	AnimationSet animationSet;
	animationSet.name = ad.name;
	assert(ad.fps < std::numeric_limits<uint8_t>::max());
	animationSet.fps = (uint8_t)ad.fps;

	std::vector<TextureResource> textures;

	//Go through described animations and load them
	for(auto const& animationDir : ad.animationDirectories)
	{
		//does directory with this path exist below descriptor path?
		std::filesystem::path dir = animationDescriptorPath.parent_path();
		dir /= animationDir;

		if(!std::filesystem::exists(dir)){
			std::cout << "Could not find directory at " << dir << " skipping" << std::endl;
			continue;
		}

		//if so, load animation inside of it
		auto oAnim = Utils::LoadAnimation(dir);
		if(!oAnim){
			std::cout << "failed to load anim at " << dir << std::endl;
			continue;
		}

		auto [animation, texture] = *oAnim;

		//add to animationSet
		animationSet.animations.push_back(animation);
		textures.push_back(texture);
	}

	//compose texture atlas
	std::vector<TextureResource*> texturePointers;
	for(auto& tex : textures){
		texturePointers.push_back(&tex);
	}
	auto oPackResult = Utils::PackTextures(animationSet.name + "_atlas", texturePointers);

	if(!oPackResult){
		std::cout << "failed to pack " << animationSet.name << std::endl;
		return "";
	}

	//We assume order of animation and order of start locations are the same
	Utils::PackResult packResult = *oPackResult;
	for(auto& animation : animationSet.animations){
		Utils::StartLocation loc = packResult.labelledStartLocations[animation.name];
		animation.startX = loc.x;
		animation.startY = loc.y;
	}

	//update animation set with any remaining details
	animationSet.animTexture = packResult.textureAtlas;

	//place in to resource manager
	m_anims.insert({animationSet.name, animationSet});

	//return resource manager key
	return animationSet.name;

}

AnimationSet const& ResourceManager::GetAnimation(AnimationMapKey id) const noexcept
{
	return m_anims.at(id);
}

ResourceManager::AnimationMap const& ResourceManager::GetAllAnimations() const noexcept
{
	return m_anims;
}



