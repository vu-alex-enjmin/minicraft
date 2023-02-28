#include "atlas_uv_mapper.h"

#define SET_OFFSET(x, y) xOffset = x; yOffset = y; return

void AtlasUVMapper::getUVsForCube(const MCube::MCubeType type, const cubeSide side, float *uvs)
{
	int xOffset, yOffset;
	getOffsetForCube(type, side, xOffset, yOffset);
	getUVsForOffset(xOffset, yOffset, uvs);
}

void AtlasUVMapper::getOffsetForCube(const MCube::MCubeType type, const cubeSide side, int &xOffset, int &yOffset)
{
	switch (type)
	{
		case MCube::CUBE_HERBE:
			if (side == cubeSide::NEG_Z) {
				SET_OFFSET(1, 0);
			} else if (side == cubeSide::POS_Z) {
				SET_OFFSET(2, 0);
			} else {
				SET_OFFSET(3, 0);
			}
		case MCube::CUBE_TERRE:
			SET_OFFSET(1, 0);
		case MCube::CUBE_PIERRE:
			SET_OFFSET(0, 1);
		case MCube::CUBE_SABLE_01:
			SET_OFFSET(1, 1);
		case MCube::CUBE_LAINE_01:
			SET_OFFSET(2, 1);
	}

	xOffset = 0;
	yOffset = 0;
}

void AtlasUVMapper::getUVsForOffset(const int xOffset, const int yOffset, float *uvs)
{
	// TOP LEFT
	uvs[0] = float(xOffset) * invWidth;
	uvs[1] = float(yOffset) * invHeight;

	// TOP RIGHT
	uvs[2] = float(xOffset + 1) * invWidth;
	uvs[3] = float(yOffset) * invHeight;

	// BOTTOM RIGHT
	uvs[4] = float(xOffset + 1) * invWidth;
	uvs[5] = float(yOffset + 1) * invHeight;

	// BOTTOM LEFT
	uvs[6] = float(xOffset) * invWidth;
	uvs[7] = float(yOffset + 1) * invHeight;
}
