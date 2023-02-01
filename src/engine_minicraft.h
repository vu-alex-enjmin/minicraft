#ifndef __YOCTO__ENGINE_TEST__
#define __YOCTO__ENGINE_TEST__

#include <cmath>

#include "engine/engine.h"

#include "avatar.h"
#include "world.h"

class MEngineMinicraft : public YEngine {

public:
	YVbo* VboCube;
	MWorld* World;
	GLuint ShaderCubeDebug;
	GLuint ShaderCube;
	GLuint ShaderSun;
	int pointCount;
	int64_t timeOffset = 0;
	

	//Gestion singleton
	static YEngine* getInstance()
	{
		if (Instance == NULL)
			Instance = new MEngineMinicraft();
		return Instance;
	}

	/*HANDLERS GENERAUX*/
	void loadShaders() {
		ShaderCubeDebug = Renderer->createProgram("shaders/cube_debug");
		ShaderCube = Renderer->createProgram("shaders/cube");
		ShaderSun = Renderer->createProgram("shaders/sun");
	}

	struct Point
	{
		float x, y, z;
		float nX, nY, nZ;
		float u, v;
	};

	void init()
	{
		YLog::log(YLog::ENGINE_INFO, "Minicraft Started : initialisation");

		Renderer->setBackgroundColor(YColor(0.0f, 0.0f, 0.0f, 1.0f));
		Renderer->Camera->setPosition(YVec3f(10, 10, 10));

		//Creation du VBO
		VboCube = new YVbo(3, 36, YVbo::PACK_BY_ELEMENT_TYPE);

		//Définition du contenu du VBO
		float normals[3 * 6] =
		{
			1, 0, 0,
			-1, 0, 0,
			0, 1, 0,
			0, -1, 0,
			0, 0, 1,
			0, 0, -1
		};

		float uvs[2 * 4] =
		{
			0, 0,
			1, 0,
			0, 1,
			0, 1
		};

		float pos[4 * 6 * 3] =
		{
			// RIGHT
			1, -1, -1,
			1, -1, 1,
			1, 1, 1,
			1, 1, -1,
			// LEFT
			-1, -1, -1,
			-1, 1, -1,
			-1, 1, 1,
			-1, -1, 1,
			// FORWARD
			-1, 1, -1,
			1, 1, -1,
			1, 1, 1,
			-1, 1, 1,
			// BACK
			-1, -1, -1,
			-1, -1, 1,
			1, -1, 1,
			1, -1, -1,
			// UP
			-1, -1, 1,
			-1, 1, 1,
			1, 1, 1,
			1, -1, 1,
			// DOWN
			-1, -1, -1,
			1, -1, -1,
			1, 1, -1,
			-1, 1, -1
		};

		VboCube->setElementDescription(0, YVbo::Element(3)); //Sommet
		VboCube->setElementDescription(1, YVbo::Element(3)); //Normale
		VboCube->setElementDescription(2, YVbo::Element(2)); //UV

		//On demande d'allouer la mémoire coté CPU
		VboCube->createVboCpu();

		Point corners[4];

		//On ajoute les sommets
		for (int i = 0; i < 6; i++)
		{
			for (int v = 0; v < 4; v++)
			{
				int index = i * 4 + v;
				corners[v].x = pos[index * 3 + 0];
				corners[v].y = pos[index * 3 + 1];
				corners[v].z = pos[index * 3 + 2];
				corners[v].nX = normals[i * 3];
				corners[v].nY = normals[i * 3 + 1];
				corners[v].nZ = normals[i * 3 + 2];
				corners[v].u = uvs[v * 2 + 0];
				corners[v].v = uvs[v * 2 + 1];
			}
			addQuad(corners[0], corners[1], corners[2], corners[3]);
		}

		//On envoie le contenu au GPU
		VboCube->createVboGpu();

		//On relache la mémoire CPU
		VboCube->deleteVboCpu();

		
		glFrontFace(GL_CW);

		World = new MWorld();
		World->init_world(0);
	}



	void addQuad(Point& a, Point& b, Point& c, Point& d)
	{
		addTriangle(a, b, c);
		addTriangle(c, d, a);
	}

	void addTriangle(Point& a, Point& b, Point& c)
	{
		addPoint(a);
		addPoint(b);
		addPoint(c);
	}

	void addPoint(Point& p)
	{
		VboCube->setElementValue(0, pointCount, p.x, p.y, p.z); //Sommet (lié au layout(0) du shader)
		VboCube->setElementValue(1, pointCount, p.nX, p.nY, p.nZ);   //Normale (lié au layout(1) du shader)
		VboCube->setElementValue(2, pointCount, p.u, p.v);      //UV (lié au layout(2) du shader)
		pointCount++;
	}

