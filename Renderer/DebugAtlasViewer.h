#pragma once
#include <span>
#include "QuadDefs.h"

namespace Gfx::Debug
{

//Debugging class used to display the current state of a texture atlas on the GPU
//essentially a giant quad that covers the entire atlas
class DebugAtlasViewer{
public:
    DebugAtlasViewer(float displaySize, uint32_t atlasWidth, uint32_t atlasHeight);
    
    void SetVisibility(bool bVis);

private:
    std::span<Gfx::QuadTransform> _transforms;
	std::span<Gfx::AnimUniform> _anims;
};
}