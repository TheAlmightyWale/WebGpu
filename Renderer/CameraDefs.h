#pragma once
#include "Core/MathDefs.h"

namespace Gfx
{

struct GlobalUniform
{
	Mat4f projection;
	Mat4f view;
	Mat4f model;
	Vec4f color;
	float time;
	float _pad[3] = {0.f, 0.f, 0.f}; // struct must be 16byte aligned
};
static_assert(sizeof(GlobalUniform) % 16 == 0);

struct CamUniform
{
	Vec2f position;
	Vec2f extents;
};
static_assert(sizeof(CamUniform) % 16 == 0);

}