#ifndef __YOCTO__ENGINE_MINICRAFT__
#define __YOCTO__ENGINE_MINICRAFT__

#include <cmath>
#include <limits>

#include "engine/engine.h"
#include "my_physics.h"
#include "sky_renderer.h"

#include "avatar.h"
#include "world.h"
#include "atlas_uv_mapper.h"

#define SHADOW_CASCADE_COUNT 4
// taille de la shadowMap d'une seule cascade
#define SHADOWMAP_SIZE 8192

bool isPowerOfTwo(uint32 value)
{
	if (value == 0)
		return false;
	return ((value & (value - 1)) == 0);
}

class MEngineMinicraft : public YEngine {

public:
	YVbo* VboCube;
	MWorld* World;
	MAvatar* avatar;
	SkyRenderer skyRenderer;

	AtlasUVMapper* uvMapper;
	YTexFile* atlasTex;

	// Shadows
	YFbo* shadowFbos[SHADOW_CASCADE_COUNT];
	YCamera* shadowCameras[SHADOW_CASCADE_COUNT];
	YMat44 shadowCamerasVP[SHADOW_CASCADE_COUNT];
	float cascadeDepths[SHADOW_CASCADE_COUNT+1];
	float cascadeFarClipZ[SHADOW_CASCADE_COUNT];

	// SSR
	YMat44 mainCameraV;
	YMat44 mainCameraP;
	YMat44 mainCameraInvP;

	// Post Processing
	bool postProcessEnabled;
	YFbo* screenFbos[2];
	int currentScreenFboIndex;
	GLuint ShaderVignettePP;
	GLuint ShaderChromaticAberrationPP;
	GLuint ShaderWaterEffectsPP;
	GLuint ShaderGammaCorrectPP;

	// Post Processing parameters
	GUISlider *vignetteIntensity;
	GUISlider *vignetteRadius;
	GUISlider *chromaticAberrationHorizontal;
	GUISlider *chromaticAberrationVertical;
	GUISlider *waterRefractionIntensity;
	GUISlider *waterReflectionIntensity;
	
	// Main shaders
	GLuint ShaderCubeDebug;
	GLuint ShaderCube;
	GLuint ShaderSun;
	GLuint ShaderWorld;
	GLuint ShaderWorldOpaque;
	GLuint ShaderWorldWater;
	GLuint ShaderWorldWaterSimple;
	GLuint ShaderShadows;

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
		ShaderShadows = Renderer->createProgram("shaders/shadows");
		ShaderWorldOpaque = Renderer->createProgram("shaders/world_opaque");
		ShaderWorldWater = Renderer->createProgram("shaders/world_water");
		ShaderWorldWaterSimple = Renderer->createProgram("shaders/world_water_simple");
		ShaderVignettePP = Renderer->createProgram("shaders/postprocess/vignette");
		ShaderChromaticAberrationPP = Renderer->createProgram("shaders/postprocess/chromatic_aberration");
		ShaderWaterEffectsPP = Renderer->createProgram("shaders/postprocess/water_effects");
		ShaderGammaCorrectPP = Renderer->createProgram("shaders/postprocess/gamma");
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
		postProcessEnabled = true;

		YLog::log(YLog::ENGINE_INFO, "Minicraft Started : initialisation");

		Renderer->Camera->setPosition(YVec3f(10, 10, 10));

		initShadows();
		createVboCube();
		
		glFrontFace(GL_CW);

		uvMapper = new AtlasUVMapper();
		World = new MWorld(uvMapper);
		World->init_world(0);

		avatar = new MAvatar(Renderer->Camera, World);

		screenFbos[0] = new YFbo(true, 3);
		screenFbos[0]->init(Renderer->ScreenWidth, Renderer->ScreenHeight);
		screenFbos[1] = new YFbo(true, 1);
		screenFbos[1]->init(Renderer->ScreenWidth, Renderer->ScreenHeight);

		atlasTex = YTexManager::getInstance()->loadTexture("textures/atlas.png");

