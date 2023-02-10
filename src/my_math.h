#ifndef __MY_MATH__
#define __MY_MATH__

float lerp(const float a, const float b, const float t);

float inverseLerp(const float a, const float b, const float x);

float saturate(const float value);

float clamp(const float value, const float min, const float max);

float radToDeg(const float radians);

float degToRad(const float degrees);

#endif
