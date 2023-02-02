#pragma once

#include "engine/render/renderer.h"
#include "engine/render/vbo.h"
#include "cube.h"

#define FOUR_CORNERS(a,b,c,d) corners[a],corners[d],corners[c],corners[b]

/**
  * On utilise des chunks pour que si on modifie juste un cube, on ait pas
  * besoin de recharger toute la carte dans le buffer, mais juste le chunk en question
  */
class MChunk
{
	public :

		static const int CHUNK_SIZE = 64; ///< Taille d'un chunk en nombre de cubes (n*n*n)
		MCube _Cubes[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE]; ///< Cubes contenus dans le chunk

		YVbo * VboOpaque = NULL;
		YVbo * VboTransparent = NULL;

		MChunk * Voisins[6];

		int _XPos, _YPos, _ZPos; ///< Position du chunk dans le monde

		MChunk(int x, int y, int z)
		{
			memset(Voisins, 0x00, sizeof(void*)* 6);
			_XPos = x;
			_YPos = y;
			_ZPos = z;
		}

		/*
		Creation des VBO
		*/

		//On met le chunk ddans son VBO
		void toVbos(void)
		{
			SAFEDELETE(VboOpaque);
			SAFEDELETE(VboTransparent);

			//Compter les sommets
			int opaqueVertexCount = 0;
			int transparentVertexCount = 0;
			MCube* neighbours[6];
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				for (int y = 0; y < CHUNK_SIZE; y++)
				{
					for (int z = 0; z < CHUNK_SIZE; z++)
					{
						MCube::MCubeType cubeType = _Cubes[x][y][z].getType();
						if (cubeType != MCube::CUBE_AIR)
						{
							get_surrounding_cubes(x, y, z, &neighbours[0], &neighbours[1], &neighbours[2], &neighbours[3], &neighbours[4], &neighbours[5]);

							if (typeIsTransparent(cubeType))
							{
								for (int i = 0; i < 6; i++)
								{
									if (neighbours[i] == NULL || neighbours[i]->getType() == MCube::CUBE_AIR)
										transparentVertexCount += 6;
								}
							}
							else
							{
								for (int i = 0; i < 6; i++)
								{
									if (neighbours[i] == NULL || neighbours[i]->getType() == MCube::CUBE_AIR || typeIsTransparent(neighbours[i]->getType()))
										opaqueVertexCount += 6;
								}
							}
						}
					}
				}
			}

			//Créer les VBO
			VboOpaque = new YVbo(4, opaqueVertexCount, YVbo::PACK_BY_ELEMENT_TYPE);
			VboOpaque->setElementDescription(0, YVbo::Element(3)); //Sommet
			VboOpaque->setElementDescription(1, YVbo::Element(3)); //Normale
			VboOpaque->setElementDescription(2, YVbo::Element(2)); //UV
			VboOpaque->setElementDescription(3, YVbo::Element(1)); //Type
			VboOpaque->createVboCpu();

			VboTransparent = new YVbo(4, transparentVertexCount, YVbo::PACK_BY_ELEMENT_TYPE);
			VboTransparent->setElementDescription(0, YVbo::Element(3)); //Sommet
			VboTransparent->setElementDescription(1, YVbo::Element(3)); //Normale
			VboTransparent->setElementDescription(2, YVbo::Element(2)); //UV
			VboTransparent->setElementDescription(3, YVbo::Element(1)); //Type
			VboTransparent->createVboCpu();

