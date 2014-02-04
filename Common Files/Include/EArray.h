#include "StdAfx.h"
#include "assert.h"
#include <algorithm>

using namespace std;

#pragma once

#define EARRAY_DEFAULT_CAPACITY 4

struct EArray
{
	int iSize;
	int iMaxSize;
	void* pData;
};

#define EARRAY_HEADER_SIZE (sizeof(int) + sizeof(int))
#define EARRAY_FROM_HANDLE(a) (EArray*)(((char*)(*a)) - EARRAY_HEADER_SIZE)
#define eaSize(a) ((*((char**)a)) ? *((int*)((*((char**)a)) - EARRAY_HEADER_SIZE)) : 0)
#define eaCapacity(a) *((int*)(((char*)(*a)) - sizeof(int)))

//standard
void eaCreate(void*** pData, int iSize = EARRAY_DEFAULT_CAPACITY, bool bStaticSize = false);
void eaResize(void*** pData);
void eaPush_void(void*** pData, void* pObj);
void eaInsert_void(void*** pData, void* pObj, int index);
void eaClear_void(void*** pData);
void eaRemove_void(void*** pData, int index);
void eaRemoveFast_void(void*** pData, int index);
void eaDestroy_void(void*** pData);
void eaCopy_void(void*** pDst, void*** pSrc);
#define eaPush(a,b) eaPush_void((void***)a, (void*)b);
#define eaInsert(a,b,c) eaInsert_void((void***)a, (void*)b, c);
#define eaRemove(a,b) eaRemove_void((void***)a, b);
#define eaRemoveFast(a,b) eaRemoveFast_void((void***)a, b);
#define eaPop(a) eaRemoveFast_void((void***)a, eaSize(a)-1);
#define eaDestroy(a) eaDestroy_void((void***)a);
#define eaClear(a) eaClear_void((void***)a);
#define eaCopy(dst, src) eaCopy_void((void***)dst, (void***)src);


//stack
__forceinline void eaStackCreate(void*** pData, int iSize = EARRAY_DEFAULT_CAPACITY, bool bStaticSize = 0)
{
	assert (!(*pData));
	EArray* pArray = (EArray*)alloca(iSize * sizeof(void*) + EARRAY_HEADER_SIZE);
	pArray->iSize = bStaticSize ? iSize : 0;
	pArray->iMaxSize = iSize;
	memset(&pArray->pData, 0, iSize * sizeof(void*));
	*pData = (void**)(&pArray->pData);
}

__forceinline void eaStackResize(void*** pData)
{
	assert(pData && *pData);

	EArray* pOld = EARRAY_FROM_HANDLE(pData);
	int iCap = pOld->iMaxSize;
	EArray* pArray = (EArray*)alloca(iCap * 2 * sizeof(void*) + EARRAY_HEADER_SIZE);

	memcpy(pArray, pOld, iCap * sizeof(void*) + EARRAY_HEADER_SIZE);

	pArray->iMaxSize = iCap*2;

	*pData = (void**)(&pArray->pData);
}

__forceinline void eaStackPush_void(void*** pData, void* pObj)
{
	if (!(*pData))
	{
		eaStackCreate(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaStackResize(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	(*pData)[eaSize(pData)] = pObj;
	pArray->iSize++;
}

__forceinline void eaStackInsert_void(void*** pData, void* pObj, int index)
{
	if (!(*pData))
	{
		eaStackCreate(pData);
	}
	EArray* pArray = EARRAY_FROM_HANDLE(pData);
	if (eaSize(pData) == eaCapacity(pData))
	{
		eaStackResize(pData);
		pArray = EARRAY_FROM_HANDLE(pData);
	}
	for (int i = pArray->iSize; i > index; i--)
	{
		(*pData)[i] = (*pData)[i-1];
	}
	(*pData)[index] = pObj;
	pArray->iSize++;
}
#define eaStackPush(a,b) eaStackPush_void((void***)a, (void*)b);
#define eaStackInsert(a,b,c) eaStackInsert_void((void***)a, (void*)b, c);

//int
void eaCreateInt(int** pData, int iSize = EARRAY_DEFAULT_CAPACITY, bool bStaticSize = false);
void eaResizeInt(int** pData);
void eaPushInt(int** pData, int val);
void eaDestroyInt(int*** pData);

//float
void eaCreateFlt(float** pData, int iSize = EARRAY_DEFAULT_CAPACITY, bool bStaticSize = false);
void eaResizeFlt(float** pData);
void eaPushFlt(float** pData, float val);
void eaDestroyFlt(float*** pData);