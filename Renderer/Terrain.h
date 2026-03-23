#pragma once
#include <cstdint>
#include <vector>
#include "QuadDefs.h"
#include "Core/ResourceDefs.h"

class ResourceManager;

//Handles generating a grid of tiles and stores their locations
// eventually will handle also generating the associated initial height map
// eventually having a hierarchical layout so that we can do things like modify height and flow fields
class Terrain {
public:

	Terrain(uint32_t width, uint32_t height, uint32_t cellSize, ResourceManager const& resourceManager);
	inline std::vector<QuadTransform> const& Cells() {
		return _cells;
	}
	inline std::vector<AnimUniform> const& CellAnimations() {
		return _cellAnim;
	}

	void Animate(float dT);

private:
	std::vector<QuadTransform> _cells;
	std::vector<AnimUniform> _cellAnim;
	std::vector<Animation> _animations;

	float _secs = 0.f;

};