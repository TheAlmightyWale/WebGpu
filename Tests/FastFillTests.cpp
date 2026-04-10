#include <gtest/gtest.h>
#include "Utils.h"
#include <vector>
#include <cstddef>

// Convenience cast
static std::byte B(uint8_t v) { return static_cast<std::byte>(v); }

// Build the expected output: pattern repeated across destSize bytes (truncated)
static std::vector<std::byte> ExpectedPattern(std::vector<uint8_t> const& pattern, size_t destSize)
{
    std::vector<std::byte> result;
    result.reserve(destSize);
    for (size_t i = 0; i < destSize; i++)
        result.push_back(B(pattern[i % pattern.size()]));
    return result;
}

// Single-byte pattern, power-of-2 dest
TEST(FastFill, singleBytePattern)
{
    std::vector<std::byte> src = {B(0xAB)};
    std::vector<std::byte> dest(16, B(0));

    Gfx::Utils::FastFill(dest.data(), 16, src.data(), 1);

    EXPECT_EQ(dest, ExpectedPattern({0xAB}, 16));
}

// Core case: 2-byte pattern, dest is an exact power-of-2 multiple
TEST(FastFill, twoBytePatterExactPowerOfTwo)
{
    std::vector<std::byte> src = {B(0xAA), B(0xBB)};
    std::vector<std::byte> dest(16, B(0));

    Gfx::Utils::FastFill(dest.data(), 16, src.data(), 2);

    EXPECT_EQ(dest, ExpectedPattern({0xAA, 0xBB}, 16));
}

// dest == srcSize: just copies the source once
TEST(FastFill, destEqualsSrcSize)
{
    std::vector<std::byte> src = {B(1), B(2), B(3), B(4)};
    std::vector<std::byte> dest(4, B(0));

    Gfx::Utils::FastFill(dest.data(), 4, src.data(), 4);

    EXPECT_EQ(dest, ExpectedPattern({1, 2, 3, 4}, 4));
}

// dest == 2 * srcSize: one doubling step, no remainder
TEST(FastFill, destTwiceSrcSize)
{
    std::vector<std::byte> src = {B(1), B(2)};
    std::vector<std::byte> dest(4, B(0));

    Gfx::Utils::FastFill(dest.data(), 4, src.data(), 2);

    EXPECT_EQ(dest, ExpectedPattern({1, 2}, 4));
}

// Non-power-of-2 dest, evenly divisible by src
TEST(FastFill, nonPowerOfTwoDestDivisibleBySrc)
{
    std::vector<std::byte> src = {B(0xCA), B(0xFE)};
    std::vector<std::byte> dest(10, B(0));

    Gfx::Utils::FastFill(dest.data(), 10, src.data(), 2);

    EXPECT_EQ(dest, ExpectedPattern({0xCA, 0xFE}, 10));
}

// dest not divisible by srcSize: pattern is truncated at the end
TEST(FastFill, destNotDivisibleBySrc_even)
{
    // dest=8, src=3: [A,B,C,A,B,C,A,B]
    std::vector<std::byte> src = {B(1), B(2), B(3)};
    std::vector<std::byte> dest(8, B(0));

    Gfx::Utils::FastFill(dest.data(), 8, src.data(), 3);

    EXPECT_EQ(dest, ExpectedPattern({1, 2, 3}, 8));
}

TEST(FastFill, destNotDivisibleBySrc_oddDestOddSrcAtHalf)
{
    // dest=7, src=3
    std::vector<std::byte> src = {B(1), B(2), B(3)};
    std::vector<std::byte> dest(7, B(0));

    Gfx::Utils::FastFill(dest.data(), 7, src.data(), 3);

    EXPECT_EQ(dest, ExpectedPattern({1, 2, 3}, 7));
}


TEST(FastFill, destNotDivisibleBySrc_oddDestSrcAtHalf)
{
    std::vector<std::byte> src = {B(0xDE), B(0xAD), B(0xBE), B(0xEF)};
    std::vector<std::byte> dest(9, B(0));

    Gfx::Utils::FastFill(dest.data(), 9, src.data(), 4);

    EXPECT_EQ(dest, ExpectedPattern({0xDE, 0xAD, 0xBE, 0xEF}, 9));
}

// Large buffer — verifies doubling reaches the end correctly
TEST(FastFill, largeBuffer)
{
    std::vector<std::byte> src = {B(0x01), B(0x02), B(0x03)};
    const uint32_t destSize = 1024;
    std::vector<std::byte> dest(destSize, B(0));

    Gfx::Utils::FastFill(dest.data(), destSize, src.data(), 3);

    EXPECT_EQ(dest, ExpectedPattern({0x01, 0x02, 0x03}, destSize));
}

// Edge case: destSize == 1 (loop never executes, copies first byte of src)
TEST(FastFill, destSizeOne)
{
    std::vector<std::byte> src = {B(0x42), B(0x99)};
    std::vector<std::byte> dest(1, B(0));

    Gfx::Utils::FastFill(dest.data(), 1, src.data(), 2);

    EXPECT_EQ(dest[0], B(0x42));
}

// Verify bytes outside the dest range are untouched (no overflow)
TEST(FastFill, doesNotWritePastDestination)
{
    // Allocate 20 bytes, only fill the first 16
    std::vector<std::byte> dest(20, B(0xFF));
    std::vector<std::byte> src = {B(0xAB), B(0xCD)};

    Gfx::Utils::FastFill(dest.data(), 16, src.data(), 2);

    // First 16 bytes should be the pattern
    EXPECT_EQ(std::vector<std::byte>(dest.begin(), dest.begin() + 16),
              ExpectedPattern({0xAB, 0xCD}, 16));
    // Trailing guard bytes must be untouched
    EXPECT_EQ(dest[16], B(0xFF));
    EXPECT_EQ(dest[17], B(0xFF));
    EXPECT_EQ(dest[18], B(0xFF));
    EXPECT_EQ(dest[19], B(0xFF));
}
