
#include "stdafx.h"
#include "PerlinMap.h"
#include "mtrand.h"
#include "math.h"
#include <d3dx9math.h>

CPerlinMap::CPerlinMap()
{
	pMap = NULL;
	size = 0;
}
CPerlinMap::~CPerlinMap()
{
	if (pMap)
		delete [] pMap;
}

#define dotprod(x, y, i, j) (((x)*(i)) + ((y)*(j)))

#define dotprod3D(x, y, z, i, j, k) (((x)*(i)) + ((y)*(j)) + ((z)*(k)))


inline float weightedAverage(float a, float b, float delta)
{
	return a + (3*delta*delta - 2*delta*delta*delta) * (b-a);
}

void CPerlinMap::Generate(int size, int factor)
{
	this->size = size;
	pMap = new float[size*size];
	int vectorSize = (size/factor)+1;
	FLOATPOINT* pVectors = new FLOATPOINT[vectorSize*vectorSize];
	for (int i = 0; i < vectorSize*vectorSize; i++)
	{
		float angle = ((float)randomFloats()) * 3.14159f * 2;
		pVectors[i].x = cos(angle);
		pVectors[i].y = sin(angle);
	}
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
		{
			float xDist = (i%factor + 0.25f)/factor;
			float yDist = (j%factor + 0.25f)/factor;
			int iVec = i/factor + (j/factor)*vectorSize;
			float a = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist, yDist);
			iVec = i/factor+1 + (j/factor)*vectorSize;
			float b = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist-1, yDist);
			iVec = i/factor + (j/factor+1)*vectorSize;
			float c = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist, yDist-1);
			iVec = i/factor+1 + (j/factor+1)*vectorSize;
			float d = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist-1, yDist-1);
			float val = weightedAverage(weightedAverage(a, b, xDist), weightedAverage(c, d, xDist), yDist);
			pMap[i+j*size] = val;//weightedAverage(weightedAverage(a, b, xDist/factor), weightedAverage(c, d, xDist/factor), yDist/factor);
		}
		delete [] pVectors;
}

