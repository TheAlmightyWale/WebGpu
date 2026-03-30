#include "Terrain.h"
#include "Core/ResourceManager.h"

namespace Gfx
{

Terrain::Terrain(uint32_t width, uint32_t height, uint32_t cellSize, ResourceManager const& resourceManager)
{
	uint32_t totalCells = width * height;
	_cells.reserve(totalCells);

	auto cell1Anim = resourceManager.GetAnimation("Cell1");
	auto cell2Anim = resourceManager.GetAnimation("Cell2");
	_animations.push_back(cell1Anim.animations[0]);
	_animations.push_back(cell2Anim.animations[0]);

	for (uint32_t cellId = 0; cellId < totalCells; cellId++) {
		uint32_t colPos = cellId % width;
		uint32_t rowPos = cellId / width;
		float x = (float)(cellSize * colPos);
		float y = (float)(cellSize * rowPos);
		QuadTransform cell{
			{x,y,0.0f}, 0.f /*pad*/,
			{cellSize, cellSize}
		};
		_cells.emplace_back(cell);

		uint32_t animId = cellId % 2;
		Animation animation = _animations[animId];
		
		AnimUniform anim;
		anim.currentFrameIndex = cellId % animation.length;
		anim.animId = animId;
		anim.startCoord = { 0,0 };
		anim.frameDimensions = { animation.frameRes.width, animation.frameRes.height };
		_cellAnim.emplace_back(anim);
	}
}

void Terrain::Animate(float dT)
{
	constexpr float k_fps = 1;

	if (_secs > k_fps)
	{
		for (auto& anim : _cellAnim)
		{
			uint32_t animLength = _animations[anim.animId].length;
			anim.currentFrameIndex = (anim.currentFrameIndex + 1) % animLength;
		}
		_secs = 0.f;
	}

	_secs += dT;
}

}
