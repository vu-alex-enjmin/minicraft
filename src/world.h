#ifndef __WORLD_H__
#define __WORLD_H__

#include "external/gl/glew.h"
#include "external/gl/freeglut.h" 
#include "engine/utils/types_3d.h"
#include "cube.h"
#include "chunk.h"
#include "atlas_uv_mapper.h"
#include "engine/noise/perlin.h"
#include "customPerlin.h"
#include "random_utils.h"
#include "my_math.h"

class MWorld
{
public :
	typedef uint8 MAxis;
	static const int AXIS_X = 0b00000001;
	static const int AXIS_Y = 0b00000010;
	static const int AXIS_Z = 0b00000100;

	#ifdef _DEBUG
	static const int MAT_SIZE = 1; //en nombre de chunks
	#else
	static const int MAT_SIZE = 3; //en nombre de chunks
	#endif // DEBUG

	static const int MAT_HEIGHT = 3; //en nombre de chunks
	static const int MAT_SIZE_CUBES = (MAT_SIZE * MChunk::CHUNK_SIZE);
	static const int MAT_HEIGHT_CUBES = (MAT_HEIGHT * MChunk::CHUNK_SIZE);
	static const int MAT_SIZE_METERS = (MAT_SIZE * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE);
	static const int MAT_HEIGHT_METERS = (MAT_HEIGHT * MChunk::CHUNK_SIZE  * MCube::CUBE_SIZE);

	MChunk * Chunks[MAT_SIZE][MAT_SIZE][MAT_HEIGHT];
	
	MWorld(AtlasUVMapper *uvMapper)
	{
		//On cr�e les chunks
		for(int x=0;x<MAT_SIZE;x++)
			for(int y=0;y<MAT_SIZE;y++)
				for(int z=0;z<MAT_HEIGHT;z++)
					Chunks[x][y][z] = new MChunk(x,y,z,uvMapper);

		for(int x=0;x<MAT_SIZE;x++)
			for(int y=0;y<MAT_SIZE;y++)
				for(int z=0;z<MAT_HEIGHT;z++)
				{
					MChunk * cxPrev = NULL;
					if(x > 0)
						cxPrev = Chunks[x-1][y][z];
					MChunk * cxNext = NULL;
					if(x < MAT_SIZE-1)
						cxNext = Chunks[x+1][y][z];

					MChunk * cyPrev = NULL;
					if(y > 0)
						cyPrev = Chunks[x][y-1][z];
					MChunk * cyNext = NULL;
					if(y < MAT_SIZE-1)
						cyNext = Chunks[x][y+1][z];

					MChunk * czPrev = NULL;
					if(z > 0)
						czPrev = Chunks[x][y][z-1];
					MChunk * czNext = NULL;
					if(z < MAT_HEIGHT-1)
						czNext = Chunks[x][y][z+1];

					Chunks[x][y][z]->setVoisins(cxPrev,cxNext,cyPrev,cyNext,czPrev,czNext);
				}
	}

	inline MCube * getCube(int x, int y, int z)
	{	
		if(x < 0)x = 0;
		if(y < 0)y = 0;
		if(z < 0)z = 0;
		if(x >= MAT_SIZE * MChunk::CHUNK_SIZE) x = (MAT_SIZE * MChunk::CHUNK_SIZE)-1;
		if(y >= MAT_SIZE * MChunk::CHUNK_SIZE) y = (MAT_SIZE * MChunk::CHUNK_SIZE)-1;
		if(z >= MAT_HEIGHT * MChunk::CHUNK_SIZE) z = (MAT_HEIGHT * MChunk::CHUNK_SIZE)-1;

		return &(Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->_Cubes[x % MChunk::CHUNK_SIZE][y % MChunk::CHUNK_SIZE][z % MChunk::CHUNK_SIZE]);
	}

