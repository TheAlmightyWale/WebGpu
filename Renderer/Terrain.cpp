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
			{x,y,0.0f}, {0.f},
			{cellSize, cellSize}
		};
		
		_cells.emplace_back(cell);
	}
}
