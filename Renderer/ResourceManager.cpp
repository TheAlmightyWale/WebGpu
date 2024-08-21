#include "ResourceManager.h"
#include "Utils.h"

void ResourceManager::LoadAllAnimations(std::filesystem::path const& parentFolder)
{
	std::cout << "Attempting to load Animations in " << parentFolder << "\n";
	for (auto const& dir : std::filesystem::directory_iterator{ parentFolder }) {
		if (dir.is_directory()) {
			auto oAnim = Utils::LoadAnimationTexture(dir);
			if (oAnim) m_anims.emplace((*oAnim).label, *oAnim);
		}
	}
}

TextureResource const& ResourceManager::GetAnimation(AnimationMapKey id) const noexcept
{
	return m_anims.at(id);
}

ResourceManager::AnimationMap const& ResourceManager::GetAllAnimations() const noexcept
{
	return m_anims;
}



