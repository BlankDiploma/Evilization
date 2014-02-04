// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "atlstr.h"
#include "assert.h"

#pragma once

#define CAtlString "Fuck you, don't even think about it."


struct Point
{
	LONG x, y;
	Point() {}
	operator POINT ()
	{
		POINT pt = {x,y};
		return pt;
	}
};

struct FLOATPOINT
{
	float x, y;
};

struct GfxSheetVertex
{
	float x , y , z , rhw ;
	DWORD diffuseColor ;
	float u , v ;
	float u1 , v1 ;
} ;

struct RAY
{
	int x;
	int y;
	float angle;//0-360
};

enum multiValType{MULTIVAL_NONE = 0, MULTIVAL_INT, MULTIVAL_FLOAT, MULTIVAL_PTR, MULTIVAL_ITEM, MULTIVAL_ACTORLIST, MULTIVAL_STRING, MULTIVAL_ACTOR, MULTIVAL_POINT};

struct multiVal
{
	float floatVal;
	int intVal;
	void* ptr;
	bool bPtrIsOwned;
	multiValType myType;
	POINT pt;

	void clear()
	{
		if (bPtrIsOwned)
		{
			if (myType = MULTIVAL_STRING)
				free((TCHAR*)ptr);
			else
				assert(1);//deleting unknown type
		}
		floatVal = 0.0;
		intVal = 0;
		ptr = NULL;
		bPtrIsOwned = false;
		myType = MULTIVAL_NONE;
	}

	void copyFrom(multiVal* pSrc)
	{
		clear();
		floatVal = pSrc->floatVal;
		intVal = pSrc->intVal;
		bPtrIsOwned = pSrc->bPtrIsOwned;
		myType = pSrc->myType;
		if (bPtrIsOwned)
		{
			if (myType = MULTIVAL_STRING)
			{
				ptr = wcsdup((TCHAR*)pSrc->ptr);
			}
			else
				assert(1);//copying unknown type
		}
		else
			ptr = pSrc->ptr;
	}

	multiVal()
	{
		bPtrIsOwned = false;
		clear();
	}

	void SetInt(int val)
	{
		floatVal = (float)val;
		intVal = val;
		myType = MULTIVAL_INT;
	}

	void SetPoint(POINT point)
	{
		pt = point;
		myType = MULTIVAL_POINT;
	}

	void SetFloat(float val)
	{
		intVal = (int)val;
		floatVal = val;
		myType = MULTIVAL_FLOAT;
	}

	void SetString(const TCHAR* string)
	{
		myType = MULTIVAL_STRING;
		ptr = _wcsdup(string);
		bPtrIsOwned = true;
	}

	void SetUnownedString(const TCHAR* string)
	{
		myType = MULTIVAL_STRING;
		ptr = (void*)string;
	}

	void SetPtr(void* data)
	{
		myType = MULTIVAL_PTR;
		ptr = data;
		bPtrIsOwned = false;
	}

	void parseFromString(const TCHAR* string)
	{
		if (string[0] == '\"')
		{
			TCHAR* newStr = wcsdup(string+1);
			newStr[wcslen(newStr)-1] = '\0';
			myType = MULTIVAL_STRING;;
			ptr = newStr;
			
			bPtrIsOwned = true;
		}
		else if (wcschr(string, '.'))
		{
			myType = MULTIVAL_FLOAT;
			floatVal = (float)_wtof(string);
		}
		else
		{
			myType = MULTIVAL_INT;
			intVal = _wtoi(string);
		}
	}

	BOOL GetBool()
	{
		switch (myType)
		{
		case MULTIVAL_NONE:
			{
				return false;
			}break;
		case MULTIVAL_INT:
			{
				return !!intVal;
			}break;
		case MULTIVAL_FLOAT:
			{
				return !!floatVal;
			}break;
		case MULTIVAL_STRING:
		case MULTIVAL_PTR:
			{
				return !!ptr;
			}break;
		}
		return false;
	}

	POINT GetPoint()
	{
		switch (myType)
		{
		case MULTIVAL_POINT:
			{
				return pt;
			}break;
		default:
			{
				POINT pt;
				pt.x = 0;
				pt.y = 0;
				return pt;
			}break;
		}
	}

	float GetFloat()
	{
		switch (myType)
		{
		case MULTIVAL_INT:
			{
				return (float)intVal;
			}break;
		case MULTIVAL_FLOAT:
			{
				return floatVal;
			}break;
		case MULTIVAL_NONE:
		case MULTIVAL_STRING:
		case MULTIVAL_PTR:
			{
				return 0;
			}break;
		}
		return 0;
	}

	int GetInt()
	{
		switch (myType)
		{
		case MULTIVAL_INT:
			{
				return intVal;
			}break;
		case MULTIVAL_FLOAT:
			{
				return (int)floatVal;
			}break;
		case MULTIVAL_NONE:
		case MULTIVAL_STRING:
		case MULTIVAL_PTR:
			{
				return 0;
			}break;
		}
		return 0;
	}

	void* GetPtr()
	{
		switch (myType)
		{
		case MULTIVAL_NONE:
		case MULTIVAL_INT:
		case MULTIVAL_FLOAT:
		case MULTIVAL_STRING:
			{
				return NULL;
			}break;
		case MULTIVAL_PTR:
			{
				return ptr;
			}break;
		}
		return NULL;
	}

	TCHAR* GetString()
	{
		switch (myType)
		{
		case MULTIVAL_NONE:
		case MULTIVAL_INT:
		case MULTIVAL_FLOAT:
		case MULTIVAL_PTR:
			{
				return NULL;
			}break;
		case MULTIVAL_STRING:
			{
				return ((TCHAR*)ptr);
			}break;
		}
		return NULL;
	}

};

#define RECT_WIDTH(a) (a##right - a##left)
#define RECT_HEIGHT(a) (a##bottom - a##top)
#define FOR_ALL_IN_RECT(a) for(int ix = (a)->left; ix < (a)->right; ix++) for(int iy = (a)->top; iy < (a)->bottom; iy++)

extern int g_HexSize;
#define HEX_SIZE g_HexSize
#define ZOOM_PERCENT (((float)HEX_SIZE)/DEFAULT_HEX_SIZE)

// TODO: reference additional headers your program requires here
