#include "customPerlin.h"
#include <cmath>

#define clamp01(x) min(1, max(0, x))
#define OCTAVES 3
#define LACUNARITY 2.14853
#define PERSISTENCE 0.5
#define OFFSET_MULTIPLIER_MULTIPLIER 1.2563745

CustomPerlin::CustomPerlin()
{
	basePerlin.Freq = 0.0047438451471;
}

float CustomPerlin::sample(float x, float y, float z)
{
	float maxValue = 0.0f;
	float value = 0.0f;

	float currentLoopMaxValue = 1.0f;
	float currentLoopFrequency = 1.0f;
	YVec3f baseOffset(485.154412247, -322.1457, -147.0152647);
	YVec3f offsetMultiplier(0.45874956959, 1.245687, 2.485965);

	for (int i = 0; i < OCTAVES; i++)
	{
		YVec3f offset = baseOffset * offsetMultiplier;
		offset = YVec3f();
		value += sampleSimple(
			(x + offset.X) * currentLoopFrequency,
			(y + offset.Y) * currentLoopFrequency,
			(z + offset.Z) * currentLoopFrequency);

		maxValue += currentLoopMaxValue;
		currentLoopMaxValue *= PERSISTENCE;
		currentLoopFrequency *= LACUNARITY;
	}

	return clamp01(value / maxValue);
}

float CustomPerlin::sample(float x, float y)
{
	return sample(x, y, 0);
}

float CustomPerlin::sampleSimple(float x, float y, float z)
{
	float value = basePerlin.sample(x, y, z);
	value = (value - 0.6f) * 3.0f + 0.5f;
	return value;
}

float CustomPerlin::sampleSimple(float x, float y)
{
	return sampleSimple(x, y, 0);
}