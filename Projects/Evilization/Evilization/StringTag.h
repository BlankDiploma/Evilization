#include "stdafx.h"
class CHexUnit;

struct StringTagContext
{
	CHexUnit* pUnit;
	CHexCity* pCity;
	wchar_t bracketChar;
	StringTagContext()
	{
		bracketChar = '{';
	}
};

typedef void (*stringTagCallback)(const TCHAR* tag, TCHAR* pOut, StringTagContext* pSubject);

void formatStringTags(const char* fmt, TCHAR* pOut, stringTagCallback pCallback, StringTagContext* pContext);