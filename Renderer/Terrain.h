#pragma once
#include <cstdint>
#include <vector>
#include "QuadDefs.h"

//Handles generating a grid of tiles of stores their locations
// eventually will handle also generating the associated initial height map
// eventually having a hierarchical layout so that we can do things like modify height and flow fields
class Terrain {
public:

	Terrain(uint32_t width, uint32_t height, uint32_t cellSize);
	inline std::vector<QuadTransform> const& Cells() {
		return _cells;
	}

private:
	std::vector<QuadTransform> _cells;

};