void CPerlinMap::AdditiveGenerate(int factor, float finalMulti, float** ppNeighborVectors, int neighboroffset, bool bWrapX)
{
	int vectorSize = (size/factor)+1;
	FLOATPOINT* pVectors = new FLOATPOINT[vectorSize * vectorSize];
	for (int i = 0; i < vectorSize*vectorSize; i++)
	{
		float angle = ((float)randomFloats()) * 3.14159f * 2;
		pVectors[i].x = cos(angle);
		pVectors[i].y = sin(angle);
	}
	if (bWrapX)
	{
		for (int i = 0; i < vectorSize; i++)
		{
			pVectors[vectorSize*i].x = pVectors[vectorSize*i + vectorSize-1].x;
			pVectors[vectorSize*i].y = pVectors[vectorSize*i + vectorSize-1].y;
		}
	}
	//load angles from neighbors
	
	if (ppNeighborVectors)
	{
		for (int i = 0; i < vectorSize; i++)
		{
			if (ppNeighborVectors[kDir_PosX])
			{
				int index = (i+1)*vectorSize -1;
				pVectors[index].x = ppNeighborVectors[kDir_PosX][neighboroffset+i*2];
				pVectors[index].y = ppNeighborVectors[kDir_PosX][neighboroffset+i*2+1];
			}
			if (ppNeighborVectors[kDir_NegX])
			{
				int index = i;
				pVectors[index].x = ppNeighborVectors[kDir_NegX][neighboroffset+i*2];
				pVectors[index].y = ppNeighborVectors[kDir_NegX][neighboroffset+i*2+1];
			}
			if (ppNeighborVectors[kDir_PosZ])
			{
				int index = i*vectorSize;
				pVectors[index].x = ppNeighborVectors[kDir_PosZ][neighboroffset+i*2];
				pVectors[index].y = ppNeighborVectors[kDir_PosZ][neighboroffset+i*2+1];
			}
			if (ppNeighborVectors[kDir_NegZ])
			{
				int index = (vectorSize-1) * vectorSize + i;
				pVectors[index].x = ppNeighborVectors[kDir_NegZ][neighboroffset+i*2];
				pVectors[index].y = ppNeighborVectors[kDir_NegZ][neighboroffset+i*2+1];
			}
		}
	}

	//save angles for later
	for (int i = 0; i < vectorSize; i++)
	{
		if (pSavedVectors[kDir_PosX])
		{
			int index = (i+1)*vectorSize -1;
			pSavedVectors[kDir_PosX][neighboroffset+i*2] = pVectors[index].x;
			pSavedVectors[kDir_PosX][neighboroffset+i*2+1] = pVectors[index].y;
		}
		if (pSavedVectors[kDir_NegX])
		{
			int index = i;
			pSavedVectors[kDir_NegX][neighboroffset+i*2] = pVectors[index].x;
			pSavedVectors[kDir_NegX][neighboroffset+i*2+1] = pVectors[index].y;
		}
		if (pSavedVectors[kDir_PosZ])
		{
			int index = i*vectorSize;
			pSavedVectors[kDir_PosZ][neighboroffset+i*2] = pVectors[index].x;
			pSavedVectors[kDir_PosZ][neighboroffset+i*2+1] = pVectors[index].y;
		}
		if (pSavedVectors[kDir_NegZ])
		{
			int index = (vectorSize-1) * vectorSize + i;
			pSavedVectors[kDir_NegZ][neighboroffset+i*2] = pVectors[index].x;
			pSavedVectors[kDir_NegZ][neighboroffset+i*2+1] = pVectors[index].y;
		}
	}
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
		{
			float xDist = (i%factor + 0.25f)/factor;
			float yDist = (j%factor + 0.25f)/factor;
			int iVec = i/factor + (j/factor)*vectorSize;
			float a = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist, yDist);
			iVec = i/factor+1 + (j/factor)*vectorSize;
			float b = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist-1, yDist);
			iVec = i/factor + (j/factor+1)*vectorSize;
			float c = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist, yDist-1);
			iVec = i/factor+1 + (j/factor+1)*vectorSize;
			float d = dotprod(pVectors[iVec].x, pVectors[iVec].y, xDist-1, yDist-1);
			float val = weightedAverage(weightedAverage(a, b, xDist), weightedAverage(c, d, xDist), yDist);
			pMap[i+j*size] += val * finalMulti;//weightedAverage(weightedAverage(a, b, xDist/factor), weightedAverage(c, d, xDist/factor), yDist/factor);
		}
		delete [] pVectors;
}

void CPerlinMap::GenerateMutlipleLevels(int size, int min, int max, float** ppNeighborVectors, bool bWrapX)
{
	int num = 0;
	this->size = size;
	int neighboroffset = 0;
	int totalnumvectors = 0;
	pMap = new float[size*size];
	memset(pMap, 0, sizeof(float)*size*size);
	memset(pSavedVectors, 0, sizeof(float*)*kDir_Count);
	
	for (int i = max; i >= min; i = i >> 1)
	{
		totalnumvectors += (size/max+1);
	}

	if (ppNeighborVectors)
	{
		for (int i = 0; i < kDir_Count2D; i++)
		{
			if (!ppNeighborVectors[i])
				pSavedVectors[i] = new float[totalnumvectors*2];
		}
	}

	for (int i = max; i >= min; i = i >> 1)
	{
		AdditiveGenerate(i, pow(0.5f, num), ppNeighborVectors, neighboroffset, bWrapX);
		neighboroffset += (size/max+1)*2;
		num++;
	}
	Normalize();
	if (ppNeighborVectors)
	{
		for (int i = 0; i < kDir_Count2D; i++)
		{
			if (ppNeighborVectors[i])
				delete [] ppNeighborVectors[i];
		}
	}
}

