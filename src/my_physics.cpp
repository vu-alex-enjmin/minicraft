#include "my_physics.h"
#include <limits>

#define TEST_FACE_INTERSECTION(a, b, c, d, side) \
if (lineCubeFaceIntersection( \
	lineOrigin, lineDirection, \
	corners[a], corners[b], corners[c], corners[d], \
	currentIntersection, currentIntersectionT, maxDist)) \
{ \
	if (currentIntersectionT < outIntersectionT) \
	{ \
		collisionSide = side; \
		outIntersectionT = currentIntersectionT; \
		outIntersection = currentIntersection; \
	} \
}

bool linePlaneIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const YVec3f &planePoint, const YVec3f &planeNormal,
	YVec3f &outIntersection, float &outIntersectionT, 
	const float maxDist)
{
	float a = planeNormal.X;
	float b = planeNormal.Y;
	float c = planeNormal.Z;

	float denominator = a * lineDirection.X + b * lineDirection.Y + c * lineDirection.Z;
	if (std::abs(denominator) < 1e-6f)
		return false;

	float d = -(
		planeNormal.X * planePoint.X +
		planeNormal.Y * planePoint.Y +
		planeNormal.Z * planePoint.Z
		);
	float numerator = -(a * lineOrigin.X + b * lineOrigin.Y + c * lineOrigin.Z + d);
	outIntersectionT = numerator / denominator;

	if (outIntersectionT < 1e-6f || outIntersectionT > maxDist)
		return false;

	outIntersection = lineOrigin + lineDirection * outIntersectionT;
	return true;
}

bool lineCubeFaceIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const YVec3f &a, const YVec3f &b,
	const YVec3f &c, const YVec3f &d,
	YVec3f &outIntersection, float& outIntersectionT, 
	const float maxDist)
{
	YVec3f faceNormal = (a - b).cross(c - b);
	if (!linePlaneIntersection(lineOrigin, lineDirection, a, faceNormal, outIntersection, outIntersectionT, maxDist))
		return false;

	YVec3f ab_x_ap = (b - a).cross(outIntersection - a);
	YVec3f bc_x_bp = (c - b).cross(outIntersection - b);
	YVec3f cd_x_cp = (d - c).cross(outIntersection - c);
	YVec3f da_x_dp = (a - d).cross(outIntersection - d);

	float dot1 = ab_x_ap.dot(bc_x_bp);
	if (dot1 < 0)
		return false;
		
	float dot2 = cd_x_cp.dot(da_x_dp);
	if (dot2 < 0)
		return false;

	float dot3 = bc_x_bp.dot(cd_x_cp);
	return (dot3 >= 0);
}

cubeSide lineUnitCubeIntersection(
	const YVec3f &lineOrigin, const YVec3f &lineDirection,
	const int cubeX, const int cubeY, const int cubeZ,
	YVec3f &outIntersection, float &outIntersectionT,
	const float maxDist)
{
	YVec3f corners[8]
	{
		// BOTTOM
		YVec3f(cubeX, cubeY, cubeZ),
		YVec3f(cubeX + 1, cubeY, cubeZ),
		YVec3f(cubeX + 1, cubeY + 1, cubeZ),
		YVec3f(cubeX, cubeY + 1, cubeZ),
		// TOP
		YVec3f(cubeX, cubeY, cubeZ + 1),
		YVec3f(cubeX + 1, cubeY, cubeZ + 1),
		YVec3f(cubeX + 1, cubeY + 1, cubeZ + 1),
		YVec3f(cubeX, cubeY + 1, cubeZ + 1)
	};

	cubeSide collisionSide = NONE;
	outIntersectionT = std::numeric_limits<float>::infinity();
	
	float currentIntersectionT;
	YVec3f currentIntersection;
	// LEFT (-X)
	TEST_FACE_INTERSECTION(4, 7, 3, 0, NEG_X);

	// RIGHT (+X)
	TEST_FACE_INTERSECTION(6, 5, 1, 2, POS_X);

	// BACK (-Y)
	TEST_FACE_INTERSECTION(5, 4, 0, 1, NEG_Y);

	// FRONT (+Y)
	TEST_FACE_INTERSECTION(7, 6, 2, 3, POS_Y);

	// BOTTOM (-Z)
	TEST_FACE_INTERSECTION(1, 0, 3, 2, NEG_Z);

	// UP (+Z)
	TEST_FACE_INTERSECTION(4, 5, 6, 7, POS_Z);

	return collisionSide;
}
