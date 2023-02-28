#ifndef __MY_PHYSICS__
#define __MY_PHYSICS__

#include "cube_side.h"
#include "engine/utils/types_3d.h"

bool linePlaneIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const YVec3f &planePoint, const YVec3f &planeNormal,
	YVec3f &outIntersection, float &outIntersectionT, 
	const float maxDist);

bool lineCubeFaceIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const YVec3f &a, const YVec3f &b,
	const YVec3f &c, const YVec3f &d,
	YVec3f &outIntersection, float& outIntersectionT, 
	const float maxDist);

cubeSide lineUnitCubeIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const int cubeX, const int cubeY, const int cubeZ,
	YVec3f &outIntersection, float &outIntersectionT,
	const float maxDist);

#endif