void CPerlinMap::Generate3D(int size, int factor)
{
	D3DXMATRIX matRotation;
	this->size = size;
	pMap = new float[size*size*size];
	int vectorSize = (size/factor)+1;
	D3DXVECTOR3* pVectors = new D3DXVECTOR3[vectorSize*vectorSize*vectorSize];
	for (int i = 0; i < vectorSize*vectorSize*vectorSize; i++)
	{
		pVectors[i].x = 1.0f;
		pVectors[i].y = 0;
		pVectors[i].z = 0;
		D3DXMatrixRotationYawPitchRoll(&matRotation, ((float)randomFloats()) * 3.14159f * 2, ((float)randomFloats()) * 3.14159f * 2, ((float)randomFloats()) * 3.14159f * 2);
		D3DXVec3TransformCoord(&pVectors[i], &pVectors[i], &matRotation);
	}
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			for (int k = 0; k < size; k++)
			{
				float xDist = (i%factor + 0.25f)/factor;
				float yDist = (j%factor + 0.25f)/factor;
				float zDist = (k%factor + 0.25f)/factor;

				int iVec = i/factor + (j/factor)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float a = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist, zDist);
				iVec = i/factor+1 + (j/factor)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float b = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist, zDist);
				iVec = i/factor + (j/factor+1)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float c = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist-1, zDist);
				iVec = i/factor+1 + (j/factor+1)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float d = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist-1, zDist);
				
				iVec = i/factor + (j/factor)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float aa = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist, zDist-1);
				iVec = i/factor+1 + (j/factor)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float bb = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist, zDist-1);
				iVec = i/factor + (j/factor+1)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float cc = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist-1, zDist-1);
				iVec = i/factor+1 + (j/factor+1)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float dd = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist-1, zDist-1);

				float avg1 = weightedAverage(weightedAverage(a, b, xDist), weightedAverage(c, d, xDist), yDist);
				float avg2 = weightedAverage(weightedAverage(aa, bb, xDist), weightedAverage(cc, dd, xDist), yDist);
				float final = weightedAverage(avg1, avg2, zDist);
				pMap[i+ j*size + k*size*size] = final;//weightedAverage(weightedAverage(a, b, xDist/factor), weightedAverage(c, d, xDist/factor), yDist/factor);
			}
	delete [] pVectors;
}

