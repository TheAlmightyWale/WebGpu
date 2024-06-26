#include "Terrain.h"

Terrain::Terrain(uint32_t width, uint32_t height, uint32_t cellSize)
{
	uint32_t totalCells = width * height;
	_cells.reserve(totalCells);

	for (uint32_t cellId = 0; cellId < totalCells; cellId++) {
		uint32_t colPos = cellId % width;
		uint32_t rowPos = cellId / width;
		float x = (float)(cellSize * colPos);
		float y = (float)(cellSize * rowPos);
		uint32_t textureIndex = 0;
		QuadTransform cell{
			{x,y,0.0f}, textureIndex,
			{cellSize, cellSize}
		};
		_cells.emplace_back(cell);

		AnimUniform anim;
		anim.currentFrameIndex = cellId % 8;
		anim.startCoord = { 0,0 };
		anim.frameDimensions = { 50,38 };
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
			anim.currentFrameIndex = ++anim.currentFrameIndex % 8;
		}
		_secs = 0.f;
	}

	_secs += dT;
}