	void update(float elapsed)
	{
		float speed = 10.0f;

		float horizontal = 0.0f;
		if (qKeyDown)
			horizontal += -1.0f;
		if (dKeyDown)
			horizontal += 1.0f;

		float vertical = 0.0f;
		if (sKeyDown)
			vertical += -1.0f;
		if (zKeyDown)
			vertical += 1.0f;

		Renderer->Camera->move(
			(Renderer->Camera->RightVec * horizontal +
			Renderer->Camera->Direction * vertical) * speed * elapsed
		);
	}

	void renderObjects()
	{
		glUseProgram(0);

		//Rendu des axes
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		glColor3d(1, 0, 0);
		glVertex3d(0, 0, 0);
		glVertex3d(10000, 0, 0);
		glColor3d(0, 1, 0);
		glVertex3d(0, 0, 0);
		glVertex3d(0, 10000, 0);
		glColor3d(0, 0, 1);
		glVertex3d(0, 0, 0);
		glVertex3d(0, 0, 10000);
		glEnd();

		// Calcul des valeurs paramétriques pour le soleil
		int64_t fullDayMillis = computeTimeMillis(24, 0, 0, 0);
		int64_t sunRiseTime = computeTimeMillis(6, 0, 0, 0);
		int64_t sunSetTime = computeTimeMillis(19, 0, 0, 0);
		int64_t midnightTime = int64_t(sunSetTime + (fullDayMillis - (sunSetTime - sunRiseTime)) * 0.5f) % fullDayMillis;
		SYSTEMTIME sysTime;
		GetLocalTime(&sysTime);
		int64_t realTimeMillis = computeTimeMillis(sysTime);
		int64_t currentTimeMillis = realTimeMillis + timeOffset;
		currentTimeMillis %= fullDayMillis;

		float sunProgressT;
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

		// std::cout << sunProgressT << endl;

		float time = this->DeltaTimeCumul;
		float scaledTime = time * 3.0f;
		float sawTime = 1.0 - (scaledTime - int(scaledTime));
		float sinTime = std::sin(sawTime * M_PI * 0.5);

		// Calcul et mise à jour de la couleur du soleil et du ciel
		float distToZenith = std::abs(sunProgressT - 0.5);
		float distToZenithTransitionColorRange = 0.1f;
		float sunAppearanceColorT = saturate((0.5 - distToZenith) / distToZenithTransitionColorRange);

		/*
		std::cout << sunProgressT << " " << distToZenith << " "
		<< sunRiseTime << " " << sunSetTime << " " << currentTimeMillis << " " 
		<< sunAppearanceColorT << std::endl;
		*/

		YColor baseSunColor(1, 0.9, 0.25, 1);
		YColor sunAppearanceColor(1., 0.1, 0, 1);
		YColor sunColor = sunAppearanceColor.interpolate(baseSunColor, sunAppearanceColorT);
		YColor black = YColor(0, 0, 0, 1);
		sunColor = sunColor.interpolate(black, 1 - sinTime);
		
		float sunAppearanceSkyColorT = (0.5 - distToZenith) / distToZenithTransitionColorRange;
		// std::cout << sunProgressT << " " << distToZenith << " " << sunAppearanceSkyColorT << std::endl;

		YColor brightSkyColor(0.2, 0.7, 1, 1);
		YColor darkSkyColor(0, 0.05, 0.2, 1);
		YColor sunAppearanceSkyColor(0.7, 0.2, 0, 1);

		YColor skyColor;
		if (sunAppearanceSkyColorT > 0)
		{
			skyColor = sunAppearanceSkyColor.interpolate(brightSkyColor, sunAppearanceSkyColorT);
		}
		else
		{
			skyColor = sunAppearanceSkyColor.interpolate(darkSkyColor, -sunAppearanceSkyColorT);
		}
		Renderer->setBackgroundColor(skyColor);
		
		// Calcul et application de la transformation du soleil
		float scale = 0.25f + 0.125 * sinTime;

		float sunRiseAngle = 90.0f;
		float sunSetAngle = -90.0f;
		float sunAngle = lerp(sunRiseAngle, sunSetAngle, sunProgressT);
		
		glPushMatrix();
			YVec3<float> camPos = Renderer->Camera->Position;
			// glTranslatef(camPos.X, camPos.Y, camPos.Z);
			glRotatef(sunAngle, -1, 1, 0);
			// glTranslatef(0, 0, 20);
			glScalef(scale, scale, scale);
			glRotatef(-sunAngle, -1, 1, 0);

			glRotatef(360 * time * 1.25, 1, 0, 0);
			glRotatef(360 * time * 2, 0, 1, 0);
			glRotatef(360 * time * 0.75, 0, 0, 1);

			// Tracé du soleil
			glUseProgram(ShaderSun); //Demande au GPU de charger ces shaders
			GLuint var = glGetUniformLocation(ShaderSun, "sun_color");
			glUniform3f(var, sunColor.R, sunColor.V, sunColor.B);
			Renderer->updateMatricesFromOgl(); //Calcule toute les matrices à partir des deux matrices OGL
			Renderer->sendMatricesToShader(ShaderSun); //Envoie les matrices au shader
			VboCube->render(); //Demande le rendu du VBO
		glPopMatrix();

		glPushMatrix();
			World->render_world_basic(ShaderCube, VboCube);
		glPopMatrix();
	}

