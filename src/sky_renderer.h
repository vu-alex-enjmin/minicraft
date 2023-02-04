#ifndef __SKY_RENDERER__
#define __SKY_RENDERER__

#include <cstdint>
#include "external/gl/glew.h"
#include "engine/utils/types_3d.h"
#include "engine/render/vbo.h"

class SkyRenderer
{
private:
	float sunProgressT = 0;

	void updateSunProgressT(const uint64_t timeOffsetMillis);
	void updateSunAndSkyColors();
public:
	GLuint ShaderSun = 0;

	int64_t fullDayMillis;
	int64_t sunRiseTime;
	int64_t sunSetTime;
	int64_t midnightTime;

	YColor skyColor;
	YColor outerSunColor;
	YColor innerSunColor;
	YColor ambientColor;
	YColor lightingSunColor;

	float sunAngle = 0;
	YVec3f sunDirection;

	SkyRenderer();

	void loadShaders();

	void updateSkyValues(const uint64_t timeOffsetMillis);

	void render(YVbo* sunVbo, const float sunScale, const float sunDistance);
};

#endif