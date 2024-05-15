#pragma once
#include "MathDefs.h"

struct QuadTransform
{
	Vec3f position;
	float _pad;
	Vec2f scale;
	float _pad2[2]; //Each member must be 16 byte aligned, min 32 bytes
};
static_assert(sizeof(QuadTransform) % 16 == 0);