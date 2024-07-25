#pragma once
#include "MathDefs.h"

struct QuadTransform
{
	Vec3f position;
	float _pad1 = 0.f;
	Vec2f scale;
	float _pad2[2] = {0.f,0.f}; //Each member must be 16 byte aligned, min 32 bytes
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
	uint32_t currentFrameIndex = 0;
	uint32_t animId;
	float _padding[2] = {0.f,0.f};
};

static_assert(sizeof(AnimUniform) % 16 == 0);