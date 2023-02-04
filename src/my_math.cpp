#include "my_math.h"

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
