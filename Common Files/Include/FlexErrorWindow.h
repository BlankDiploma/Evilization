#include "stdafx.h"
#include "strhashmap.h"

#ifndef FLEX_ERROR_USE_STDERR

#include "resource.h"
#include "windows.h"

struct ErrorTracker;
typedef stdext::hash_map<const TCHAR*, ErrorTracker, stringHasher> ErrorHash;

extern HWND hWndMain;
extern HINSTANCE hInst;

struct ErrorTracker
{
	const TCHAR* pchText;
	ErrorHash stFilenameToTracker;
	int Count;
	ErrorTracker* pParent;
	bool bForceAssert;
	ErrorTracker()
	{
		pParent = NULL;
		pchText = NULL;
		Count = 0;
		bForceAssert = 0;
	}
	int id;
};

void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...);
#define Errorf(text, ...) ErrorInternalf(_T(text), NULL, __VA_ARGS__)
#define ErrorFilenamef(text, file, ...) ErrorInternalf(_T(text), file, __VA_ARGS__)
#define ErrorAutoStructf(text, pStruct, ...) ErrorInternalf(_T(text), pStruct->filename, __VA_ARGS__)

#else
	
void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...);
#define Errorf(text, ...) ErrorInternalf(_T(text)_T("\n"), NULL, __VA_ARGS__)
#define ErrorFilenamef(text, file, ...) ErrorInternalf(_T(text)_T("\n"), file, __VA_ARGS__)
#define ErrorAutoStructf(text, pStruct, ...) ErrorInternalf(_T(text)_T("\n"), pStruct->filename, __VA_ARGS__)

#endif
