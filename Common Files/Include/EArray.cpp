#include "StdAfx.h"
#include "EArray.h"
#include "assert.h"

using namespace std;

//standard
void eaCreate(void*** pData, int iSize, bool bStaticSize)
{
	assert (!(*pData));
	EArray* pArray = (EArray*)operator new(iSize * sizeof(void*) + EARRAY_HEADER_SIZE);
	pArray->iSize = bStaticSize ? iSize : 0;
	pArray->iMaxSize = iSize;
	memset(&pArray->pData, 0, iSize * sizeof(void*));
	*pData = (void**)(&pArray->pData);
}

void eaResize(void*** pData)
{
	assert(pData && *pData);

	EArray* pOld = EARRAY_FROM_HANDLE(pData);
	int iCap = pOld->iMaxSize;
	EArray* pArray = (EArray*)operator new(iCap * 2 * sizeof(void*) + EARRAY_HEADER_SIZE);

	memcpy(pArray, pOld, iCap * sizeof(void*) + EARRAY_HEADER_SIZE);

	pArray->iMaxSize = iCap*2;

	delete pOld;
	*pData = (void**)(&pArray->pData);
}

void eaPush_void(void*** pData, void* pObj)
{
	if (!pData || !(*pData))
	{
		eaCreate(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaResize(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	(*pData)[eaSize(pData)] = pObj;
	pArray->iSize++;
}

void eaInsert_void(void*** pData, void* pObj, int index)
{
	if (!pData || !(*pData))
	{
		eaCreate(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaResize(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	for (int i = eaSize(pData); i > index; i--)
	{
		(*pData)[i] = (*pData)[i-1];
	}
	(*pData)[index] = pObj;
	pArray->iSize++;
}

//Use RemoveFast when you don't care about the order of the array.
void eaRemoveFast_void(void*** pData, int index)
{
	if (!pData || !(*pData))
	{
		return;
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);

	if (index < 0 || index >= pArray->iSize)
		return;

	(*pData)[index] = (*pData)[eaSize(pData)-1];
	pArray->iSize--;
}

void eaRemove_void(void*** pData, int index)
{
	if (!pData || !(*pData))
	{
		return;
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);

	if (index < 0 || index >= pArray->iSize)
		return;

	for (int i = index; i < pArray->iSize-1; i++)
	{
		(*pData)[i] = (*pData)[i+1];
	}
	pArray->iSize--;
}

void eaDestroy_void(void*** pData)
{
	if (pData && *pData)
	{
		EArray* pArray = EARRAY_FROM_HANDLE(pData);
		delete pArray;
		*pData = NULL;
	}
}

void eaClear_void(void*** pData)
{
	if (pData && *pData)
	{
		EArray* pArray = EARRAY_FROM_HANDLE(pData);
		if (pArray)
		{
			pArray->iSize = 0;
			*pData = NULL;
		}
	}
}

void eaCopy_void(void*** pDst, void*** pSrc)
{
	if (!pDst || !pSrc || !(*pSrc))
		return;
	
	EArray* pSrcArray = EARRAY_FROM_HANDLE(pSrc);

	if (!(*pDst))
		eaCreate(pDst, pSrcArray->iMaxSize);
	else
		eaClear(pDst);

	EArray* pDstArray = EARRAY_FROM_HANDLE(pDst);

	pDstArray->iSize = pSrcArray->iSize;
	memcpy(&pDstArray->pData, &pSrcArray->pData, pSrcArray->iSize * sizeof(void*));
}


//int
void eaCreateInt(int** pData, int iSize, bool bStaticSize)
{
	assert (!(*pData));
	EArray* pArray = (EArray*)operator new(iSize * sizeof(int) + EARRAY_HEADER_SIZE);
	pArray->iSize = bStaticSize ? iSize : 0;
	pArray->iMaxSize = iSize;
	memset(&pArray->pData, 0, iSize * sizeof(int));
	*pData = (int*)(&pArray->pData);
}

void eaResizeInt(int** pData)
{
	assert(pData && *pData);

	EArray* pOld = EARRAY_FROM_HANDLE(pData);
	int iCap = pOld->iMaxSize;
	EArray* pArray = (EArray*)operator new(iCap * 2 * sizeof(int) + EARRAY_HEADER_SIZE);

	memcpy(pArray, pOld, iCap * sizeof(int) + EARRAY_HEADER_SIZE);

	pArray->iMaxSize = iCap*2;

	delete pOld;
	*pData = (int*)(&pArray->pData);
}

void eaPushInt(int** pData, int val)
{
	if (!(*pData))
	{
		eaCreateInt(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaResizeInt(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	(*pData)[eaSize(pData)] = val;
	pArray->iSize++;
}

void eaDestroyInt(int*** pData)
{
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	delete [] pArray->pData;
	delete pArray;
	*pData = NULL;
}

//float
void eaCreateFlt(float** pData, int iSize, bool bStaticSize)
{
	assert (!(*pData));
	EArray* pArray = (EArray*)operator new(iSize * sizeof(float) + EARRAY_HEADER_SIZE);
	pArray->iSize = bStaticSize ? iSize : 0;
	pArray->iMaxSize = iSize;
	memset(&pArray->pData, 0, iSize * sizeof(float));
	*pData = (float*)(&pArray->pData);
}

void eaResizeFlt(float** pData)
{
	assert(pData && *pData);

	EArray* pOld = EARRAY_FROM_HANDLE(pData);
	int iCap = pOld->iMaxSize;
	EArray* pArray = (EArray*)operator new(iCap * 2 * sizeof(float) + EARRAY_HEADER_SIZE);

	memcpy(pArray, pOld, iCap * sizeof(float) + EARRAY_HEADER_SIZE);

	pArray->iMaxSize = iCap*2;

	delete pOld;
	*pData = (float*)(&pArray->pData);
}

void eaPushFlt(float** pData, float val)
{
	if (!(*pData))
	{
		eaCreateFlt(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaResizeFlt(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	(*pData)[eaSize(pData)] = val;
	pArray->iSize++;
}

void eaDestroyFlt(float*** pData)
{
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	delete [] pArray->pData;
	delete pArray;
	*pData = NULL;
}