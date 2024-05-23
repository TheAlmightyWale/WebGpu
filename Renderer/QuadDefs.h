#pragma once
#include "MathDefs.h"

struct QuadTransform
{
	Vec3f position;
	uint32_t textureIndex;
	Vec2f scale;
	float _pad2[2]; //Each member must be 16 byte aligned, min 32 bytes
};
static_assert(sizeof(QuadTransform) % 16 == 0);

struct CamUniforms
{
	Vec2f position;
	Vec2f extents;
};
static_assert(sizeof(CamUniforms) % 16 == 0);

struct AnimUniform
{
	Vec2f startCoord;
	Vec2f frameDimensions;
	uint32_t currentFrameIndex;
	float _padding[3];
};

static_assert(sizeof(AnimUniform) % 16 == 0);