#ifndef __ATLAS_UV_MAPPER__
#define __ATLAS_UV_MAPPER__

#include "cube.h"
#include "cube_side.h"

class AtlasUVMapper
{
public:
	static constexpr int width = 4;
	static constexpr int height = 2;
	float invWidth;
	float invHeight;

	AtlasUVMapper()
	{
		invWidth = 1.0 / width;
		invHeight = 1.0 / height;
	}

	void getUVsForCube(const MCube::MCubeType type, const cubeSide side, float* uvs);

private:
	void getOffsetForCube(const MCube::MCubeType type, const cubeSide side, int &xOffset, int &yOffset);
	void getUVsForOffset(const int xOffset, const int yOffset, float* uvs);
};

#endif