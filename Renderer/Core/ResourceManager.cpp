#include "ResourceManager.h"
#include "Utils.h"

#include "glaze/glaze.hpp"

namespace Gfx
{

namespace Detail
{
	std::optional<std::filesystem::path> FindDescriptorFile_(std::filesystem::path directory)
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

	ResourceManager::AtlasArray GenerateAtlasArray_(uint32_t atlasWidth, uint32_t atlasHeight)
	{
		return {
			TextureAtlas(atlasWidth, atlasHeight, "Atlas_0"),
			TextureAtlas(atlasWidth, atlasHeight, "Atlas_1")
			//TextureAtlas(atlasWidth, atlasHeight, "Atlas_2"),
			//TextureAtlas(atlasWidth, atlasHeight, "Atlas_3"),
			//TextureAtlas(atlasWidth, atlasHeight, "Atlas_4")
		};
	}

}

ResourceManager::ResourceManager(uint32_t atlasWidth, uint32_t atlasHeight)
	: m_atlas{Detail::GenerateAtlasArray_(atlasWidth, atlasHeight)}
{}

std::vector<ResourceManager::AnimationMapKey> ResourceManager::LoadAllAnimations(std::filesystem::path const& parentFolder)
{
	std::cout << "Attempting to load Animations in " << parentFolder << "\n";
	std::vector<ResourceManager::AnimationMapKey> animationsLoaded;
	for (auto const& dir : std::filesystem::directory_iterator{ parentFolder }) {
		if (dir.is_directory()) {
			//find json descriptor in directory
			//then load animationSet described by that
			std::optional<std::filesystem::path> animationDescriptorFile = Detail::FindDescriptorFile_(dir.path().lexically_normal());
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

		//add to animationSet
		animationSet.animations.push_back((*oAnim).first);
		textures.push_back((*oAnim).second);
	}

	
	for(auto& atlas : m_atlas){
		//attempt to pack into atlas
		bool res = atlas.AddToAtlas(animationSet, textures);
		//if this fails try the next one, otherwise done
		if(res) break;
	}

	//check if animationset has an atlas ID set
	//if not we have failed to pack it entirely
	if(animationSet.atlasId == k_invalidAtlasId)
	{
		std::cout << "Failed to pack " << animationSet.name << " in Resource Manager atlases." << std::endl;
		return "";
	}

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

}



