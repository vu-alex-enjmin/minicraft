#pragma once

#include "engine/render/renderer.h"
#include "engine/render/vbo.h"
#include "cube.h"

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

			//Créer les VBO

			//Remplir les VBO
			
		}

		//Ajoute un quad du cube. Attention CCW
		int addQuadToVbo(YVbo * vbo, int iVertice, YVec3f & a, YVec3f & b, YVec3f & c, YVec3f & d, float type) {

			return 6;
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