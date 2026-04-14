#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include "QuadDefs.h"
#include "Core/ResourceDefs.h"

namespace Gfx
{

class ResourceManager;

//Handles generating a grid of tiles and stores their locations
// eventually will handle also generating the associated initial height map
// eventually having a hierarchical layout so that we can do things like modify height and flow fields
class Terrain {
public:

	Terrain(uint32_t width, uint32_t height, uint32_t cellSize, ResourceManager const& resourceManager);
	inline std::span<QuadTransform> const& Cells() {
		return _cells;
	}
	inline std::span<AnimUniform> const& CellAnimations() {
		return _cellAnim;
	}

	void Animate(float dT);

private:
	std::span<QuadTransform> _cells;
	std::span<AnimUniform> _cellAnim;
	std::vector<uint32_t> _cellAnimIds;
	std::vector<Animation> _animations;

	float _secs = 0.f;

};

}