#ifndef __YOCTO__ENGINE_MINICRAFT__
#define __YOCTO__ENGINE_MINICRAFT__

#include <cmath>
#include <limits>

#include "engine/engine.h"
#include "my_physics.h"
#include "sky_renderer.h"

#include "avatar.h"
#include "world.h"

class MEngineMinicraft : public YEngine {

public:
	YVbo* VboCube;
	MWorld* World;
	MAvatar* avatar;
	SkyRenderer skyRenderer;
	GLuint ShaderCubeDebug;
	GLuint ShaderCube;
	GLuint ShaderSun;
	GLuint ShaderWorld;
	int pointCount;
	int64_t timeOffset = 0;
	float pickingRange = 3.5f;

	YVec3f intersection;
	int intersectionCubeX, intersectionCubeY, intersectionCubeZ;
	cubeSide intersectionCubeSide;

	//Gestion singleton
	static YEngine* getInstance()
	{
		if (Instance == NULL)
			Instance = new MEngineMinicraft();
		return Instance;
	}

	/*HANDLERS GENERAUX*/
	void loadShaders() 
	{
		ShaderCubeDebug = Renderer->createProgram("shaders/cube_debug");
		ShaderCube = Renderer->createProgram("shaders/cube");
		ShaderWorld = Renderer->createProgram("shaders/world");
		skyRenderer.loadShaders();
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

		avatar = new MAvatar(Renderer->Camera, World);
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
		// Camera (non-first person)
		float speed = ctrlDown ? 150.0f : 50.0f;

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

		// First person
		if (firstPerson)
		{
			avatar->gauche = qKeyDown;
			avatar->droite = dKeyDown;
			avatar->recule = sKeyDown;
			avatar->avance = zKeyDown;
			avatar->Run = firstPerson && ctrlDown;
		}

		avatar->update(elapsed);
		if (firstPerson)
			Renderer->Camera->moveTo(avatar->Position + YVec3f(0, 0, avatar->Height * 0.925 * 0.5 * MCube::CUBE_SIZE));
	}

	void renderObjects()
	{
		glUseProgram(0);

		// Rendu des axes
		renderAxii();

		// Rendu du ciel et du soleil
		skyRenderer.updateSkyValues(timeOffset);
		skyRenderer.render(VboCube, 1, 10);

		// Rendu de l'avatar
		renderAvatar();

		// Rendu du monde
		renderWorld();

		// Calcul et rendu de la cible de picking
		intersectionCubeSide = World->getRayCollision(Renderer->Camera->Position, Renderer->Camera->Direction, intersection, pickingRange, intersectionCubeX, intersectionCubeY, intersectionCubeZ);
		drawIntersectedCubeSide();
	}

	void renderAxii()
	{
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
	}

	void renderAvatar()
	{
		glPushMatrix();
			glUseProgram(ShaderCube);
			GLuint shaderCube_cubeColor = glGetUniformLocation(ShaderCube, "cube_color");
			glUniform4f(shaderCube_cubeColor, 1.0f, 1.0f, 1.0f, 1.0f);
			glTranslatef(avatar->Position.X, avatar->Position.Y, avatar->Position.Z);
			glScalef(0.5 * MCube::CUBE_SIZE, 0.5 * MCube::CUBE_SIZE, 0.5 * MCube::CUBE_SIZE);
			glScalef(avatar->Width, avatar->Width, avatar->Height);
			Renderer->updateMatricesFromOgl(); //Calcule toute les matrices à partir des deux matrices OGL
			Renderer->sendMatricesToShader(ShaderCube); //Envoie les matrices au shader
			VboCube->render(); //Demande le rendu du VBO
		glPopMatrix();
	}

	void renderWorld()
	{
		glPushMatrix();
			glUseProgram(ShaderWorld);
			GLuint shaderWorld_sunColor = glGetUniformLocation(ShaderWorld, "sun_color");
			glUniform3f(shaderWorld_sunColor, skyRenderer.lightingSunColor.R, skyRenderer.lightingSunColor.V, skyRenderer.lightingSunColor.B);
			GLuint shaderWorld_ambientColor = glGetUniformLocation(ShaderWorld, "ambient_color");
			glUniform3f(shaderWorld_ambientColor, skyRenderer.ambientColor.R, skyRenderer.ambientColor.V, skyRenderer.ambientColor.B);
			GLuint shaderWorld_sunDirection = glGetUniformLocation(ShaderWorld, "sun_direction");
			glUniform3f(shaderWorld_sunDirection, skyRenderer.sunDirection.X, skyRenderer.sunDirection.Y, skyRenderer.sunDirection.Z);

			YVec3f cameraPos = Renderer->Camera->Position;
			GLuint shaderWorld_cameraPos = glGetUniformLocation(ShaderWorld, "camera_pos");
			glUniform3f(shaderWorld_cameraPos, cameraPos.X, cameraPos.Y, cameraPos.Z);

			// Opaque
			World->render_world_vbo(false, false);
			// Transparent
			World->render_world_vbo(false, true);
		glPopMatrix();
	}

