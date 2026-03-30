#include "Packing.h"
#include <vector>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

namespace Gfx
{
    SkylinePacker::SkylinePacker(uint32_t atlasWidth, uint32_t atlasHeight)
    {
        //Recommended for node memory to be at least width in size
        _nodes.resize(atlasWidth);
        stbrp_init_target(&_context, atlasWidth, atlasHeight, _nodes.data(), atlasWidth);
        //Can optionally set a different heuristic by default we use BL, which packs at bottom-most left
        //stbrp_setup_heuristic(...)
    }

    std::optional<RectPackResult> SkylinePacker::Pack(uint32_t rectWidth, uint32_t rectHeight)
    {
        stbrp_rect stbRect;
        stbRect.w = rectWidth;
        stbRect.h = rectHeight;

        int res = stbrp_pack_rects(&_context, &stbRect, 1);
        if(res != 1){
            return std::nullopt;
        }

        return RectPackResult{
            (uint32_t)stbRect.x,
            (uint32_t)stbRect.y
        };
    }
}