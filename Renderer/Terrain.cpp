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
		QuadTransform cell{
			{x,y,0.0f}, 0.f /*pad*/,
			{cellSize, cellSize}
		};
		_cells.emplace_back(cell);

		AnimUniform anim;
		anim.currentFrameIndex = cellId % 8;
		anim.animId = cellId % 2;
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
			anim.currentFrameIndex = (anim.currentFrameIndex + 1) % 8;
		}
		_secs = 0.f;
	}

	_secs += dT;
}
