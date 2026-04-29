#include "DebugAtlasViewer.h"
#include "GlobalBuffers.h"
#include "Core/ResourceDefs.h"

namespace Gfx::Debug
{
    DebugAtlasViewer::DebugAtlasViewer(float displaySize, uint32_t atlasWidth, uint32_t atlasHeight)
    {
        _transforms = Gfx::g_transformBuffer.RegisterData(Gfx::k_maxAtlas);
        _anims = Gfx::g_animationBuffer.RegisterData(Gfx::k_maxAtlas);

        // Fill in debug atlas data
        for (uint32_t i = 0; i < Gfx::k_maxAtlas; i++)
        {
            _transforms[i] = Gfx::QuadTransform{
                Vec3f(-10.f - displaySize, (i * -5.0f) - (i * displaySize), 0.0f),
                1, // is visible
                Vec2f(displaySize, displaySize)};

            // cover entierty of atlas
            _anims[i] = Gfx::AnimUniform{
                Vec2f(0.f, 0.f),
                Vec2f(atlasWidth, atlasHeight),
                0,
                i};
        }
    }

    void DebugAtlasViewer::SetVisibility(bool bVis)
    {
        for(auto& transform : _transforms)
        {
            transform.bIsVisible = bVis? 1 : 0; 
        }
    }

}