void CPerlinMap::AdditiveGenerate3D(int factor, float finalMulti, float** ppNeighborVectors, int neighboroffset)
{
	D3DXMATRIX matRotation;
	int vectorSize = (size/factor)+1;
	D3DXVECTOR3* pVectors = new D3DXVECTOR3[vectorSize*vectorSize*vectorSize];
	for (int i = 0; i < vectorSize*vectorSize*vectorSize; i++)
	{
		pVectors[i].x = 1.0f;
		pVectors[i].y = 0;
		pVectors[i].z = 0;
		D3DXMatrixRotationYawPitchRoll(&matRotation, ((float)randomFloats()) * 3.14159f * 2, ((float)randomFloats()) * 3.14159f * 2, ((float)randomFloats()) * 3.14159f * 2);
		D3DXVec3TransformCoord(&pVectors[i], &pVectors[i], &matRotation);
	}
	//load angles from neighbors
	for (int i = 0; i < vectorSize*vectorSize; i++)
	{
		int j = i/vectorSize;
		int k = i%vectorSize;
		if (ppNeighborVectors[kDir_PosX])//posx
		{
			int index = j*vectorSize*vectorSize + (k+1)*vectorSize-1;
			pVectors[index].x = ppNeighborVectors[kDir_PosX][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_PosX][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_PosX][neighboroffset+i*3+2];
		}
		if (ppNeighborVectors[kDir_NegX])//negx
		{
			int index = j*vectorSize*vectorSize + k*vectorSize;
			pVectors[index].x = ppNeighborVectors[kDir_NegX][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_NegX][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_NegX][neighboroffset+i*3+2];
		}
		
		if (ppNeighborVectors[kDir_PosZ])//posz
		{
			int index = i;
			pVectors[index].x = ppNeighborVectors[kDir_PosZ][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_PosZ][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_PosZ][neighboroffset+i*3+2];
		}
		if (ppNeighborVectors[kDir_NegZ])//negz
		{
			int index = vectorSize*vectorSize*(vectorSize-1) + i;
			pVectors[index].x = ppNeighborVectors[kDir_NegZ][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_NegZ][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_NegZ][neighboroffset+i*3+2];
		}
		
		if (ppNeighborVectors[kDir_PosY])//posz
		{
			int index = j*vectorSize*vectorSize + k + (vectorSize-1)*vectorSize;
			pVectors[index].x = ppNeighborVectors[kDir_PosY][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_PosY][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_PosY][neighboroffset+i*3+2];
		}
		if (ppNeighborVectors[kDir_NegY])//negz
		{
			int index = j*vectorSize*vectorSize + k;
			pVectors[index].x = ppNeighborVectors[kDir_NegY][neighboroffset+i*3];
			pVectors[index].y = ppNeighborVectors[kDir_NegY][neighboroffset+i*3+1];
			pVectors[index].z = ppNeighborVectors[kDir_NegY][neighboroffset+i*3+2];
		}
	}
	//save for later
	for (int i = 0; i < vectorSize*vectorSize; i++)
	{
		int j = i/vectorSize;
		int k = i%vectorSize;
		if (pSavedVectors[kDir_PosX])//posx
		{
			int index = j*vectorSize*vectorSize + (k+1)*vectorSize-1;
			pSavedVectors[kDir_PosX][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_PosX][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_PosX][neighboroffset+i*3+2] = pVectors[index].z;
		}
		if (pSavedVectors[kDir_NegX])//negx
		{
			int index = j*vectorSize*vectorSize + k*vectorSize;
			pSavedVectors[kDir_NegX][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_NegX][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_NegX][neighboroffset+i*3+2] = pVectors[index].z;
		}
		
		if (pSavedVectors[kDir_PosZ])//posz
		{
			int index = i;
			pSavedVectors[kDir_PosZ][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_PosZ][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_PosZ][neighboroffset+i*3+2] = pVectors[index].z;
		}
		if (pSavedVectors[kDir_NegZ])//negz
		{
			int index = vectorSize*vectorSize*(vectorSize-1) + i;
			pSavedVectors[kDir_NegZ][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_NegZ][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_NegZ][neighboroffset+i*3+2] = pVectors[index].z;
		}
		
		if (pSavedVectors[kDir_PosY])//posz
		{
			int index = j*vectorSize*vectorSize + k + (vectorSize-1)*vectorSize;
			pSavedVectors[kDir_PosY][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_PosY][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_PosY][neighboroffset+i*3+2] = pVectors[index].z;
		}
		if (pSavedVectors[kDir_NegY])//negz
		{
			int index = j*vectorSize*vectorSize + k;
			pSavedVectors[kDir_NegY][neighboroffset+i*3] = pVectors[index].x;
			pSavedVectors[kDir_NegY][neighboroffset+i*3+1] = pVectors[index].y;
			pSavedVectors[kDir_NegY][neighboroffset+i*3+2] = pVectors[index].z;
		}
	}

	int arrayIndex = 0;
	//For performance reasons, this loop is arranged so that we can just write every index in order without needing to convert from i/j/k to arrayindex.
	
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			for (int k = 0; k < size; k++)
			{
				float xDist = (i%factor + 0.25f)/factor;
				float yDist = (j%factor + 0.25f)/factor;
				float zDist = (k%factor + 0.25f)/factor;
				
				int iVec = i/factor + (j/factor)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float a = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist, zDist);
				iVec = i/factor+1 + (j/factor)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float b = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist, zDist);
				iVec = i/factor + (j/factor+1)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float c = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist-1, zDist);
				iVec = i/factor+1 + (j/factor+1)*vectorSize + (k/factor)*vectorSize*vectorSize;
				float d = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist-1, zDist);
				
				iVec = i/factor + (j/factor)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float aa = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist, zDist-1);
				iVec = i/factor+1 + (j/factor)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float bb = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist, zDist-1);
				iVec = i/factor + (j/factor+1)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float cc = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist, yDist-1, zDist-1);
				iVec = i/factor+1 + (j/factor+1)*vectorSize + (k/factor+1)*vectorSize*vectorSize;
				float dd = dotprod3D(pVectors[iVec].x, pVectors[iVec].y, pVectors[iVec].z, xDist-1, yDist-1, zDist-1);

				float avg1 = weightedAverage(weightedAverage(a, b, xDist), weightedAverage(c, d, xDist), yDist);
				float avg2 = weightedAverage(weightedAverage(aa, bb, xDist), weightedAverage(cc, dd, xDist), yDist);
				float final = weightedAverage(avg1, avg2, zDist);

				final *= finalMulti;
				pMap[arrayIndex++] += final;//weightedAverage(weightedAverage(a, b, xDist/factor), weightedAverage(c, d, xDist/factor), yDist/factor);
			}
	delete [] pVectors;
}

