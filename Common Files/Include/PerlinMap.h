#include "stdafx.h"

#pragma once

class MTRand;

extern MTRand randomFloats;

enum GridDirection {kDir_PosX = 0, kDir_NegX, kDir_PosZ, kDir_NegZ, kDir_Count2D = 4, kDir_PosY = 4, kDir_NegY = 5, kDir_Count = 6};
#define OPPOSITE_DIRECTION(a) ((a) ^ 1)

class CPerlinMap
{
public:
	CPerlinMap();
	~CPerlinMap();
	float GetAt(int x, int y);
	void Generate(int size, int factor);
	void GenerateMutlipleLevels(int size, int min, int max, float** ppNeighborVectors);
	void AdditiveGenerate(int factor, float finalMulti, float** ppNeighborVectors, int neighboroffset);
	float GetAt(int x, int y, int z);
	void Generate3D(int size, int factor);
	void GenerateMutlipleLevels3D(int size, int min, int max, float** ppNeighborVectors);
	void AdditiveGenerate3D(int factor, float finalMulti, float** ppNeighborVectors, int neighboroffset);
	float* RelinquishSavedVectorOwnership(int direction);
	bool HasAnySavedVectors();
private:
	void Normalize();
	void Normalize3D();
	float* pMap;
	int size;
	float min, max;
	float* pSavedVectors[6];	//for 2d: posx posy negx negy    for 3d: posx posy posz negx negy negz
							//vectors are packed in order of increasing factor
};