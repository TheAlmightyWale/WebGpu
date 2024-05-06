#include <array>
#include "MeshDefs.h"

namespace Gfx
{
	struct QuadVertex
	{
		float x, y, z;
		float u, v;
	};

	struct Quad
	{
		std::array<QuadVertex, 6> vertices = {
			//Bottom left to top left in counter clockwise order
			QuadVertex{0.f, 0.f, 0.f, 0.f, 1.f},
			QuadVertex{1.f, 0.f, 0.f, 1.f, 1.f},
			QuadVertex{1.f, 1.f, 0.f, 1.f, 0.f},
			QuadVertex{1.f, 1.f, 0.f, 1.f, 0.f},
			QuadVertex{0.f, 1.f, 0.f, 0.f, 0.f},
			QuadVertex{0.f, 0.f, 0.f, 0.f, 1.f},
		};
	};

}