void CPerlinMap::GenerateMutlipleLevels3D(int size, int min, int max, float** ppNeighborVectors)
{
	int neighboroffset = 0;
	int totalnumvectors = 0;

	this->size = size;
	pMap = new float[size*size*size];
	memset(pMap, 0, sizeof(float)*size*size);
	memset(pSavedVectors, 0, sizeof(float*)*kDir_Count);
	
	for (int i = max; i >= min; i = i >> 1)
	{
		totalnumvectors += (size/max+1);
	}

	for (int i = 0; i < kDir_Count; i++)
	{
		if (!ppNeighborVectors[i])
			pSavedVectors[i] = new float[totalnumvectors*3];
	}

	float multiplier = 1.0f;
	for (int i = max; i >= min; i = i >> 1)
	{
		AdditiveGenerate3D(i, multiplier, ppNeighborVectors, neighboroffset);
		neighboroffset += (size/max+1)*3;
		multiplier *= 0.5f;
	}
	Normalize3D();

	for (int i = 0; i < kDir_Count; i++)
	{
		if (ppNeighborVectors[i])
			delete [] ppNeighborVectors[i];
	}
}

void CPerlinMap::Normalize()
{
	float min = 1.0, max = 0.0;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			float val = GetAt(i, j);
			if (val < min)
				min = val;
			if (val > max)
				max = val;
		}
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			pMap[i+j*size] -= min;
			pMap[i+j*size] *= 1.0f/(max-min);
			//at this point everything should be in the range [0.0, 1.0] but to account for floating-point errors we'll cap 'em off.
			if (pMap[i+j*size] > 1.0f)
				pMap[i+j*size] = 1.0f;
			if (pMap[i+j*size] < 0.0f)
				pMap[i+j*size] = 0.0f;
		}
	}
}
void CPerlinMap::Normalize3D()
{
	float min = 1.0, max = 0.0;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			for (int k = 0; k < size; k++)
			{
				float val = GetAt(i, j, k);
				if (val < min)
					min = val;
				if (val > max)
					max = val;
			}
		}
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			for (int k = 0; k < size; k++)
			{
				int index = i + j*size + k*size*size;
				pMap[index] -= min;
				pMap[index] *= 1.0f/(max-min);
				//at this point everything should be in the range [0.0, 1.0] but to account for floating-point errors we'll cap 'em off.
				if (pMap[index] > 1.0f)
					pMap[index] = 1.0f;
				if (pMap[index] < 0.0f)
					pMap[index] = 0.0f;
			}
		}
	}
}

float CPerlinMap::GetAt(int x, int y)
{
	return pMap[x + y*size];
}

float CPerlinMap::GetAt(int x, int y, int z)
{
	return pMap[x + y*size + z*size*size];
}

float* CPerlinMap::RelinquishSavedVectorOwnership(int direction)
{
	float* pTemp = NULL;
	if (direction >= 0 && direction < kDir_Count && pSavedVectors[direction])
	{
		pTemp = pSavedVectors[direction];
		pSavedVectors[direction] = NULL;
	} 
	return pTemp;
}

bool CPerlinMap::HasAnySavedVectors()
{
	for (int i = 0; i < kDir_Count; i++)
	{
		if (pSavedVectors[i])
			return true;
	}
	return false;
}

