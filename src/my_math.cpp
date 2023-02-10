#include "my_math.h"

#define _USE_MATH_DEFINES

#include <cmath>
#include <algorithm>

float lerp(const float a, const float b, const float t)
{
	return a * (1.0f - t) + (b * t);
}

float inverseLerp(const float a, const float b, const float x)
{
	return (x - a) / (b - a);
}

float saturate(const float value)
{
	return std::max<float>(0.0f, std::min<float>(1.0f, value));
}

float clamp(const float value, const float min, const float max)
{
	return std::max<float>(min, std::min<float>(value, max));
}

float radToDeg(const float radians)
{
	return (radians / M_PI) * 180.0f;
}

float degToRad(const float degrees)
{
	return (degrees / 180.0f) * M_PI;
}
