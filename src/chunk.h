#pragma once

#include "engine/render/renderer.h"
#include "engine/render/vbo.h"
#include "cube.h"

#define FOUR_CORNERS(a,b,c,d) corners[a],corners[d],corners[c],corners[b]
// Système d'occlusion ambiante tirée de cet article : https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
#define EIGHT_AO_NEIGHBOURS(a,b,c,d,e,f,g,h) aoNeighbours[a], aoNeighbours[b], aoNeighbours[c], aoNeighbours[d], aoNeighbours[e], aoNeighbours[f], aoNeighbours[g], aoNeighbours[h]
#define AO_VALUE(side1,side2,corner) ((side1 && side2) ? 0 : (3 - (side1 + side2 + corner)))
#define AO_FLOAT(aoInt) (aoInt/3.0f)

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
						if (!_Cubes[x][y][z].getDraw())
							continue;

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
			VboOpaque = new YVbo(5, opaqueVertexCount, YVbo::PACK_BY_ELEMENT_TYPE);
			VboOpaque->setElementDescription(0, YVbo::Element(3)); //Sommet
			VboOpaque->setElementDescription(1, YVbo::Element(3)); //Normale
			VboOpaque->setElementDescription(2, YVbo::Element(2)); //UV
			VboOpaque->setElementDescription(3, YVbo::Element(1)); //Type
			VboOpaque->setElementDescription(4, YVbo::Element(1)); //Occlusion Ambiante
			VboOpaque->createVboCpu();

			VboTransparent = new YVbo(5, transparentVertexCount, YVbo::PACK_BY_ELEMENT_TYPE);
			VboTransparent->setElementDescription(0, YVbo::Element(3)); //Sommet
			VboTransparent->setElementDescription(1, YVbo::Element(3)); //Normale
			VboTransparent->setElementDescription(2, YVbo::Element(2)); //UV
			VboTransparent->setElementDescription(3, YVbo::Element(1)); //Type
			VboTransparent->setElementDescription(4, YVbo::Element(1)); //Occlusion Ambiante
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
						if (!_Cubes[x][y][z].getDraw())
							continue;

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
		int addQuadToVbo(
			YVbo * vbo, int iVertice, 
			YVec3f &a, YVec3f &b, YVec3f &c, YVec3f &d, 
			float type, 
			bool aoNeighbour0, bool aoNeighbour1, bool aoNeighbour2, bool aoNeighbour3, 
			bool aoNeighbour4, bool aoNeighbour5, bool aoNeighbour6, bool aoNeighbour7)
		{
			YVec3f n = (a - b).cross(c - b).normalize();

			int aoA = AO_VALUE(aoNeighbour7, aoNeighbour1, aoNeighbour0);
			int aoB = AO_VALUE(aoNeighbour5, aoNeighbour7, aoNeighbour6);
			int aoC = AO_VALUE(aoNeighbour3, aoNeighbour5, aoNeighbour4);
			int aoD = AO_VALUE(aoNeighbour1, aoNeighbour3, aoNeighbour2);

			if (aoA + aoC >= aoB + aoD)
			{
				// Premier triangle
				vbo->setElementValue(0, iVertice, a.X, a.Y, a.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoA)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, b.X, b.Y, b.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoB)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, c.X, c.Y, c.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoC)); // Occlusion Ambiante

				// Second triangle
				iVertice++;
				vbo->setElementValue(0, iVertice, c.X, c.Y, c.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoC)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, d.X, d.Y, d.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoD)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, a.X, a.Y, a.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoA)); // Occlusion Ambiante
			}
			else
			{
				// Premier triangle
				vbo->setElementValue(0, iVertice, a.X, a.Y, a.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoA)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, b.X, b.Y, b.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoB)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, d.X, d.Y, d.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoD)); // Occlusion Ambiante

				// Second triangle
				iVertice++;
				vbo->setElementValue(0, iVertice, d.X, d.Y, d.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 1, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoD)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, b.X, b.Y, b.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 0); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoB)); // Occlusion Ambiante

				iVertice++;
				vbo->setElementValue(0, iVertice, c.X, c.Y, c.Z); // Sommet
				vbo->setElementValue(1, iVertice, n.X, n.Y, n.Z); // Normale
				vbo->setElementValue(2, iVertice, 0, 1); // UV
				vbo->setElementValue(3, iVertice, type); // type
				vbo->setElementValue(4, iVertice, AO_FLOAT(aoC)); // Occlusion Ambiante
			}

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

			bool aoNeighbours[8 + 4 + 8];
			fillAoNeighbours(x, y, z, aoNeighbours);

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
					type,
					EIGHT_AO_NEIGHBOURS(12, 19, 18, 11, 6, 7, 0 ,8)
				);
			}
			
			// RIGHT (+X)
			if (faceShouldBeVisible(currentTypeIsTransparent, xNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(6, 5, 1, 2),
					type,
					EIGHT_AO_NEIGHBOURS(16, 15, 14, 9, 2, 3, 4, 10)
				);
			}
			
			// BACK (-Y)
			if (faceShouldBeVisible(currentTypeIsTransparent, yPrev))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(5, 4, 0, 1),
					type,
					EIGHT_AO_NEIGHBOURS(14, 13, 12, 8, 0, 1, 2, 9)
				);
			}

			// FRONT (+Y)
			if (faceShouldBeVisible(currentTypeIsTransparent, yNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(7, 6, 2, 3),
					type,
					EIGHT_AO_NEIGHBOURS(18, 17, 16, 10, 4, 5, 6, 11)
				);
			}

			// BOTTOM (-Z)
			if (faceShouldBeVisible(currentTypeIsTransparent, zPrev))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(1, 0, 3, 2),
					type,
					EIGHT_AO_NEIGHBOURS(2, 1, 0, 7, 6, 5, 4, 3)
				);
			}

			// UP (+Z)
			if (faceShouldBeVisible(currentTypeIsTransparent, zNext))
			{
				vertexCount += addQuadToVbo(
					vbo, iVertice + vertexCount,
					FOUR_CORNERS(4, 5, 6, 7),
					type,
					EIGHT_AO_NEIGHBOURS(12, 13, 14, 15, 16, 17, 18, 19)
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

		// Remplit les informations sur les voisins pour l'occlusion ambiante
		void fillAoNeighbours(const int x, const int y, const int z, bool* aoNeighbours)
		{
			// Couche 1 (3x3-1) : la couche 3x3 au dessus, moins le centre
			aoNeighbours[0] = cubeExistsAndIsOccluding(x - 1, y - 1, z - 1);
			aoNeighbours[1] = cubeExistsAndIsOccluding(x, y - 1, z - 1);
			aoNeighbours[2] = cubeExistsAndIsOccluding(x + 1, y - 1, z - 1);
			aoNeighbours[3] = cubeExistsAndIsOccluding(x + 1, y, z - 1);
			aoNeighbours[4] = cubeExistsAndIsOccluding(x + 1, y + 1, z - 1);
			aoNeighbours[5] = cubeExistsAndIsOccluding(x, y + 1, z - 1);
			aoNeighbours[6] = cubeExistsAndIsOccluding(x - 1, y + 1, z - 1);
			aoNeighbours[7] = cubeExistsAndIsOccluding(x - 1, y, z - 1);

			// Couche 2 (4) : les 4 coins de la couche 3x3 du milieu
			aoNeighbours[8] = cubeExistsAndIsOccluding(x - 1, y - 1, z);
			aoNeighbours[9] = cubeExistsAndIsOccluding(x + 1, y - 1, z);
			aoNeighbours[10] = cubeExistsAndIsOccluding(x + 1, y + 1, z);
			aoNeighbours[11] = cubeExistsAndIsOccluding(x - 1, y + 1, z);

			// Couche 3 (3x3-1) : la couche 3x3 en dessous, moins le centre
			aoNeighbours[12] = cubeExistsAndIsOccluding(x - 1, y - 1, z + 1);
			aoNeighbours[13] = cubeExistsAndIsOccluding(x, y - 1, z + 1);
			aoNeighbours[14] = cubeExistsAndIsOccluding(x + 1, y - 1, z + 1);
			aoNeighbours[15] = cubeExistsAndIsOccluding(x + 1, y, z + 1);
			aoNeighbours[16] = cubeExistsAndIsOccluding(x + 1, y + 1, z + 1);
			aoNeighbours[17] = cubeExistsAndIsOccluding(x, y + 1, z + 1);
			aoNeighbours[18] = cubeExistsAndIsOccluding(x - 1, y + 1, z + 1);
			aoNeighbours[19] = cubeExistsAndIsOccluding(x - 1, y, z + 1);
		}

		bool cubeExistsAndIsOccluding(const int x, const int y, const int z)
		{
			if (x < 0)
				return (Voisins[0] != NULL) && Voisins[0]->cubeExistsAndIsOccluding(x + CHUNK_SIZE, y, z);
			if (x >= CHUNK_SIZE)
				return (Voisins[1] != NULL) && Voisins[1]->cubeExistsAndIsOccluding(x - CHUNK_SIZE, y, z);
			if (y < 0)
				return (Voisins[2] != NULL) && Voisins[2]->cubeExistsAndIsOccluding(x, y + CHUNK_SIZE, z);
			if (y >= CHUNK_SIZE)
				return (Voisins[3] != NULL) && Voisins[3]->cubeExistsAndIsOccluding(x, y - CHUNK_SIZE, z);
			if (z < 0)
				return (Voisins[4] != NULL) && Voisins[4]->cubeExistsAndIsOccluding(x, y, z + CHUNK_SIZE);
			if (z >= CHUNK_SIZE)
				return (Voisins[5] != NULL) && Voisins[5]->cubeExistsAndIsOccluding(x, y, z - CHUNK_SIZE);

			return _Cubes[x][y][z].isOpaque();
		}
};