		initUi();
	}

	void initUi()
	{
		const uint16 spacing = 4;
		const uint16 headerX = 12;
		const uint16 paramX = headerX + 16;
		uint16 y = 36;

		addHeader("Vignette", headerX, y);
		addParamSlider("Intensity", paramX, y, 0, 1, 1.0, &vignetteIntensity);
		addParamSlider("Radius", paramX, y, 0.5, 3.0, 1.75, &vignetteRadius);
		y += spacing;

		addHeader("Chromatic Aberration", headerX, y);
		addParamSlider("Horizontal", paramX, y, 0, 2.0, 1.0, &chromaticAberrationHorizontal);
		addParamSlider("Vertical", paramX, y, 0, 2.0, 0.75, &chromaticAberrationVertical);
		y += spacing;

		addHeader("Water Effects Intensity", headerX, y);
		addParamSlider("Refraction", paramX, y, 0, 1.0, 0.25, &waterRefractionIntensity);
		addParamSlider("Reflection", paramX, y, 0, 1.0, 0.5, &waterReflectionIntensity);
	}

	void addHeader(const std::string& labelName, uint16 x, uint16& y, GUILabel **addedLabel = nullptr)
	{
		GUILabel* label = new GUILabel();
		label->Text = labelName;
		label->X = x;
		label->Y = y;
		y += label->Height;
		ScreenParams->addElement(label);
		if (addedLabel != nullptr)
			*addedLabel = label;
	}

	void addParamSlider(const std::string &labelName, uint16 x, uint16 &y, float min, float max, float startValue, GUISlider **addedSlider = nullptr, GUILabel **addedLabel = nullptr)
	{
		GUILabel *label = new GUILabel();
		label->Text = labelName;
		label->X = x;
		label->Y = y;
		x += 120;
		ScreenParams->addElement(label);
		if (addedLabel != nullptr)
			*addedLabel = label;
		
		GUISlider *slider = new GUISlider();
		slider->setPos(x, y);
		slider->setSize(120, 20);
		slider->setMaxMin(max, min);
		slider->setValue(startValue);
		y += slider->Height + 2;
		ScreenParams->addElement(slider);
		if (addedSlider != nullptr)
			*addedSlider = slider;
	}

	void initShadows()
	{
		cascadeDepths[0] = NearPlane;
		cascadeDepths[1] = lerp(NearPlane, FarPlane, 0.03f);
		cascadeDepths[2] = lerp(NearPlane, FarPlane, 0.09f);
		cascadeDepths[3] = lerp(NearPlane, FarPlane, 0.3f);
		cascadeDepths[SHADOW_CASCADE_COUNT] = FarPlane;

		//Creation des FBO
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
		{
			shadowCameras[i] = new YCamera();
			shadowFbos[i] = new YFbo(true, 1, 1.0f, false, true);
			shadowFbos[i]->init(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
		}
	}

	void createVboCube()
	{
		//Creation du VBO
		VboCube = new YVbo(3, 36, YVbo::PACK_BY_ELEMENT_TYPE);

		//D�finition du contenu du VBO
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

		//On demande d'allouer la m�moire cot� CPU
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

		//On relache la m�moire CPU
		VboCube->deleteVboCpu();
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
		VboCube->setElementValue(0, pointCount, p.x, p.y, p.z); //Sommet (li� au layout(0) du shader)
		VboCube->setElementValue(1, pointCount, p.nX, p.nY, p.nZ);   //Normale (li� au layout(1) du shader)
		VboCube->setElementValue(2, pointCount, p.u, p.v);      //UV (li� au layout(2) du shader)
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
		// Mise a jour des valeurs du soleil
		skyRenderer.updateSkyValues(timeOffset);

		// Calcul des textures d'ombres
		configureShadowCameras();
		renderShadowMaps();

		if (postProcessEnabled)
			screenFbos[0]->setAsOutFBO(true, true);


		setColorMaskEnabled(0, false);
		setSecondaryMasksEnabled(true);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		setColorMaskEnabled(0, true);
		setSecondaryMasksEnabled(false);

		// Rendu des axes
		glUseProgram(0);
		renderAxii();

		// Rendu du ciel et du soleil
		skyRenderer.render(VboCube, 1, 10);

		// Rendu de l'avatar
		renderAvatar();

		// Rendu du monde
		renderWorld();

		// Calcul et rendu de la cible de picking
		intersectionCubeSide = World->getRayCollision(Renderer->Camera->Position, Renderer->Camera->Direction, intersection, pickingRange, intersectionCubeX, intersectionCubeY, intersectionCubeZ);
		drawIntersectedCubeSide();

		if (postProcessEnabled)
		{
			// Recup des matrices pour les SSR
			Renderer->updateMatricesFromOgl();
			mainCameraV = Renderer->MatV;
			mainCameraP = Renderer->MatP;
			mainCameraInvP = Renderer->MatIP;
			screenFbos[0]->setAsOutFBO(false, false);
		}
		
		// Post Process
		currentScreenFboIndex = 0;
		postProcess();
	}

	void setColorMaskEnabled(int mask, bool enabled)
	{
		GLboolean glEnabled = enabled ? GL_TRUE : GL_FALSE;
		glColorMaski(mask, glEnabled, glEnabled, glEnabled, glEnabled);
	}

	void setSecondaryMasksEnabled(bool enabled)
	{
		for (int i = 1; i < screenFbos[0]->NbColorTex; i++)
		{
			setColorMaskEnabled(i, enabled);
		}
	}

	// Mostly adapted from : https://ogldev.org/www/tutorial49/tutorial49.html
	void configureShadowCameras()
	{
		// Initialization
		YCamera* baseCamera = Renderer->Camera;
		
		baseCamera->look();
		YMat44 baseCameraP;
		float matProjTab[16];
		glGetFloatv(GL_PROJECTION_MATRIX, matProjTab);
		memcpy(baseCameraP.Mat.t, matProjTab, 16 * sizeof(float));
		baseCameraP.transpose();

		YCamera baseCameraCopy = *baseCamera;
		baseCameraCopy.moveTo(YVec3f());
		YMat44 baseCameraCopyVInv = baseCameraCopy.getViewMatrix();
		baseCameraCopyVInv.invert();

		YCamera lightCamera;
		lightCamera.Position = YVec3f();
		lightCamera.LookAt = -skyRenderer.sunDirection;
		lightCamera.updateVecs();
		YMat44 lightCameraV = lightCamera.getViewMatrix();
		
		float aspectRatio = (float)Renderer->ScreenWidth / (float)Renderer->ScreenHeight;
		float tanHalfVerticalFOV = std::tanf(degToRad(baseCamera->FovY / 2.0f));
		float tanHalfHorizontalFOV = std::tanf(degToRad(baseCamera->FovY * aspectRatio / 2.0f));

#pragma push_macro("max")
#undef max
#pragma push_macro("min")
#undef min
		// Do the configuration
		YVec3f frustumCorners[8];
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
		{
			const float subFrustumNear = cascadeDepths[i];
			const float subFrustumFar = cascadeDepths[i+1];

			float xNear = subFrustumNear * tanHalfHorizontalFOV;
			float xFar = subFrustumFar * tanHalfHorizontalFOV;
			float yNear = subFrustumNear * tanHalfVerticalFOV;
			float yFar = subFrustumFar * tanHalfVerticalFOV;

			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();

			frustumCorners[0] = YVec3f(xNear, yNear, -subFrustumNear);
			frustumCorners[1] = YVec3f(-xNear, yNear, -subFrustumNear);
			frustumCorners[2] = YVec3f(-xNear, -yNear, -subFrustumNear); 
			frustumCorners[3] = YVec3f(xNear, -yNear, -subFrustumNear);
			frustumCorners[4] = YVec3f(xFar, yFar, -subFrustumFar);
			frustumCorners[5] = YVec3f(-xFar, yFar, -subFrustumFar);
			frustumCorners[6] = YVec3f(-xFar, -yFar, -subFrustumFar); 
			frustumCorners[7] = YVec3f(xFar, -yFar, -subFrustumFar);

			for (int j = 0; j < 8; j++)
			{
				YVec3f lightSpaceFrustumCorner = lightCameraV * (baseCameraCopyVInv * frustumCorners[j]);

				minX = std::min(minX, lightSpaceFrustumCorner.X);
				maxX = std::max(maxX, lightSpaceFrustumCorner.X);
				minY = std::min(minY, lightSpaceFrustumCorner.Y);
				maxY = std::max(maxY, lightSpaceFrustumCorner.Y);
				minZ = std::min(minZ, lightSpaceFrustumCorner.Z);
				maxZ = std::max(maxZ, lightSpaceFrustumCorner.Z);
			}

			minZ -= FarPlane;
			maxZ += FarPlane;

			shadowCameras[i]->Position = Renderer->Camera->Position;
			shadowCameras[i]->LookAt = -skyRenderer.sunDirection + Renderer->Camera->Position;
			shadowCameras[i]->updateVecs();
			shadowCameras[i]->setProjectionOrtho(minX, maxX, minY, maxY, minZ, maxZ);

			cascadeFarClipZ[i] = (baseCameraP * YVec3f(0, 0, -subFrustumFar)).Z;
		}
#pragma pop_macro("min")
#pragma pop_macro("max")
	}

	void renderShadowMaps()
	{
		YCamera* baseCamera = Renderer->Camera;

		glDrawBuffer(GL_NONE);
		glUseProgram(ShaderShadows);
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
		{
			shadowFbos[i]->setAsOutFBO(true, true);
			Renderer->Camera = shadowCameras[i];
			shadowCameras[i]->look();

			glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
			Renderer->updateMatricesFromOgl();
			Renderer->sendMatricesToShader(ShaderShadows);
			World->render_world_vbo(false, false);

			shadowCamerasVP[i] = Renderer->MatP;
			shadowCamerasVP[i] *= Renderer->MatV;
			shadowFbos[i]->setAsOutFBO(false, false);
		}
		Renderer->Camera = baseCamera;
		glDrawBuffer(GL_BACK);

		Renderer->Camera->look();
		glViewport(0, 0, Renderer->ScreenWidth, Renderer->ScreenHeight);
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
			Renderer->updateMatricesFromOgl(); //Calcule toute les matrices � partir des deux matrices OGL
			Renderer->sendMatricesToShader(ShaderCube); //Envoie les matrices au shader
			VboCube->render(); //Demande le rendu du VBO
		glPopMatrix();
	}

	void renderWorld()
	{
		glPushMatrix();
			// Opaque
			loadWorldShader(ShaderWorldOpaque);
			atlasTex->setAsShaderInput(ShaderWorldOpaque, GL_TEXTURE0, "tex_atlas");
			World->render_world_vbo(false, false);
			
			// Transparent
			setSecondaryMasksEnabled(true);

			GLuint waterShader = postProcessEnabled ? ShaderWorldWater : ShaderWorldWaterSimple;
			loadWorldShader(waterShader);
			Renderer->sendTimeToShader(DeltaTimeCumul, waterShader);
			World->render_world_vbo(false, true);

			setSecondaryMasksEnabled(false);
		glPopMatrix();
	}

	void loadWorldShader(GLuint shader)
	{
		glUseProgram(shader);
		GLuint shader_sunColor = glGetUniformLocation(shader, "sun_color");
		glUniform3f(shader_sunColor, skyRenderer.outerSunColor.R, skyRenderer.outerSunColor.V, skyRenderer.outerSunColor.B);
		GLuint shader_sunLightColor = glGetUniformLocation(shader, "sun_light_color");
		glUniform3f(shader_sunLightColor, skyRenderer.lightingSunColor.R, skyRenderer.lightingSunColor.V, skyRenderer.lightingSunColor.B);
		GLuint shader_fogColor = glGetUniformLocation(shader, "fog_color");
		glUniform3f(shader_fogColor, skyRenderer.skyColor.R, skyRenderer.skyColor.V, skyRenderer.skyColor.B);
		GLuint shader_ambientColor = glGetUniformLocation(shader, "ambient_color");
		glUniform3f(shader_ambientColor, skyRenderer.ambientColor.R, skyRenderer.ambientColor.V, skyRenderer.ambientColor.B);
		GLuint shader_sunDirection = glGetUniformLocation(shader, "sun_direction");
		glUniform3f(shader_sunDirection, skyRenderer.sunDirection.X, skyRenderer.sunDirection.Y, skyRenderer.sunDirection.Z);
		YVec3f cameraPos = Renderer->Camera->Position;
		GLuint shader_cameraPos = glGetUniformLocation(shader, "camera_pos");
		glUniform3f(shader_cameraPos, cameraPos.X, cameraPos.Y, cameraPos.Z);

		// Shadow map data
		GLuint shader_invShadowmapSize = glGetUniformLocation(shader, "inv_shadowmap_size");
		glUniform1f(shader_invShadowmapSize, 1.0 / SHADOWMAP_SIZE);
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
		{
			GLuint shader_shadowVPi = glGetUniformLocation(shader, ("shadow_vp[" + std::to_string(i) + "]").c_str());
			glUniformMatrix4fv(shader_shadowVPi, 1, true, shadowCamerasVP[i].Mat.t);
			GLuint shader_shadowCascadeFarClipZi = glGetUniformLocation(shader, ("shadow_cascade_far_clip_z[" + std::to_string(i) + "]").c_str());
			glUniform1f(shader_shadowCascadeFarClipZi, cascadeFarClipZ[i]);
			shadowFbos[i]->setDepthAsShaderInput(GL_TEXTURE1 + i, ("shadow_map[" + std::to_string(i) + "]").c_str());
		}

		Renderer->sendNearFarToShader(shader);
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

	void postProcess()
	{
		if (!postProcessEnabled)
			return;
		
		// Init
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		// Actual post processing
		// Water Effects
		initPostProcess(ShaderWaterEffectsPP);
		sendMatrixToShader(ShaderWaterEffectsPP, mainCameraV, "v");
		sendMatrixToShader(ShaderWaterEffectsPP, mainCameraP, "p");
		sendMatrixToShader(ShaderWaterEffectsPP, mainCameraInvP, "inv_p");
		sendSliderValueToShader(waterRefractionIntensity, "refraction_intensity", ShaderWaterEffectsPP);
		sendSliderValueToShader(waterReflectionIntensity, "reflection_intensity", ShaderWaterEffectsPP);
		Renderer->sendScreenDimensionsToShader(ShaderWaterEffectsPP);
		doPostProcess();
		// God Rays
		// Gamma Correction
		// doSinglePostProcess(ShaderGammaCorrectPP);
		// Chromatic Aberration
		initPostProcess(ShaderChromaticAberrationPP);
		sendSliderValueToShader(chromaticAberrationHorizontal, "horizontal_intensity", ShaderChromaticAberrationPP);
		sendSliderValueToShader(chromaticAberrationVertical, "vertical_intensity", ShaderChromaticAberrationPP);
		doPostProcess();
		// Vignette
		initPostProcess(ShaderVignettePP);
		sendSliderValueToShader(vignetteIntensity, "intensity", ShaderVignettePP);
		sendSliderValueToShader(vignetteRadius, "radius", ShaderVignettePP);
		doPostProcess(true);

		// Cleanup
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
	}

	void sendSliderValueToShader(GUISlider *slider, const char* paramName, GLuint shader)
	{
		GLuint location = glGetUniformLocation(shader, paramName);
		glUniform1f(location, slider->Value);
	}

	void initPostProcess(GLuint postProcessShader)
	{
		glUseProgram(postProcessShader);
		currentScreenFboIndex = 1 - currentScreenFboIndex;
	}

	void doPostProcess(bool isLast = false)
	{
		// Choix des FBO pour le post process
		YFbo* fboWithScreenData = screenFbos[1 - currentScreenFboIndex];
		YFbo* targetFbo = screenFbos[currentScreenFboIndex];
		
		// "Ecriture" du post process dans un FBO, ou sur l'�cran si c'est le dernier post-process
		if (!isLast)
			targetFbo->setAsOutFBO(true, false);
		
		fboWithScreenData->setColorAsShaderInput(0, GL_TEXTURE0, "TexColor");
		screenFbos[0]->setDepthAsShaderInput(GL_TEXTURE1, "TexDepth");
		screenFbos[0]->setColorAsShaderInput(1, GL_TEXTURE2, "TexNormal");
		screenFbos[0]->setColorAsShaderInput(2, GL_TEXTURE3, "TexWaterColor");
		screenFbos[0]->setColorAsShaderInput(3, GL_TEXTURE4, "TexWaterAlpha");
		
		Renderer->sendNearFarToShader(YRenderer::CURRENT_SHADER);
		Renderer->drawFullScreenQuad();

		if (!isLast)
			targetFbo->setAsOutFBO(false, false);
	}

	void sendMatrixToShader(GLuint shader, YMat44 &mat, const char *name)
	{
		GLuint location = glGetUniformLocation(shader, name);
		glUniformMatrix4fv(location, 1, true, mat.Mat.t);
	}

	void drawWireQuad(YVec3f a, YVec3f b, YVec3f c, YVec3f d, YColor color)
	{
		glColor3f(color.R, color.V, color.B);

		glBegin(GL_LINES);

		glVertex3f(a.X, a.Y, a.Z);
		glVertex3f(b.X, b.Y, b.Z);

		glVertex3f(b.X, b.Y, b.Z);
		glVertex3f(c.X, c.Y, c.Z);

		glVertex3f(c.X, c.Y, c.Z);
		glVertex3f(d.X, d.Y, d.Z);

		glVertex3f(d.X, d.Y, d.Z);
		glVertex3f(a.X, a.Y, a.Z);

		glEnd();
	}

	void resize(int width, int height) 
	{
		screenFbos[0]->resize(width, height);
		screenFbos[1]->resize(width, height);
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
			else if ((key == 'p' || key == 'P') && down)
				postProcessEnabled = !postProcessEnabled;
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