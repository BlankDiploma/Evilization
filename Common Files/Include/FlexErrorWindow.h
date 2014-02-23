#include "stdafx.h"
#include "windows.h"
#include "strhashmap.h"
#include "resource.h"

struct ErrorTracker;
typedef stdext::hash_map<const wchar_t*, ErrorTracker, stringHasher> ErrorHash;

extern HWND hWndMain;
extern HINSTANCE hInst;

struct ErrorTracker
{
	const TCHAR* pchText;
	ErrorHash stFilenameToTracker;
	int Count;
	ErrorTracker* pParent;
	ErrorTracker()
	{
		pParent = NULL;
		pchText = NULL;
		Count = 0;
	}
	int id;
};

void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...);
#define Errorf(text, ...) ErrorInternalf(_T(text), NULL, __VA_ARGS__)
#define ErrorFilenamef(text, file, ...) ErrorInternalf(_T(text), _T(file), __VA_ARGS__)
#define ErrorAutoStructf(text, pStruct, ...) ErrorInternalf(_T(text), pStruct->filename, __VA_ARGS__)