			//Remplir les VBO
			int transparentVertexIndex = 0;
			int opaqueVertexIndex = 0;
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				for (int y = 0; y < CHUNK_SIZE; y++)
				{
					for (int z = 0; z < CHUNK_SIZE; z++)
					{
						MCube::MCubeType cubeType = _Cubes[x][y][z].getType();
						if (cubeType != MCube::CUBE_AIR)
						{
							if (typeIsTransparent(cubeType))
								transparentVertexIndex += addCubeToVbo(VboTransparent, transparentVertexIndex, x, y, z, true);
							else
								opaqueVertexIndex += addCubeToVbo(VboOpaque, opaqueVertexIndex, x, y, z, false);
						}
					}
				}
			}

			//On envoie le contenu au GPU
			VboOpaque->createVboGpu();
			VboTransparent->createVboGpu();

			//On relache la mémoire CPU
			VboOpaque->deleteVboCpu();
			VboTransparent->deleteVboCpu();
		}

		//Ajoute un quad du cube. Attention CCW
		int addQuadToVbo(YVbo * vbo, int iVertice, YVec3f & a, YVec3f & b, YVec3f & c, YVec3f & d, float type) {
			YVec3f n = (a - b).cross(c - b).normalize();

			// Premier triangle
			vbo->setElementValue(0, iVertice, a.X, a.Y, a.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 0, 1); // UV
			vbo->setElementValue(3, iVertice, type); // type

			iVertice++;
			vbo->setElementValue(0, iVertice, b.X, b.Y, b.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 1, 1); // UV
			vbo->setElementValue(3, iVertice, type); // type

			iVertice++;
			vbo->setElementValue(0, iVertice, c.X, c.Y, c.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 1, 0); // UV
			vbo->setElementValue(3, iVertice, type); // type

			// Second triangle
			iVertice++;
			vbo->setElementValue(0, iVertice, c.X, c.Y, c.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 1, 0); // UV
			vbo->setElementValue(3, iVertice, type); // type

			iVertice++;
			vbo->setElementValue(0, iVertice, d.X, d.Y, d.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 0, 0); // UV
			vbo->setElementValue(3, iVertice, type); // type

			iVertice++;
			vbo->setElementValue(0, iVertice, a.X, a.Y, a.Z); // Sommet
			vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
			vbo->setElementValue(2, iVertice, 0, 1); // UV
			vbo->setElementValue(3, iVertice, type); // type

			return 6;
		}

		int addCubeToVbo(YVbo* vbo, int iVertice, int x, int y, int z, bool currentTypeIsTransparent) 
		{
			MCube &cube = _Cubes[x][y][z];
			YVec3f corners[8]
			{
				// BOTTOM
				YVec3f(x, y, z) * MCube::CUBE_SIZE,
				YVec3f(x + 1, y, z) * MCube::CUBE_SIZE,
				YVec3f(x + 1, y + 1, z) * MCube::CUBE_SIZE,
				YVec3f(x, y + 1, z) * MCube::CUBE_SIZE,
				// TOP
				YVec3f(x, y, z + 1) * MCube::CUBE_SIZE,
				YVec3f(x + 1, y, z + 1) * MCube::CUBE_SIZE,
				YVec3f(x + 1, y + 1, z + 1) * MCube::CUBE_SIZE,
				YVec3f(x, y + 1, z + 1) * MCube::CUBE_SIZE
			};
			int vertexCount = 0;
			float type = cube.getType();

			MCube *xPrev, *xNext, *yPrev, *yNext, *zPrev, *zNext;
			get_surrounding_cubes(x, y, z, &xPrev, &xNext, &yPrev, &yNext, &zPrev, &zNext);
			
			// LEFT (-X)
			if (faceShouldBeVisible(currentTypeIsTransparent, xPrev))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(4, 7, 3, 0),
					type
				);
			}
			
			// RIGHT (+X)
			if (faceShouldBeVisible(currentTypeIsTransparent, xNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(6, 5, 1, 2),
					type
				);
			}
			
			// BACK (-Y)
			if (faceShouldBeVisible(currentTypeIsTransparent, yPrev))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(5, 4, 0, 1),
					type
				);
			}

			// FRONT (+Y)
			if (faceShouldBeVisible(currentTypeIsTransparent, yNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(7, 6, 2, 3),
					type
				);
			}

			// BOTTOM (-Z)
			if (faceShouldBeVisible(currentTypeIsTransparent, zPrev))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(1, 0, 3, 2),
					type
				);
			}

			// UP (+Z)
			if (faceShouldBeVisible(currentTypeIsTransparent, zNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(4, 5, 6, 7),
					type
				);
			}

			return vertexCount;
		}

		bool faceShouldBeVisible(bool currentTypeIsTransparent, MCube* neighbour)
		{
			if (currentTypeIsTransparent)
			{
				return (neighbour == NULL || neighbour->getType() == MCube::CUBE_AIR);
			}
			else
			{
				return (neighbour == NULL || neighbour->getType() == MCube::CUBE_AIR || typeIsTransparent(neighbour->getType()));
			}
		}

		bool typeIsTransparent(MCube::MCubeType type)
		{
			return (type == MCube::CUBE_EAU);
		}

		//Permet de compter les triangles ou des les ajouter aux VBO
		void foreachVisibleTriangle(bool countOnly, int * nbVertOpaque, int * nbVertTransp, YVbo * VboOpaque, YVbo * VboTrasparent) {


		}

		/*
		Gestion du chunk
		*/

		void reset(void)
		{
			for(int x=0;x<CHUNK_SIZE;x++)
				for(int y=0;y<CHUNK_SIZE;y++)
					for(int z=0;z<CHUNK_SIZE;z++)
					{
						_Cubes[x][y][z].setDraw(false);
						_Cubes[x][y][z].setType(MCube::CUBE_AIR);
					}
		}

		void setVoisins(MChunk * xprev, MChunk * xnext, MChunk * yprev, MChunk * ynext, MChunk * zprev, MChunk * znext)
		{
			Voisins[0] = xprev;
			Voisins[1] = xnext;
			Voisins[2] = yprev;
			Voisins[3] = ynext;
			Voisins[4] = zprev;
			Voisins[5] = znext;
		}

		void get_surrounding_cubes(int x, int y, int z, MCube ** cubeXPrev, MCube ** cubeXNext,
			MCube ** cubeYPrev, MCube ** cubeYNext,
			MCube ** cubeZPrev, MCube ** cubeZNext)
		{

			*cubeXPrev = NULL;
			*cubeXNext = NULL;
			*cubeYPrev = NULL;
			*cubeYNext = NULL;
			*cubeZPrev = NULL;
			*cubeZNext = NULL;

			if (x == 0 && Voisins[0] != NULL)
				*cubeXPrev = &(Voisins[0]->_Cubes[CHUNK_SIZE - 1][y][z]);
			else if (x > 0)
				*cubeXPrev = &(_Cubes[x - 1][y][z]);

			if (x == CHUNK_SIZE - 1 && Voisins[1] != NULL)
				*cubeXNext = &(Voisins[1]->_Cubes[0][y][z]);
			else if (x < CHUNK_SIZE - 1)
				*cubeXNext = &(_Cubes[x + 1][y][z]);

			if (y == 0 && Voisins[2] != NULL)
				*cubeYPrev = &(Voisins[2]->_Cubes[x][CHUNK_SIZE - 1][z]);
			else if (y > 0)
				*cubeYPrev = &(_Cubes[x][y - 1][z]);

			if (y == CHUNK_SIZE - 1 && Voisins[3] != NULL)
				*cubeYNext = &(Voisins[3]->_Cubes[x][0][z]);
			else if (y < CHUNK_SIZE - 1)
				*cubeYNext = &(_Cubes[x][y + 1][z]);

			if (z == 0 && Voisins[4] != NULL)
				*cubeZPrev = &(Voisins[4]->_Cubes[x][y][CHUNK_SIZE - 1]);
			else if (z > 0)
				*cubeZPrev = &(_Cubes[x][y][z - 1]);

			if (z == CHUNK_SIZE - 1 && Voisins[5] != NULL)
				*cubeZNext = &(Voisins[5]->_Cubes[x][y][0]);
			else if (z < CHUNK_SIZE - 1)
				*cubeZNext = &(_Cubes[x][y][z + 1]);
		}

		void render(bool transparent)
		{
			if (transparent)
				VboTransparent->render();
			else
				VboOpaque->render();
		}

		/**
		  * On verifie si le cube peut être vu
		  */
		bool test_hidden(int x, int y, int z)
		{
			MCube * cubeXPrev = NULL; 
			MCube * cubeXNext = NULL; 
			MCube * cubeYPrev = NULL; 
			MCube * cubeYNext = NULL; 
			MCube * cubeZPrev = NULL; 
			MCube * cubeZNext = NULL; 

			if(x == 0 && Voisins[0] != NULL)
				cubeXPrev = &(Voisins[0]->_Cubes[CHUNK_SIZE-1][y][z]);
			else if(x > 0)
				cubeXPrev = &(_Cubes[x-1][y][z]);

			if(x == CHUNK_SIZE-1 && Voisins[1] != NULL)
				cubeXNext = &(Voisins[1]->_Cubes[0][y][z]);
			else if(x < CHUNK_SIZE-1)
				cubeXNext = &(_Cubes[x+1][y][z]);

			if(y == 0 && Voisins[2] != NULL)
				cubeYPrev = &(Voisins[2]->_Cubes[x][CHUNK_SIZE-1][z]);
			else if(y > 0)
				cubeYPrev = &(_Cubes[x][y-1][z]);

			if(y == CHUNK_SIZE-1 && Voisins[3] != NULL)
				cubeYNext = &(Voisins[3]->_Cubes[x][0][z]);
			else if(y < CHUNK_SIZE-1)
				cubeYNext = &(_Cubes[x][y+1][z]);

			if(z == 0 && Voisins[4] != NULL)
				cubeZPrev = &(Voisins[4]->_Cubes[x][y][CHUNK_SIZE-1]);
			else if(z > 0)
				cubeZPrev = &(_Cubes[x][y][z-1]);

			if(z == CHUNK_SIZE-1 && Voisins[5] != NULL)
				cubeZNext = &(Voisins[5]->_Cubes[x][y][0]);
			else if(z < CHUNK_SIZE-1)
				cubeZNext = &(_Cubes[x][y][z+1]);

			if( cubeXPrev == NULL || cubeXNext == NULL ||
				cubeYPrev == NULL || cubeYNext == NULL ||
				cubeZPrev == NULL || cubeZNext == NULL )
				return false;

			if (cubeXPrev->isOpaque() == true && //droite
				cubeXNext->isOpaque() == true && //gauche
				cubeYPrev->isOpaque() == true && //haut
				cubeYNext->isOpaque() == true && //bas
				cubeZPrev->isOpaque() == true && //devant
				cubeZNext->isOpaque() == true)  //derriere
				return true;
			return false;
		}

		void disableHiddenCubes(void)
		{
			for(int x=0;x<CHUNK_SIZE;x++)
				for(int y=0;y<CHUNK_SIZE;y++)
					for(int z=0;z<CHUNK_SIZE;z++)
					{
						_Cubes[x][y][z].setDraw(true);
						if(test_hidden(x,y,z))
							_Cubes[x][y][z].setDraw(false);
					}
		}


};