    void updateCube(int x, int y, int z)
	{	
		if(x < 0)x = 0;
		if(y < 0)y = 0;
		if(z < 0)z = 0;
		if(x >= MAT_SIZE * MChunk::CHUNK_SIZE)x = (MAT_SIZE * MChunk::CHUNK_SIZE)-1;
		if(y >= MAT_SIZE * MChunk::CHUNK_SIZE)y = (MAT_SIZE * MChunk::CHUNK_SIZE)-1;
		if (z >= MAT_HEIGHT * MChunk::CHUNK_SIZE)z = (MAT_HEIGHT * MChunk::CHUNK_SIZE) - 1; 
		
		Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->disableHiddenCubes();
		Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->toVbos();
		
		if ((x - 1) / MChunk::CHUNK_SIZE > 0 && (x - 1) / MChunk::CHUNK_SIZE != x / MChunk::CHUNK_SIZE) {
			Chunks[(x - 1) / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[(x - 1) / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->toVbos();
		}

		if ((x + 1) / MChunk::CHUNK_SIZE < MAT_SIZE && (x + 1) / MChunk::CHUNK_SIZE != x / MChunk::CHUNK_SIZE) {
			Chunks[(x + 1) / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[(x + 1) / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->toVbos();
		}

		if ((y - 1) / MChunk::CHUNK_SIZE > 0 && (y - 1) / MChunk::CHUNK_SIZE != y / MChunk::CHUNK_SIZE) {
			Chunks[x / MChunk::CHUNK_SIZE][(y - 1) / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[x / MChunk::CHUNK_SIZE][(y - 1) / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->toVbos();
		}

		if ((y + 1) / MChunk::CHUNK_SIZE < MAT_SIZE && (y + 1) / MChunk::CHUNK_SIZE != y / MChunk::CHUNK_SIZE) {
			Chunks[x / MChunk::CHUNK_SIZE][(y + 1) / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[x / MChunk::CHUNK_SIZE][(y + 1) / MChunk::CHUNK_SIZE][z / MChunk::CHUNK_SIZE]->toVbos();
		}

		if ((z - 1) / MChunk::CHUNK_SIZE > 0 && (z - 1) / MChunk::CHUNK_SIZE != z / MChunk::CHUNK_SIZE) {
			Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][(z - 1) / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][(z - 1) / MChunk::CHUNK_SIZE]->toVbos();
		}

		if ((z + 1) / MChunk::CHUNK_SIZE < MAT_HEIGHT && (z + 1) / MChunk::CHUNK_SIZE != z / MChunk::CHUNK_SIZE) {
			Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][(z + 1) / MChunk::CHUNK_SIZE]->disableHiddenCubes();
			Chunks[x / MChunk::CHUNK_SIZE][y / MChunk::CHUNK_SIZE][(z + 1) / MChunk::CHUNK_SIZE]->toVbos();
		}
		
	}

	void deleteCube(int x, int y, int z)
	{
		MCube * cube = getCube(x,y,z);
		cube->setType(MCube::CUBE_AIR);
		cube->setDraw(false);
		updateCube(x,y,z);	
	}
			
	void init_world(int seed)
	{
		YLog::log(YLog::USER_INFO,(toString("Creation du monde seed ")+toString(seed)).c_str());

		//Reset du monde
		for(int x=0;x<MAT_SIZE;x++)
			for(int y=0;y<MAT_SIZE;y++)
				for(int z=0;z<MAT_HEIGHT;z++)
					Chunks[x][y][z]->reset();

		//Generer ici le monde en modifiant les cubes
		//Utiliser getCubes()
		srand(seed);
		CustomPerlin noise;
		generate_base_terrain(noise, YVec3f(), seed);
		carve_caves(noise, YVec3f(), seed);

		for(int x=0;x<MAT_SIZE;x++)
			for(int y=0;y<MAT_SIZE;y++)
				for(int z=0;z<MAT_HEIGHT;z++)
					Chunks[x][y][z]->disableHiddenCubes();

		add_world_to_vbo();
	}

	void generate_base_terrain(CustomPerlin &noise, const YVec3f &offset, int seed)
	{
		int minSurfaceHeight = MAT_HEIGHT_CUBES * 0.5f;
		int maxSurfaceHeight = MAT_HEIGHT_CUBES;
		float waterHeightF = MAT_HEIGHT_CUBES * 0.53f;
		int waterHeight = waterHeightF;

		int maxSandHeight = waterHeight + 2;

		int minStoneHeight = 3;
		int maxStoneHeight = 7;

		int z;
		for (int x = 0; x < MAT_SIZE_CUBES; x++)
		{
			for (int y = 0; y < MAT_SIZE_CUBES; y++)
			{
				float heightNoiseValue = noise.sample((x + 177448), (y - 483302), 1.0f);
				heightNoiseValue = heightNoiseValue * heightNoiseValue;
				float stoneNoiseValue = noise.sampleSimple((x - 1245253) * 57.1433545, (y + 12347) * 72.1585211);

				float endHeightF = heightNoiseValue * maxSurfaceHeight + (1 - heightNoiseValue) * minSurfaceHeight;
				int endHeight = endHeightF;
				int sandHeight = maxSandHeight - ((endHeightF - endHeight) >= 0.75);
				int stoneHeight = endHeight - (stoneNoiseValue * maxStoneHeight + (1 - stoneNoiseValue) * minStoneHeight);

				for (z = endHeight - 1; z >= 0; z--)
				{
					MCube* cube = getCube(x, y, z);
					if (z <= stoneHeight)
					{
						cube->setType(MCube::CUBE_PIERRE);
					}
					else if (endHeight <= sandHeight && z <= sandHeight)
					{
						cube->setType(MCube::CUBE_SABLE_01);
					}
					else if (z < endHeight - 1)
					{
						cube->setType(MCube::CUBE_TERRE);
					}
					else
					{
						cube->setType(MCube::CUBE_HERBE);
					}
				}

				for (z = endHeight; z <= waterHeight; z++)
				{
					MCube* cube = getCube(x, y, z);
					cube->setType(MCube::CUBE_EAU);
				}
			}
		}
	}

	void carve_caves(CustomPerlin &noise, const YVec3f &offset, const int seed)
	{
		float noiseValueGrid[MAT_SIZE_CUBES][MAT_SIZE_CUBES];

		float baseWormNoiseScale = 2.172339f;;
		for (int x = 0; x < MAT_SIZE_CUBES; x++)
		{
			for (int y = 0; y < MAT_SIZE_CUBES; y++)
			{
				noiseValueGrid[x][y] = noise.sample((x - 327448) * baseWormNoiseScale, (y + 13302) * baseWormNoiseScale, 1.0f);
			}
		}

		std::vector<YVec3f> wormTrajectory;

		for (int x = 1; x < MAT_SIZE_CUBES - 1; x++)
		{
			for (int y = 1; y < MAT_SIZE_CUBES - 1; y++)
			{
				// Only create worms at noise local maximas
				if (noiseValueGrid[x][y] <= noiseValueGrid[x - 1][y])
					continue;
				if (noiseValueGrid[x][y] <= noiseValueGrid[x + 1][y])
					continue;
				if (noiseValueGrid[x][y] <= noiseValueGrid[x][y - 1])
					continue;
				if (noiseValueGrid[x][y] <= noiseValueGrid[x][y + 1])
					continue;

				// Compute worm trajectory
				wormTrajectory.clear();
				fillPerlinWormTrajectory(x, y, offset.X, offset.Y, seed, noise, wormTrajectory);

				// Check that trajectory does not hit water
				bool wormTrajectoryIsValid = true;
				for (int i = 0; i < wormTrajectory.size(); i++)
				{
					YVec3f& point = wormTrajectory[i];
					int pX = std::lroundf(point.X);
					int pY = std::lroundf(point.Y);
					int pZ = std::lroundf(point.Z);

					if (pX < 0 || pX >= MAT_SIZE_CUBES || pY < 0 || pY >= MAT_SIZE_CUBES)
						continue;

					MCube* cube = getCube(pX, pY, pZ);
					if (cube->getType() == MCube::CUBE_EAU) 
					{
						wormTrajectoryIsValid = false;
						break;
					}
				}
				if (!wormTrajectoryIsValid)
					continue;

				// Carve using the worm's trajectory
				carveWithWormTrajectory(wormTrajectory, noise);
			}
		}
	}

	void fillPerlinWormTrajectory(const int x, const int y, const int offsetX, const int offsetY, const int seed, CustomPerlin &noise, std::vector<YVec3f> &outTrajectory)
	{
		// Parameters
		static const float minXAngle = degToRad(-70.0f);
		static const float maxXAngle = degToRad(-15.0f);

		static const float maxXAngleChange = degToRad(15.0f);
		static const float maxZAngleChange = degToRad(36.0f);

		static const float stepLength = 1.125f;

		// Initialize worm
		// > Generate unique seed for worm's RNG
		int wormSeed = (x + offsetX) * 148867;
		wormSeed ^= (wormSeed >> 8);
		wormSeed *= 4895351;
		wormSeed += seed;
		wormSeed ^= (wormSeed << 8);
		wormSeed *= 878023;
		wormSeed += y + offsetY;
		wormSeed ^= (wormSeed >> 8);
		srand(wormSeed);

		// > Compute worm start point
		float startZNormalized = randomFloat(0.25, 0.95);
		startZNormalized *= startZNormalized;
		int startZ = (int)(startZNormalized * (MAT_HEIGHT_CUBES + 1));
		YVec3f startPoint(x, y, startZ);
		outTrajectory.push_back(startPoint);

		// > Compute worm angle and offset for angle noise
		float xNoiseOffset1 = randomFloat(-100000, 100000);
		float xNoiseOffset2 = randomFloat(-100000, 100000);
		float zNoiseOffset1 = randomFloat(-100000, 100000);
		float zNoiseOffset2 = randomFloat(-100000, 100000);

		float xAngle = lerp(minXAngle, maxXAngle, 0.5);
		float zAngle = lerp(-M_PI, M_PI, noise.sample(zNoiseOffset1, zNoiseOffset2, 1.0));

		// Move the worm until it dies
		YVec3f xAxis = YVec3f(1, 0, 0);
		YVec3f zAxis = YVec3f(0, 0, 1);

		YVec3f currentPoint = startPoint;
		float survivalChance = 1.0f;
		while ((currentPoint.Z > 0) && (survivalChance >= randomFloat01()))
		{
			// Add new step to worm trajectory
			YVec3f step = YVec3f(0, 1, 0).rotate(xAxis, xAngle).rotate(zAxis, zAngle) * stepLength;
			currentPoint += step;
			outTrajectory.push_back(currentPoint);

			// Add offset to angle noise (arbitrary)
			xNoiseOffset1 += 1.219f;
			xNoiseOffset2 += 0.737f;
			zNoiseOffset1 += 0.6723f;
			zNoiseOffset2 += 0.93541f;

			// Compute new angle
			float xNoiseValue = noise.sample(xNoiseOffset1 * 23.137589, xNoiseOffset2 * 37.124755, 0.65f);
			float zNoiseValue = noise.sample(zNoiseOffset1 * 29.332471, zNoiseOffset2 * 19.175897, 0.65f);

			float xAngleChange = lerp(-maxXAngleChange, maxXAngleChange, xNoiseValue);
			float zAngleChange = lerp(-maxZAngleChange, maxZAngleChange, zNoiseValue);

			xAngle = clamp(xAngle + xAngleChange, minXAngle, maxXAngle);
			zAngle += zAngleChange;

			survivalChance *= 0.9995f + 0.0005f * currentPoint.Z / MAT_HEIGHT_CUBES;
		}
	}

	void carveWithWormTrajectory(std::vector<YVec3f> &wormTrajectory, CustomPerlin &noise)
	{
		// > Compute carving parameters
		static const float wormMaxRadius = 4.5f;
		static const float wormMinRadius = 0.75f;
		static const float wormMiddleRadius = lerp(wormMinRadius, wormMaxRadius, 0.5);
		static const float radiusNoiseInfluence = 0.75f;

		float currentWormMinRadius = lerp(wormMinRadius, wormMiddleRadius, randomFloat(0.0f, 0.5f));
		float currentWormMaxRadius = lerp(wormMiddleRadius, wormMaxRadius, randomFloat(0.5f, 1.0f));

		int trajectoryLength = wormTrajectory.size();
		for (int i = 0; i < trajectoryLength; i++)
		{
			YVec3f &point = wormTrajectory[i];

			// Compute a base [0-1] value that is thinner on the two ends of the trajectory
			float radiusT = -4 * (float(i) / trajectoryLength - 0.5f) * (float(i) / trajectoryLength - 0.5f) + 1;
			// Add noise to the value
			float noiseValue = noise.sample(
				point.X * 37.2237143,
				point.Y * 21.7521991,
				point.Z * 53.124717,
				0.65f
			);
			float noisyRadiusT = lerp(radiusT, noiseValue, radiusNoiseInfluence);
			float noisyRadius = lerp(currentWormMinRadius, currentWormMaxRadius, noisyRadiusT);
			float sqrNoisyRadius = noisyRadius * noisyRadius;

			int minX = max(0, std::floorl(point.X - noisyRadius));
			int minY = max(0, std::floorl(point.Y - noisyRadius));
			int minZ = max(0, std::floorl(point.Z - noisyRadius));

			int maxX = min(MAT_SIZE_CUBES - 1, std::ceill(point.X + noisyRadius));
			int maxY = min(MAT_SIZE_CUBES - 1, std::ceill(point.Y + noisyRadius));
			int maxZ = min(MAT_HEIGHT_CUBES - 1, std::ceill(point.Z + noisyRadius));

			float diffX, diffY, diffZ;
			for (int x = minX; x <= maxX; x++)
			{
				for (int y = minY; y <= maxY; y++)
				{
					for (int z = minZ; z <= maxZ; z++)
					{
						diffX = point.X - x;
						diffY = point.Y - y;
						diffZ = point.Z - z;

						if ((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ) <= sqrNoisyRadius)
						{
							MCube *cube = getCube(x, y, z);
							if (cube->getType() != MCube::CUBE_EAU)
								cube->setType(MCube::CUBE_AIR);
						}
					}
				}
			}
		}
	}

	void add_world_to_vbo(void)
	{
		for (int x = 0; x<MAT_SIZE; x++)
			for (int y = 0; y<MAT_SIZE; y++)
				for (int z = 0; z<MAT_HEIGHT; z++)
				{
					Chunks[x][y][z]->toVbos();
				}
	}
	
	//Boites de collisions plus petites que deux cubes
	MAxis getMinCol(YVec3f pos, YVec3f dir, float width, float height, float & valueColMin, bool oneShot)
	{
		int x = (int)(pos.X / MCube::CUBE_SIZE);
		int y = (int)(pos.Y / MCube::CUBE_SIZE);
		int z = (int)(pos.Z / MCube::CUBE_SIZE);

		int xNext = (int)((pos.X + width / 2.0f) / MCube::CUBE_SIZE);
		int yNext = (int)((pos.Y + width / 2.0f) / MCube::CUBE_SIZE);
		int zNext = (int)((pos.Z + height / 2.0f) / MCube::CUBE_SIZE);

		int xPrev = (int)((pos.X - width / 2.0f) / MCube::CUBE_SIZE);
		int yPrev = (int)((pos.Y - width / 2.0f) / MCube::CUBE_SIZE);
		int zPrev = (int)((pos.Z - height / 2.0f) / MCube::CUBE_SIZE);

		if (x < 0)	x = 0;
		if (y < 0)	y = 0;
		if (z < 0)	z = 0;

		if (xPrev < 0)	xPrev = 0;
		if (yPrev < 0)	yPrev = 0;
		if (zPrev < 0)	zPrev = 0;

		if (xNext < 0)	xNext = 0;
		if (yNext < 0)	yNext = 0;
		if (zNext < 0)	zNext = 0;

		if (x >= MAT_SIZE_CUBES)	x = MAT_SIZE_CUBES - 1;
		if (y >= MAT_SIZE_CUBES)	y = MAT_SIZE_CUBES - 1;
		if (z >= MAT_HEIGHT_CUBES)	z = MAT_HEIGHT_CUBES - 1;

		if (xPrev >= MAT_SIZE_CUBES)	xPrev = MAT_SIZE_CUBES - 1;
		if (yPrev >= MAT_SIZE_CUBES)	yPrev = MAT_SIZE_CUBES - 1;
		if (zPrev >= MAT_HEIGHT_CUBES)	zPrev = MAT_HEIGHT_CUBES - 1;

		if (xNext >= MAT_SIZE_CUBES)	xNext = MAT_SIZE_CUBES - 1;
		if (yNext >= MAT_SIZE_CUBES)	yNext = MAT_SIZE_CUBES - 1;
		if (zNext >= MAT_HEIGHT_CUBES)	zNext = MAT_HEIGHT_CUBES - 1;

		//On fait chaque axe
		MAxis axis = 0x00;
		valueColMin = oneShot ? 0.5f : 10000.0f;
		float seuil = 0.0000001f;
		float prodScalMin = 1.0f;
		if (dir.getSqrSize() > 1)
			dir.normalize();

		//On verif tout les 4 angles de gauche
		if (getCube(xPrev, yPrev, zPrev)->isSolid() ||
			getCube(xPrev, yPrev, zNext)->isSolid() ||
			getCube(xPrev, yNext, zPrev)->isSolid() ||
			getCube(xPrev, yNext, zNext)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xPrev + 1, yPrev, zPrev)->isSolid() ||
				getCube(xPrev + 1, yPrev, zNext)->isSolid() ||
				getCube(xPrev + 1, yNext, zPrev)->isSolid() ||
				getCube(xPrev + 1, yNext, zNext)->isSolid()) || !oneShot)
			{
				float depassement = ((xPrev + 1) * MCube::CUBE_SIZE) - (pos.X - width / 2.0f);
				float prodScal = abs(dir.X);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_X;
					}
			}
		}

		//float depassementx2 = (xNext * NYCube::CUBE_SIZE) - (pos.X + width / 2.0f);

		//On verif tout les 4 angles de droite
		if (getCube(xNext, yPrev, zPrev)->isSolid() ||
			getCube(xNext, yPrev, zNext)->isSolid() ||
			getCube(xNext, yNext, zPrev)->isSolid() ||
			getCube(xNext, yNext, zNext)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xNext - 1, yPrev, zPrev)->isSolid() ||
				getCube(xNext - 1, yPrev, zNext)->isSolid() ||
				getCube(xNext - 1, yNext, zPrev)->isSolid() ||
				getCube(xNext - 1, yNext, zNext)->isSolid()) || !oneShot)
			{
				float depassement = (xNext * MCube::CUBE_SIZE) - (pos.X + width / 2.0f);
				float prodScal = abs(dir.X);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_X;
					}
			}
		}

		//float depassementy1 = (yNext * NYCube::CUBE_SIZE) - (pos.Y + width / 2.0f);

		//On verif tout les 4 angles de devant
		if (getCube(xPrev, yNext, zPrev)->isSolid() ||
			getCube(xPrev, yNext, zNext)->isSolid() ||
			getCube(xNext, yNext, zPrev)->isSolid() ||
			getCube(xNext, yNext, zNext)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xPrev, yNext - 1, zPrev)->isSolid() ||
				getCube(xPrev, yNext - 1, zNext)->isSolid() ||
				getCube(xNext, yNext - 1, zPrev)->isSolid() ||
				getCube(xNext, yNext - 1, zNext)->isSolid()) || !oneShot)
			{
				float depassement = (yNext * MCube::CUBE_SIZE) - (pos.Y + width / 2.0f);
				float prodScal = abs(dir.Y);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_Y;
					}
			}
		}

		//float depassementy2 = ((yPrev + 1) * NYCube::CUBE_SIZE) - (pos.Y - width / 2.0f);

		//On verif tout les 4 angles de derriere
		if (getCube(xPrev, yPrev, zPrev)->isSolid() ||
			getCube(xPrev, yPrev, zNext)->isSolid() ||
			getCube(xNext, yPrev, zPrev)->isSolid() ||
			getCube(xNext, yPrev, zNext)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xPrev, yPrev + 1, zPrev)->isSolid() ||
				getCube(xPrev, yPrev + 1, zNext)->isSolid() ||
				getCube(xNext, yPrev + 1, zPrev)->isSolid() ||
				getCube(xNext, yPrev + 1, zNext)->isSolid()) || !oneShot)
			{
				float depassement = ((yPrev + 1) * MCube::CUBE_SIZE) - (pos.Y - width / 2.0f);
				float prodScal = abs(dir.Y);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_Y;
					}
			}
		}

		//On verif tout les 4 angles du haut
		if (getCube(xPrev, yPrev, zNext)->isSolid() ||
			getCube(xPrev, yNext, zNext)->isSolid() ||
			getCube(xNext, yPrev, zNext)->isSolid() ||
			getCube(xNext, yNext, zNext)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xPrev, yPrev, zNext - 1)->isSolid() ||
				getCube(xPrev, yNext, zNext - 1)->isSolid() ||
				getCube(xNext, yPrev, zNext - 1)->isSolid() ||
				getCube(xNext, yNext, zNext - 1)->isSolid()) || !oneShot)
			{
				float depassement = (zNext * MCube::CUBE_SIZE) - (pos.Z + height / 2.0f);
				float prodScal = abs(dir.Z);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_Z;
					}
			}
		}

		//On verif tout les 4 angles du bas
		if (getCube(xPrev, yPrev, zPrev)->isSolid() ||
			getCube(xPrev, yNext, zPrev)->isSolid() ||
			getCube(xNext, yPrev, zPrev)->isSolid() ||
			getCube(xNext, yNext, zPrev)->isSolid())
		{
			//On verif que resoudre cette collision est utile
			if (!(getCube(xPrev, yPrev, zPrev + 1)->isSolid() ||
				getCube(xPrev, yNext, zPrev + 1)->isSolid() ||
				getCube(xNext, yPrev, zPrev + 1)->isSolid() ||
				getCube(xNext, yNext, zPrev + 1)->isSolid()) || !oneShot)
			{
				float depassement = ((zPrev + 1) * MCube::CUBE_SIZE) - (pos.Z - height / 2.0f);
				float prodScal = abs(dir.Z);
				if (abs(depassement) > seuil)
					if (abs(depassement) < abs(valueColMin))
					{
						prodScalMin = prodScal;
						valueColMin = depassement;
						axis = AXIS_Z;
					}
			}
		}

		return axis;
	}
		
	void render_world_basic(GLuint shader, YVbo * vboCube) 
	{
		glPushMatrix();
		glUseProgram(shader);
		
		GLuint cubeColor = glGetUniformLocation(shader, "cube_color");
		for (int z = 0; z < MAT_HEIGHT_CUBES; z++)
			for (int y = 0; y < MAT_SIZE_CUBES; y++)
				for (int x = 0; x < MAT_SIZE_CUBES; x++)
				{
					MCube* cube = getCube(x, y, z);
					if (cube->getType() != MCube::CUBE_AIR)
					{
						glPushMatrix();
						glTranslatef(x * 2, y * 2, z * 2);
						YRenderer::getInstance()->updateMatricesFromOgl();
						YRenderer::getInstance()->sendMatricesToShader(shader);
						
						if (cube->getType() == MCube::CUBE_TERRE)
							glUniform4f(cubeColor, 40.0f / 255.0f, 25.0f / 255.0f, 0.0f, 1.0f);
						else if (cube->getType() == MCube::CUBE_HERBE)
							glUniform4f(cubeColor, 0.0f, 1.0f, 0.1f, 1.0f);
						else if (cube->getType() == MCube::CUBE_EAU)
							glUniform4f(cubeColor, 0.0f, 0.1f, 1.0f, 1.0f);

						vboCube->render();
						glPopMatrix();
					}
				}

		glPopMatrix();
	}

	void render_world_vbo(bool debug, bool doTransparent)
	{
		if (!doTransparent)
		{
			glDisable(GL_BLEND);
			//Dessiner les chunks opaques
			for (int x = 0; x < MAT_SIZE; x++)
				for (int y = 0; y < MAT_SIZE; y++)
					for (int z = 0; z < MAT_HEIGHT; z++)
					{
						glPushMatrix();
						glTranslatef(
							x * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE,
							y * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE,
							z * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE
						);
						YRenderer::getInstance()->updateMatricesFromOgl();
						YRenderer::getInstance()->sendMatricesToShader(YRenderer::CURRENT_SHADER);
						Chunks[x][y][z]->render(false);
						glPopMatrix();
					}
		}
		else
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ZERO);
			glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//Dessiner les chunks transparents
			for (int x = 0; x < MAT_SIZE; x++)
				for (int y = 0; y < MAT_SIZE; y++)
					for (int z = 0; z < MAT_HEIGHT; z++)
					{
						glPushMatrix();
						glTranslatef(
							x * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE,
							y * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE,
							z * MChunk::CHUNK_SIZE * MCube::CUBE_SIZE
						);
						YRenderer::getInstance()->updateMatricesFromOgl();
						YRenderer::getInstance()->sendMatricesToShader(YRenderer::CURRENT_SHADER);
						Chunks[x][y][z]->render(true);
						glPopMatrix();
					}
			glDisable(GL_BLEND);
		}
	}

	/**
	* Attention ce code n'est pas optimal, il est compr�hensible. Il existe de nombreuses
	* versions optimis�es de ce calcul.
	*/
	inline bool intersecDroitePlan(const YVec3f & debSegment, const  YVec3f & finSegment,
		const YVec3f & p1Plan, const YVec3f & p2Plan, const YVec3f & p3Plan,
		YVec3f & inter)
	{
		
		return true;
	}

	/**
	* Attention ce code n'est pas optimal, il est compr�hensible. Il existe de nombreuses
	* versions optimis�es de ce calcul. Il faut donner les points dans l'ordre (CW ou CCW)
	*/
	inline bool intersecDroiteCubeFace(const YVec3f & debSegment, const YVec3f & finSegment,
		const YVec3f & p1, const YVec3f & p2, const YVec3f & p3, const  YVec3f & p4,
		YVec3f & inter)
	{
		
		return false;
	}

	cubeSide getRayCollision(
		const YVec3f & debSegment, const YVec3f & direction, YVec3f & inter, const float maxDist,
		int &xCube, int &yCube, int &zCube)
	{
		YVec3f adjustedOrigin = debSegment / MCube::CUBE_SIZE;

		int minX = max(0, std::floorl(adjustedOrigin.X - maxDist));
		int minY = max(0, std::floorl(adjustedOrigin.Y - maxDist));
		int minZ = max(0, std::floorl(adjustedOrigin.Z - maxDist));

		int maxX = min(MAT_SIZE_CUBES - 1, std::ceill(adjustedOrigin.X + maxDist));
		int maxY = min(MAT_SIZE_CUBES - 1, std::ceill(adjustedOrigin.Y + maxDist));
		int maxZ = min(MAT_HEIGHT_CUBES - 1, std::ceill(adjustedOrigin.Z + maxDist));
		
		float bestIntersectionT = std::numeric_limits<float>::infinity();
		cubeSide bestIntersectionSide = cubeSide::NONE;

		YVec3f currentIntersection;
		float currentIntersectionT;
		cubeSide currentTntersectionSide;
		
		for (int x = minX; x <= maxX; x++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				for (int z = minZ; z <= maxZ; z++)
				{
					if (getCube(x, y, z)->isSolid())
					{
						YVec3f toCube = YVec3f(x + 0.5, y + 0.5, z + 0.5) - adjustedOrigin;
						if (toCube.dot(direction) >= 0.0f) // Ignore les cubes derri�re le joueur
						{
							currentTntersectionSide = lineUnitCubeIntersection(adjustedOrigin, direction, x, y, z, currentIntersection, currentIntersectionT, maxDist);
							if (currentTntersectionSide)
							{
								if (currentIntersectionT < bestIntersectionT)
								{
									bestIntersectionT = currentIntersectionT;
									bestIntersectionSide = currentTntersectionSide;
									inter = currentIntersection;
									xCube = x;
									yCube = y;
									zCube = z;
								}
							}
						}
					}
				}
			}
		}

		if (bestIntersectionSide)
		{
			inter *= MCube::CUBE_SIZE;
		}

		return bestIntersectionSide;
	}

	bool placeCubeOnCubeSide(const int cubeX, const int cubeY, const int cubeZ, const cubeSide side, const MCube::MCubeType type)
	{
		if (side == NONE || type == MCube::CUBE_AIR)
			return false;

		int targetX, targetY, targetZ;
		if (!getCubePositionOnCubeSide(cubeX, cubeY, cubeZ, side, targetX, targetY, targetZ))
			return false;

		MCube *cube = getCube(targetX, targetY, targetZ);
		if (cube->getType() != MCube::CUBE_AIR)
			return false;

		cube->setType(type);
		cube->setDraw(true);
		updateCube(targetX, targetY, targetZ);

		return true;
	}

	bool getCubePositionOnCubeSide(
		const int cubeX, const int cubeY, const int cubeZ, const cubeSide side,
		int &outCubeX, int &outCubeY, int &outCubeZ)
	{
		outCubeX = cubeX;
		outCubeY = cubeY;
		outCubeZ = cubeZ;
		switch (side)
		{
			case NEG_X:
				outCubeX--;
				break;
			case POS_X:
				outCubeX++;
				break;
			case NEG_Y:
				outCubeY--;
				break;
			case POS_Y:
				outCubeY++;
				break;
			case NEG_Z:
				outCubeZ--;
				break;
			case POS_Z:
				outCubeZ++;
				break;
		}

		return !((outCubeX < 0) || (outCubeY < 0) || (outCubeZ < 0) ||
			(outCubeX >= MAT_SIZE_CUBES) || (outCubeY >= MAT_SIZE_CUBES) || (outCubeZ >= MAT_HEIGHT_CUBES));
	}

	/**
	* De meme cette fonction peut �tre grandement opitimis�e, on a priviligi� la clart�
	*/
	bool getRayCollisionWithCube(const YVec3f & debSegment, const YVec3f & finSegment,
		int x, int y, int z,
		YVec3f & inter)
	{

		return true;
	}
};



#endif