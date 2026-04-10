#include <gtest/gtest.h>
#include "Utils.h"
#include "MathDefs.h"

TEST(Blit, copyWholeRect)
{
    uint32_t largeRowSize = 100;
    uint32_t largeColSize = 100;

    uint32_t smallRowSize = 20;
    uint32_t smallColSize = 20;

    
    std::vector<uint64_t> large;
    large.resize(largeRowSize * largeColSize, 0);
    //100x100 rect with stride of 8

    //20x20 rect with stride of 8
    std::vector<uint64_t> small;
    small.resize(smallRowSize * smallColSize, 1);

    //get byte pointer
    std::byte* pDest = reinterpret_cast<std::byte*>(large.data());
    std::byte* pSrc = reinterpret_cast<std::byte*>(small.data());

    //copy into 100x100 rect from 0,0 to 20,20
    Gfx::Utils::Blit(
        pDest,
        Vec2u{0,0},
        largeRowSize,
        pSrc,
        smallRowSize,
        Rect{{0,0},
            {smallRowSize, smallColSize}},
         sizeof(uint64_t)
    );

    //check copy is correct
    for(uint32_t i = 0; i < smallRowSize; i++)
    {   
        for(uint32_t j = 0; j < smallColSize; j++)
        {
            EXPECT_EQ(large[j * largeRowSize + i], 1);
        }
    }
}

TEST(Blit, copyPartialRect)
{
    uint32_t largeRowSize = 100;
    uint32_t largeColSize = 100;

    uint32_t smallRowSize = 20;
    uint32_t smallColSize = 20;

    
    std::vector<uint64_t> large;
    large.resize(largeRowSize * largeColSize, 0);
    //100x100 rect with stride of 8

    //20x20 rect with stride of 8
    std::vector<uint64_t> small;
    small.resize(smallRowSize * smallColSize, 1);

    //get byte pointer
    std::byte* pDest = reinterpret_cast<std::byte*>(large.data());
    std::byte* pSrc = reinterpret_cast<std::byte*>(small.data());

    //copy into 100x100 rect from 0,0 to 10,10
    Rect copyRect{{0,0}, {10, 10}};
    Gfx::Utils::Blit(
        pDest,
        Vec2u{0,0},
        largeRowSize,
        pSrc,
        smallRowSize,
        copyRect,
        sizeof(uint64_t)
    );

    //check copy is correct
    for(uint32_t i = 0; i < copyRect.dims.x; i++)
    {   
        for(uint32_t j = 0; j < copyRect.dims.y; j++)
        {
            EXPECT_EQ(large[j * largeRowSize + i], 1);
        }
    }

    //copy int 100x100 rect from 10,10 to 20,20
    copyRect = Rect{{10,10}, {10, 10}};
    Vec2u offset{10,10};
    Gfx::Utils::Blit(
        pDest,
        offset,
        largeRowSize,
        pSrc,
        smallRowSize,
        copyRect,
        sizeof(uint64_t)
    );

    //check copy is correct
    for(uint32_t i = 0; i < copyRect.dims.x; i++)
    {   
        uint32_t iOffset = i + offset.x;
        for(uint32_t j = 0; j < copyRect.dims.y; j++)
        {
            uint32_t jOffset = j + offset.y;
            EXPECT_EQ(large[jOffset * largeRowSize + iOffset], 1);
        }
    }
}

TEST(Blit, copyWholeRectOffset)
{
    uint32_t largeRowSize = 100;
    uint32_t largeColSize = 100;

    uint32_t smallRowSize = 20;
    uint32_t smallColSize = 20;

    
    std::vector<uint64_t> large;
    large.resize(largeRowSize * largeColSize, 0);
    //100x100 rect with stride of 8

    //20x20 rect with stride of 8
    std::vector<uint64_t> small;
    small.resize(smallRowSize * smallColSize, 1);

    //get byte pointer
    std::byte* pDest = reinterpret_cast<std::byte*>(large.data());
    std::byte* pSrc = reinterpret_cast<std::byte*>(small.data());

    //copy into 100x100 rect from 20,20 to 40,40
    Vec2u offset{20,20};
    Gfx::Utils::Blit(
        pDest,
        offset,
        largeRowSize,
        pSrc,
        smallRowSize,
        Rect{{0,0},
            {smallRowSize, smallColSize}},
         sizeof(uint64_t)
    );

    //check copy is correct
    for(uint32_t i = 0; i < smallRowSize; i++)
    {   
        uint32_t iOffset = i + offset.x;
        EXPECT_EQ(large[(offset.y - 1) * largeRowSize + iOffset], 0);
        for(uint32_t j = 0; j < smallColSize; j++)
        {
            uint32_t jOffset = j + offset.y;
            EXPECT_EQ(large[jOffset * largeRowSize + iOffset], 1);
        }
        //Check boundaries
        EXPECT_EQ(large[(offset.y + smallColSize) * largeRowSize], 0);
    }

    //copy into 100x100 rect from 61,63 to 81,83
    offset = Vec2u{61,63};
    Gfx::Utils::Blit(
        pDest,
        offset,
        largeRowSize,
        pSrc,
        smallRowSize,
        Rect{{0,0},
            {smallRowSize, smallColSize}},
         sizeof(uint64_t)
    );

        //check copy is correct
    for(uint32_t i = 0; i < smallRowSize; i++)
    {   
        uint32_t iOffset = i + offset.x;
        EXPECT_EQ(large[(offset.y - 1) * largeRowSize + iOffset], 0);
        for(uint32_t j = 0; j < smallColSize; j++)
        {
            uint32_t jOffset = j + offset.y;
            EXPECT_EQ(large[jOffset * largeRowSize + iOffset], 1);
        }
        //Check boundaries
        EXPECT_EQ(large[(offset.y + smallColSize) * largeRowSize + iOffset], 0);
    }
}

