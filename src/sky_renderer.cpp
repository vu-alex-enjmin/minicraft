#include "sky_renderer.h"

#include "time_utils.h"
#include "my_math.h"
#include <cmath>

#include "engine/render/renderer.h"

SkyRenderer::SkyRenderer()
{
	fullDayMillis = computeTimeMillis(24, 0, 0, 0);
	sunRiseTime = computeTimeMillis(6, 0, 0, 0);
	sunSetTime = computeTimeMillis(19, 0, 0, 0);
	midnightTime = int64_t(sunSetTime + (fullDayMillis - (sunSetTime - sunRiseTime)) * 0.5) % fullDayMillis;
}

void SkyRenderer::loadShaders()
{
	ShaderSun = YRenderer::getInstance()->createProgram("shaders/sun");
}

void SkyRenderer::updateSkyValues(const uint64_t timeOffsetMillis)
{
	updateSunProgressT(timeOffsetMillis);
	updateSunAndSkyColors();

	float sunRiseAngle = 90.0f;
	float sunSetAngle = -90.0f;
	sunAngle = lerp(sunRiseAngle, sunSetAngle, sunProgressT);
	sunDirection = YVec3f(0, 0, 1).rotate(YVec3f(-1, 0.5, 0).normalize(), sunAngle * M_PI / 180.0f);
}

void SkyRenderer::render(YVbo* sunVbo, const float sunScale, const float sunDistance)
{
	YRenderer* renderer = YRenderer::getInstance();

	// Dessin du ciel
	renderer->setBackgroundColor(skyColor);

	// Dessin du soleil
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();

	YVec3f camPos = renderer->Camera->Position;
	glTranslatef(camPos.X, camPos.Y, camPos.Z);

	// Trac� du soleil
	drawSun(sunVbo, sunAngle, sunScale, sunDistance, outerSunColor, innerSunColor);

	// Trac� de la lune
	drawSun(sunVbo, sunAngle + 180, sunScale * 0.625, sunDistance, outerMoonColor, innerMoonColor);

	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

void SkyRenderer::drawSun(YVbo *sunVbo, const float angle, const float scale, const float distance, const YColor &outerColor, const YColor &innerColor)
{
	YRenderer* renderer = YRenderer::getInstance();

	glUseProgram(ShaderSun);
	GLuint shaderSun_sunColor = glGetUniformLocation(ShaderSun, "sun_color");

	glPushMatrix();
		glRotatef(angle, -1, 0.5, 0);
		glTranslatef(0, 0, distance);
		glScalef(scale, scale, scale);
		glRotatef(-angle, -1, 0.5, 0);

		glUniform3f(shaderSun_sunColor, outerColor.R, outerColor.V, outerColor.B);
		renderer->updateMatricesFromOgl(); //Calcule toute les matrices � partir des deux matrices OGL
		renderer->sendMatricesToShader(ShaderSun); //Envoie les matrices au shader
		sunVbo->render(); //Demande le rendu du VBO

		glUniform3f(shaderSun_sunColor, innerColor.R, innerColor.V, innerColor.B);
		glScalef(0.8f, 0.8f, 0.8f);
		renderer->updateMatricesFromOgl(); //Calcule toute les matrices � partir des deux matrices OGL
		renderer->sendMatricesToShader(ShaderSun); //Envoie les matrices au shader
		sunVbo->render(); //Demande le rendu du VBO
	glPopMatrix();
}

void SkyRenderer::updateSunProgressT(const uint64_t timeOffsetMillis)
{
	int64_t currentTimeMillis = getLocalTimeMillisl() + timeOffsetMillis;
	currentTimeMillis %= fullDayMillis;

	if (currentTimeMillis < sunRiseTime)
	{
		float referenceMidnightTime = (midnightTime > sunRiseTime) ? (midnightTime - fullDayMillis) : midnightTime;
		sunProgressT = -0.5f + inverseLerp(referenceMidnightTime, sunRiseTime, currentTimeMillis) * 0.5f;
	}
	else if (currentTimeMillis > sunSetTime)
	{
		float referenceMidnightTime = (midnightTime < sunSetTime) ? (midnightTime + fullDayMillis) : midnightTime;
		sunProgressT = inverseLerp(sunSetTime, referenceMidnightTime, currentTimeMillis) * 0.5f + 1.0;
	}
	else
		sunProgressT = inverseLerp(sunRiseTime, sunSetTime, currentTimeMillis);
}

void SkyRenderer::updateSunAndSkyColors()
{
	float distToZenith = std::abs(sunProgressT - 0.5);
	float distToZenithTransitionColorRange = 0.05f;
	float sunAppearanceColorT = saturate((0.5 - distToZenith) / distToZenithTransitionColorRange);

	// Sky Color
	float sunAppearanceSkyColorT = (0.5 - distToZenith) / distToZenithTransitionColorRange;
	YColor brightSkyColor(0.4f, 0.85f, 1, 1);
	YColor darkSkyColor(0.025f, 0.05f, 0.125f, 1);
	YColor sunAppearanceSkyColor(0.8f, 0.5f, 0.3f, 1);

	if (sunAppearanceSkyColorT > 0)
	{
		skyColor = sunAppearanceSkyColor.interpolate(brightSkyColor, sunAppearanceSkyColorT);
	}
	else
	{
		skyColor = sunAppearanceSkyColor.interpolate(darkSkyColor, -sunAppearanceSkyColorT);
	}

	// Sun Color
	YColor white = YColor(1, 1, 1, 1);
	YColor baseSunColor(1, 0.9f, 0.25f, 1);
	YColor sunAppearanceColor(1, 0.1f, 0, 1);
	outerSunColor = sunAppearanceColor.interpolate(baseSunColor, sunAppearanceColorT);
	innerSunColor = outerSunColor.interpolate(white, 0.25f);
	lightingSunColor = outerSunColor.interpolate(white, 0.75f);

	// Moon Color
	outerMoonColor = YColor(0.2, 0.35, 0.5, 1);
	innerMoonColor = YColor(0.75, 0.8, 0.85, 1);

	// Ambient Color
	YColor brightAmbientColor(0.6f, 0.65f, 0.7f, 1);
	YColor darkAmbientColor(0.225f, 0.225f, 0.35f, 1);
	YColor sunAppearanceAmbientColor(0.6f, 0.5f, 0.4f, 1);
	if (sunAppearanceSkyColorT > 0)
	{
		ambientColor = sunAppearanceAmbientColor.interpolate(brightAmbientColor, sunAppearanceSkyColorT);
	}
	else
	{
		ambientColor = sunAppearanceAmbientColor.interpolate(darkAmbientColor, -sunAppearanceSkyColorT);
	}
}