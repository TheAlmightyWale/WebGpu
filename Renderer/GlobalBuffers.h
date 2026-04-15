#pragma once
#include <array>
#include "CpuBuffer.hpp"
#include "QuadDefs.h"
#include "CameraDefs.h"

namespace Gfx
{

static uint32_t const k_maxGlobalBufferSize = 1024;
inline CpuBuffer<QuadTransform> g_transformBuffer(k_maxGlobalBufferSize);
inline CpuBuffer<AnimUniform> g_animationBuffer(k_maxGlobalBufferSize);
inline CpuBuffer<CamUniform> g_cameraUniformBuffer(1);

}