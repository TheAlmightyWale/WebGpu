#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include "ext/stb_rect_pack.h"

namespace Gfx
{
    struct RectPackResult
    {
        uint32_t x;
        uint32_t y; 
    };

    //Packs rectangles using the skyline algorithm
    // operates as a virtual atlas, does not actually copy any data, but does keep track
    // of rectangle placements made to it previously
    class SkylinePacker
    {
    public:
        SkylinePacker(uint32_t atlasWidth, uint32_t atlasHeight);

        //Place the input rectangle in the existing atlas
        //Returns the coordinates where the bottom-left corner of the 
        // rectangle was packed in to the atlas
        std::optional<RectPackResult> Pack(uint32_t rectWidth, uint32_t rectHeight);

    private:
        stbrp_context _context;
        std::vector<stbrp_node> _nodes;

    };
}