	int64_t computeTimeMillis(WORD hour, WORD minute, WORD second, WORD milliseconds)
	{
		return milliseconds + 1000LL * (second + 60LL * (minute + 60LL * hour));
	}

	int64_t computeTimeMillis(SYSTEMTIME &time)
	{
		return computeTimeMillis(time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	}

	void resize(int width, int height) {

	}

	float lerp(float a, float b, float t)
	{
		return a * (1.0 - t) + (b * t);
	}

	float inverseLerp(float a, float b, float x)
	{
		return (x - a) / (b - a);
	}

	float saturate(float value)
	{
		return max(0.0, min(1.0, value));
	}

	void incrementTime()
	{
		timeOffset += 20LL * 60LL * 1000LL;
	}

	/*INPUTS*/

	bool zKeyDown;
	bool qKeyDown;
	bool sKeyDown;
	bool dKeyDown;

	void keyPressed(int key, bool special, bool down, int p1, int p2)
	{
		if ((key == 'g' || key == 'G') && down)
			incrementTime();
		else if (key == 'z' || key == 'Z')
			zKeyDown = down;
		else if (key == 'q' || key == 'Q')
			qKeyDown = down;
		else if (key == 's' || key == 'S')
			sKeyDown = down;
		else if (key == 'd' || key == 'D')
			dKeyDown = down;
	}

	void mouseWheel(int wheel, int dir, int x, int y, bool inUi)
	{
		YVec3<float> lookAtToCamera = Renderer->Camera->Position - Renderer->Camera->LookAt;
		float distToLookAt = lookAtToCamera.getSize();
		float zoomMultiplier = 1.05f;
		float minDist = Renderer->Camera->Near;

		if (dir < 0)
		{
			distToLookAt *= zoomMultiplier;
		}
		else
		{
			distToLookAt /= zoomMultiplier;
			if (distToLookAt < minDist)
				distToLookAt = minDist;
		}
		
		Renderer->Camera->setPosition(
			Renderer->Camera->LookAt + lookAtToCamera.normalize() * distToLookAt
		);
	}


	void mouseClick(int button, int state, int x, int y, bool inUi)
	{
		if (button == GLUT_LEFT_BUTTON)
			leftButtonDown = (state == GLUT_DOWN);
		else if (button == GLUT_MIDDLE_BUTTON)
			middleButtonDown = (state == GLUT_DOWN);
		else if (button == GLUT_RIGHT_BUTTON)
			rightButtonDown = (state == GLUT_DOWN);


		if (state == GLUT_DOWN && (button == GLUT_RIGHT_BUTTON || button == GLUT_MIDDLE_BUTTON))
		{
			glutWarpPointer(Renderer->ScreenWidth / 2, Renderer->ScreenHeight / 2);
		}
	}

	bool leftButtonDown;
	bool middleButtonDown;
	bool rightButtonDown;

	int latestX;
	int latestY;
	void mouseMove(int x, int y, bool pressed, bool inUi)
	{
		int xDelta = Renderer->ScreenWidth / 2 - x;
		int yDelta = Renderer->ScreenHeight / 2 - y;

		if (rightButtonDown)
		{
			float rotationMultiplier = 2 * M_PI * 0.001;
			if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
			{
				Renderer->Camera->rotateAround(xDelta * rotationMultiplier);
				Renderer->Camera->rotateUpAround(yDelta * rotationMultiplier);
			}
			else
			{
				Renderer->Camera->rotate(xDelta * rotationMultiplier);
				Renderer->Camera->rotateUp(yDelta * rotationMultiplier);
			}
		}

		if (middleButtonDown)
		{
			float translationMultiplier = 0.0025f;
			if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
			{
				YVec3<float> projectedForward = Renderer->Camera->RightVec.cross(Renderer->Camera->UpRef).normalize();

				Renderer->Camera->move(
					Renderer->Camera->RightVec * xDelta * translationMultiplier +
					projectedForward * -yDelta * translationMultiplier
				);
			}
			else
			{
				Renderer->Camera->move(
					Renderer->Camera->RightVec * xDelta * translationMultiplier +
					Renderer->Camera->UpVec * -yDelta * translationMultiplier
				);
			}
		}

		if (rightButtonDown || middleButtonDown)
		{
			glutWarpPointer(Renderer->ScreenWidth / 2, Renderer->ScreenHeight / 2);
		}
	}
};


#endif