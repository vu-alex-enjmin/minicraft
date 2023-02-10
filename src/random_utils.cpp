#include "random_utils.h"

#include <cstdlib>

float randomFloat01()
{
	return float(rand()) / float(RAND_MAX);
}

float randomFloat(const float minInclusive, const float maxInclusive)
{
	return minInclusive + randomFloat01() * (maxInclusive - minInclusive);
}