	void drawIntersectedCubeSide()
	{
		if (intersectionCubeSide)
		{
			// Compute selected face
			YVec3<int> corners[4];
			switch (intersectionCubeSide)
			{
			case NEG_X:
				corners[0] = YVec3<int>(0, 0, 0);
				corners[1] = YVec3<int>(0, 1, 0);
				corners[2] = YVec3<int>(0, 1, 1);
				corners[3] = YVec3<int>(0, 0, 1);
				break;
			case POS_X:
				corners[0] = YVec3<int>(1, 0, 0);
				corners[1] = YVec3<int>(1, 1, 0);
				corners[2] = YVec3<int>(1, 1, 1);
				corners[3] = YVec3<int>(1, 0, 1);
				break;
			case NEG_Y:
				corners[0] = YVec3<int>(0, 0, 0);
				corners[1] = YVec3<int>(1, 0, 0);
				corners[2] = YVec3<int>(1, 0, 1);
				corners[3] = YVec3<int>(0, 0, 1);
				break;
			case POS_Y:
				corners[0] = YVec3<int>(0, 1, 0);
				corners[1] = YVec3<int>(1, 1, 0);
				corners[2] = YVec3<int>(1, 1, 1);
				corners[3] = YVec3<int>(0, 1, 1);
				break;
			case NEG_Z:
				corners[0] = YVec3<int>(0, 0, 0);
				corners[1] = YVec3<int>(1, 0, 0);
				corners[2] = YVec3<int>(1, 1, 0);
				corners[3] = YVec3<int>(0, 1, 0);
				break;
			case POS_Z:
				corners[0] = YVec3<int>(0, 0, 1);
				corners[1] = YVec3<int>(1, 0, 1);
				corners[2] = YVec3<int>(1, 1, 1);
				corners[3] = YVec3<int>(0, 1, 1);
				break;
			}

			// Draw selected face
			glUseProgram(0);
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);

			glPushMatrix();
				glScalef(MCube::CUBE_SIZE, MCube::CUBE_SIZE, MCube::CUBE_SIZE);
				glTranslatef(intersectionCubeX, intersectionCubeY, intersectionCubeZ);
				for (int i = 1; i >= 0; i--)
				{
					glLineWidth(3.0f * (i + 1));
					glColor3f(i, i, i);
					glBegin(GL_LINES);

					glVertex3f(corners[0].X, corners[0].Y, corners[0].Z);
					glVertex3f(corners[1].X, corners[1].Y, corners[1].Z);

					glVertex3f(corners[1].X, corners[1].Y, corners[1].Z);
					glVertex3f(corners[2].X, corners[2].Y, corners[2].Z);

					glVertex3f(corners[2].X, corners[2].Y, corners[2].Z);
					glVertex3f(corners[3].X, corners[3].Y, corners[3].Z);

					glVertex3f(corners[3].X, corners[3].Y, corners[3].Z);
					glVertex3f(corners[0].X, corners[0].Y, corners[0].Z);

					glVertex3f(corners[0].X, corners[0].Y, corners[0].Z);
					glVertex3f(corners[2].X, corners[2].Y, corners[2].Z);

					glVertex3f(corners[1].X, corners[1].Y, corners[1].Z);
					glVertex3f(corners[3].X, corners[3].Y, corners[3].Z);

					glEnd();
				}
				glLineWidth(1.0f);
			glPopMatrix();

			glEnable(GL_DEPTH_TEST);
		}
	}

	void resize(int width, int height) {

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
	
	bool firstPerson;
	bool ctrlDown;

	void keyPressed(int key, bool special, bool down, int p1, int p2)
	{
		// std::cout << "KEY PRESS " << key << " "  << special << " " << p1 << std::endl;

		if (!special)
		{
			// OTHER KEYS
			if (key == ' ' && down)
				avatar->Jump = true;

			// LETTER KEYS
			if (ctrlDown) // for some reason, when ctrl is held down, "key"'s value is shifted by 64 for letters
				key += 64;

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
			else if ((key == 'c' || key == 'C') && down)
				firstPerson = !firstPerson;
		}
		else
		{
			// SPECIAL KEYS
			if (key == 114) // CTRL
				ctrlDown = down;
		}
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


		if (state == GLUT_DOWN)
		{
			if (button == GLUT_RIGHT_BUTTON || button == GLUT_MIDDLE_BUTTON)
			{
				glutWarpPointer(Renderer->ScreenWidth / 2, Renderer->ScreenHeight / 2);
			}
			
			if (intersectionCubeSide)
			{
				if (button == GLUT_LEFT_BUTTON)
				{
					World->deleteCube(intersectionCubeX, intersectionCubeY, intersectionCubeZ);
				}
				else if (button == GLUT_RIGHT_BUTTON)
				{
					World->placeCubeOnCubeSide(intersectionCubeX, intersectionCubeY, intersectionCubeZ, intersectionCubeSide, MCube::CUBE_LAINE_01);
				}
			}
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

		if (rightButtonDown || firstPerson)
		{
			float rotationMultiplier = float(2 * M_PI * 0.001);
			if (ctrlDown && !firstPerson)
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

		if (middleButtonDown && !firstPerson)
		{
			float translationMultiplier = 0.0025f;
			if (ctrlDown)
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

		if (rightButtonDown || firstPerson || middleButtonDown)
		{
			glutWarpPointer(Renderer->ScreenWidth / 2, Renderer->ScreenHeight / 2);
		}
	}
};


#endif