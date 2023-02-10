#ifndef __CUSTOM_PERLIN__
#define __CUSTOM_PERLIN__

#include "engine/noise/perlin.h"

class CustomPerlin 
{
	private:
		YPerlin basePerlin;
	public :
		CustomPerlin();

		float sample(float, float, float, float);
		float sample(float, float, float);

		float sampleSimple(float, float, float);
		float sampleSimple(float, float);
};

#endif